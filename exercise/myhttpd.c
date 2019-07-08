#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>
#include "functions.h"
#include "queue1.h"
#define MAX_THREADS 50
#define MAX_SIZE 200

//global vars
int end_flag = 0, shutdown_f = 0, s_sock, c_sock, file_c;
long int page_count = 0;
char **names; char ***Maps;
int *lines; 

Queue Q;									//linked list - queue with filedescriptors.
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t c_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t nonempty;
pthread_cond_t nonfull;

//thread function and signal handlers
void *consumer_thr(void *);
void place(Queue *, int);
void obtain(Queue *, int *);
void int_handler(int);

int main(int argc, char **argv)
{
	//check if arguments where given as asked
	if(argc != 9){
		fprintf(stderr, "Wrong arguments given.\n-Example:\t/myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\n");
		exit(-1);
	}
	
	//variables given through input
	int i, serv_port, cmd_port, thr_num, check_count=0;
	char *root_dir;
	for(i=0; i<argc; i++){
		if(strcmp(argv[i], "-p") == 0){
			check_count++;
			serv_port = atoi(argv[i+1]);
			if(serv_port <= 0){
				fprintf(stderr, "Error, port number must be a positive int.\n");
				exit(-1);
			}
		}
		else if(strcmp(argv[i], "-c") == 0){
			check_count++;
			cmd_port = atoi(argv[i+1]);
			if(cmd_port <= 0){
				fprintf(stderr, "Error, port number must be a positive int.\n");
				exit(-1);
			}
		}
		else if(strcmp(argv[i], "-t") == 0){
			check_count++;
			thr_num = atoi(argv[i+1]);
			if(thr_num <= 0){
				fprintf(stderr, "Error, num_of_threads must be positive int\n");
				exit(-1);
			}
			if(thr_num > MAX_THREADS)
				thr_num = MAX_THREADS;
		}
		else if(strcmp(argv[i], "-d") == 0){
			DIR* dir = opendir(argv[i+1]);
			if (ENOENT == errno){
				fprintf(stderr, "Error, root directory %s doesn't exist\n", argv[i+1]);
				exit(-1);
			}
		    closedir(dir);
			check_count++;
			root_dir = strdup(argv[i+1]);
		}
	}
	if(check_count != 4){
		fprintf(stderr, "Wrong arguments given.\n-Example:\t/myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\n");
		exit(-1);
	}
	if(serv_port == cmd_port){
		fprintf(stderr, "Wrong arguments given. Serving port and Command port must be different.\n");
		exit(-1);
	}

	printf("--> Initialized server with: serving port=%d, command port=%d, thread number=%d, root directory=%s\n\n", serv_port, cmd_port, thr_num, root_dir);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, int_handler);

	int j, err, newsock, acc, msg_size, pos;	
	pthread_t *cons;								//thread pool
	void *ret;
	time_t init_time, curr_time;
	init_time = time(NULL);

	//make map of files in memory
	DIR *dr; DIR *basic;
	struct dirent *entry; struct dirent *basic_entry;
	struct stat statbuf;
	pos=0;		char *temp; char *buffer;

	file_c = getFileCount(root_dir);
	Maps = malloc(file_c*sizeof(char **));
	lines = malloc(file_c*sizeof(int));							//!! get line count of every map opened.
	names = malloc(file_c*sizeof(char *));						//!! get the actual pathname for every file
	
	if((basic = opendir(root_dir)) == NULL){
		perror("opendir failed");
		return -1;
	}
	while((basic_entry = readdir(basic)) != NULL){
		if(strcmp(basic_entry->d_name, ".") == 0 || strcmp(basic_entry->d_name, "..") == 0)		//avoid ., .. (same and parent directory)
			continue;
		buffer = malloc((strlen(root_dir)+strlen(basic_entry->d_name))*sizeof(char)+2);
		strcpy(buffer, root_dir);
		strcat(buffer, "/");
		strcat(buffer, basic_entry->d_name);
		stat(buffer, &statbuf);
		if((statbuf.st_mode & S_IFMT) == S_IFDIR){
			//we are in sites.
			if((dr = opendir(buffer)) == NULL){
				perror("opendir failed");
				return -1;
			}
			while((entry = readdir(dr)) != NULL){
				temp = malloc((strlen(buffer)+strlen(entry->d_name))*sizeof(char)+2);
				strcpy(temp, buffer);
				strcat(temp, "/");
				strcat(temp, entry->d_name);
				stat(temp, &statbuf);
				if((statbuf.st_mode & S_IFMT) == S_IFREG){
					Maps[pos] = makeMap(temp, &lines[pos]);

					names[pos] = malloc(strlen(temp)*sizeof(char)+1);
					strcpy(names[pos], temp);
					pos++;
				}
				free(temp);
			}
			closedir(dr);
		}
		free(buffer);
	}
	closedir(basic);

	//------------------------------------------------------------
	//for serving port.
	struct sockaddr_in s_server, s_client;
	struct sockaddr *s_serverptr = (struct sockaddr *)&s_server;
	struct sockaddr *s_clientptr = (struct sockaddr *)&s_client;
	socklen_t s_clientlen;
	//for command port
	struct sockaddr_in c_server, c_client;
	struct sockaddr *c_serverptr = (struct sockaddr *)&c_server;
	struct sockaddr *c_clientptr = (struct sockaddr *)&c_client;
	socklen_t c_clientlen;

	struct hostent *host;

	//make sockets - reusable
	if((s_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		perror_exit("socket", 1);

	int enable = 1;
	if(setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror_exit("setsockopt", 1);

	if((c_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		perror_exit("socket", 1);

	if(setsockopt(c_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror_exit("setsockopt", 1);
	
	//initalize condition variables.
	pthread_cond_init(&nonempty, NULL);
	pthread_cond_init(&nonfull, NULL);

	//initialize thread pool and make threads
	queue_initialize(&Q);
	cons = malloc(thr_num*sizeof(pthread_t));
	for(i=0; i<thr_num; i++){
		if(err = pthread_create(&cons[i], NULL, consumer_thr, (void *)root_dir))
			perror_exit("pthread_create", 2);
	}
	
	//fix vars and bind sockets
	s_server.sin_family = AF_INET;
	s_server.sin_addr.s_addr = htonl(INADDR_ANY);
	s_server.sin_port = htons(serv_port);
	if(bind(s_sock, s_serverptr, sizeof(s_server)) < 0)
		perror_exit("bind", 1);

	c_server.sin_family = AF_INET;
	c_server.sin_addr.s_addr = htonl(INADDR_ANY);
	c_server.sin_port = htons(cmd_port);
	if(bind(c_sock, c_serverptr, sizeof(c_server)) < 0)
		perror_exit("bind", 1);

	//start listening for connections
	if(listen(s_sock, 128) < 0)
		perror_exit("listen", 3);
	printf("# Listening for connections in serving port %d (socket %d)\n", serv_port, s_sock);
	if(listen(c_sock, 128) < 0)
		perror_exit("listen", 3);
	printf("# Listening for connections in command port %d (socket %d)\n", cmd_port, c_sock);
	
	while(1){
		if(shutdown_f == 1)
			break;
		//wait for client to connect to any port
		newsock = accept_any(s_sock, &s_clientptr, s_clientlen, c_sock, &c_clientptr, c_clientlen, &acc);
		if(newsock == -1){
			printf("# Error in accepting client (newsock -1)\n");
			continue;
		}
		else{
			printf("\n# Accepted connection in ");
			if(acc == s_sock){
				//someone connected to serving port
				printf("serving port %d\n", serv_port);

				//place filedescriptor in queue and signal
				place(&Q, newsock);
				pthread_cond_signal(&nonempty);
			}
			else if(acc == c_sock){
				//someone connected to command port
				printf("command port %d\n", cmd_port);

				char *temp = malloc(BUFF_SIZE*sizeof(char));
				if(read_cmd(newsock, 10, &temp) < 0){
					printf("# Error in reading from socket. Connection closed.\n");
					continue;
				}

				//got command, execute.
				if(strcmp(temp, "STATS") == 0){
					curr_time = time(NULL);
					int dif = (int)difftime(curr_time, init_time);
					int min = dif / 60;
					int secs = dif - (min*60);

					char *stats = malloc(BUFF_SIZE*sizeof(char));
					if(page_count-1 < 0)
						sprintf(stats, "Server up for %d:%d , served %d pages\n", min, secs, 0);
					else
						sprintf(stats, "Server up for %d:%d , served %ld pages\n", min, secs, page_count-1);
					for(i=0; stats[i] != '\0'; i++){
						if(write(newsock, &stats[i], 1) < 0)
							perror_exit("write1", 4);
					}
					free(stats);
				}
				else if(strcmp(temp, "SHUTDOWN") == 0){
					shutdown_f = 1;

					//inform about shutting down
					char *msg = strdup("Server shutting down.\n");
					for(i=0; msg[i] != '\0'; i++){
						if(write(newsock, &msg[i], 1) < 0)
							perror_exit("write2", 4);
					}
					free(msg);

					//issue threads to exit
					end_flag = 1;
					pthread_cond_broadcast(&nonempty);
				}
				else{
					//unknown command
					char *error_m = strdup("Unknown command\n");
					for(i=0; error_m[i] != '\0'; i++){
						if(write(newsock, &error_m[i], 1) < 0)
							perror_exit("write3", 4);
					}
					free(error_m);
				}
				free(temp);
				close(newsock);
			}
		}
	}

	printf("# Server shutting down.\n");

	for(i=0; i<thr_num; i++){
		if(err = pthread_join(cons[i], &ret))
			perror_exit("pthread_join", 5);
	}
	pthread_cond_destroy(&nonempty);
	pthread_cond_destroy(&nonfull);	
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&c_mutex);

	for(i=0; i<file_c; i++){
		for(j=0; j<lines[i]; j++){
			free(Maps[i][j]);
		}
		free(Maps[i]);
	}
	free(Maps);
	for(i=0; i<file_c; i++){
		free(names[i]);
	}
	free(names);
	free(lines);

	free(cons);
	queue_freeQ(&Q);
	close(s_sock);
	close(c_sock);
	free(root_dir);
	exit(0);
}

//*------------------------------------------------------------------------------*

void *consumer_thr(void *arg)
{
	int fd, msg_size, i;
	signal(SIGUSR1, c_handler);
	char *root_dir = (char *)arg;
	while(!queue_isEmpty(&Q) || end_flag==0){
		obtain(&Q, &fd);
		if(fd != -1){
			pthread_cond_signal(&nonfull);

			//read request from socket.
			int line_c, r_errno, c_errno;
			char **msg;	char *url;
			if((msg = read_header(fd, 30, &line_c, &r_errno)) != NULL){
				printf("\n--Printing message given:\n");
				printMap(msg, line_c);

				//fix \r.
				for(i=0; i<line_c; i++){
					if(msg[i][strlen(msg[i])-1] == '\r')
						msg[i][strlen(msg[i])-1] == '\0';
				}

				if(msg_check(msg, line_c, &url) < 0){				//checking request syntax
					//wrong syntax.
					if(write_answer(fd, 4, NULL, 0, NULL) < 0)
						printf("# Error in writing answer\n");
				}
				else{
					char *f_name;
					if(strlen(root_dir) > (BUFF_SIZE/2))
						f_name = malloc(2*BUFF_SIZE*sizeof(char));
					else
						f_name = malloc(BUFF_SIZE*sizeof(char));
					strcpy(f_name, root_dir);
					strcat(f_name, url);
					if(!file_exist(f_name)){
						//file doesnt exist.
						if(write_answer(fd, 2, NULL, 0, NULL) < 0)
							printf("# Error in writing answer\n");
					}
					else if(get_perms(f_name) < 0){
						//not permitted for read or write
						if(write_answer(fd, 3, NULL, 0, NULL) < 0)
							printf("# Error in writing answer\n");
					}
					else{
						//all ok, will return message.
						pthread_mutex_lock(&c_mutex);
						for(i=0; i<file_c; i++){
							if(strcmp(f_name, names[i])==0){
								if(write_answer(fd, 1, Maps[i], lines[i], &page_count) < 0)
									printf("# Error in writing answer\n");
								break;
							}
						}
						
						pthread_mutex_unlock(&c_mutex);
					}
					free(f_name);
				}
			}
			else{
				printf("# Reading request encountered problem:\n");
				switch(r_errno){
					case -1:
						printf("\tError in given input.\n");	break;
					case -2:
						printf("\tError occured in reading from socket\n");	break;
					case -3:
						printf("\tTimeout occured\n");	break;
				}
				if(write_answer(fd, 4, NULL, 0, NULL) < 0)
						printf("# Error in writing answer, connection closed.\n");
			}
			//clear space.
			close(fd);
			if(r_errno > -1){
				for(i=0; i<10; i++)
					free(msg[i]);
				free(msg);
			}
		}
	}
	pthread_exit(NULL);
}

void obtain(Queue *q, int *fd)
{
	//function used by consumer threads, obtains a value from fd queue.
	int data;

	pthread_mutex_lock(&mutex);
	if(end_flag == 1 && queue_isEmpty(q)){
		*fd = -1;
		return;
	}
	while(queue_isEmpty(q)){
		pthread_cond_wait(&nonempty, &mutex);
		if(end_flag == 1 && queue_isEmpty(q)){
			pthread_mutex_unlock(&mutex);
			pthread_cond_broadcast(&nonempty);
			*fd = -1;
			return;
		}
	}

	pthread_mutex_lock(&c_mutex);
	queue_remove(q, &data);
	pthread_mutex_unlock(&c_mutex);

	pthread_mutex_unlock(&mutex);
	*fd = data;
}

void place(Queue *q, int x)
{
	pthread_mutex_lock(&mutex);
	while(q->count >= MAX_SIZE)
		pthread_cond_wait(&nonfull, &mutex);
	queue_insert(q, x);
	pthread_mutex_unlock(&mutex);
}


void int_handler(int signum)
{
	//web server catches interrupt in order to close filedescriptors.
	printf("\n-Closing sockets...\n");
	close(s_sock);
	close(c_sock);
	queue_freeQ(&Q);
	exit(-10);
}

