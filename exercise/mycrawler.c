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
#include "functions.h"
#include "queue2.h"
#include "hashtable.h"
#define MAX_THREADS 50
#define MAX_SIZE 200

int end_flag = 0, shutdown_f = 0, made_flag=0, c_sock, port, worker_c=0;
long int page_count = 0;
char *save_dir;

url_queue urls;										//!! queue of urls to be asked by server
hash_table *urls_found;								//!! hash table of found urls - at the end will have all unique filenames.
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t c_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t t_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t e_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t nonempty;
pthread_cond_t nonfull;

struct hostent *host;
//for workers
struct FifoFd* fds;
pid_t *pids;

//thread function and signal handlers
void *consumer_thr(void *);
void place(url_queue *, char *);
void obtain(url_queue *, char **);
void int_handler(int);

int main(int argc, char **argv)
{
	//check if arguments where given as asked
	if(argc != 12){
		fprintf(stderr, "Wrong arguments given.\n-Example:\t./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\n");
		exit(-1);
	}
	//variables given through input
	int i, cmd_port, thr_num, check_count=0, k;
	char *start_url;
	//struct hostent *host;
	for(i=0; i<argc; i++){
		if(strcmp(argv[i], "-h")==0){
			if((host = gethostbyname(argv[i+1])) == NULL)
				herror("gethostbyname");
			else
				check_count++;
		}
		else if(strcmp(argv[i], "-p")==0){
			check_count++;
			port = atoi(argv[i+1]);
			if(port <= 0){
				fprintf(stderr, "Error, port number must be a positive int.\n");
				exit(-1);
			}
		}
		else if(strcmp(argv[i], "-c")==0){
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
			check_count++;
			save_dir = strdup(argv[i+1]);
			strcpy(save_dir, argv[i+1]);
		}
	}
	if(check_count != 5){
		fprintf(stderr, "Wrong arguments given.\n-Example:\t./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\n");
		exit(-1);
	}
	if(port == cmd_port){
		fprintf(stderr, "Wrong arguments given. Serving port and Command port must be different.\n");
		exit(-1);
	}
	char *needle;
	if((needle = strstr(argv[argc-1], ":")) != NULL){
		check_count = 0;
		while(check_count < 3){
			if(needle[0] == '/')
				check_count++;
			needle+=1;
		}
		needle -= 1;
		//start_url = malloc(strlen(needle)*sizeof(char)+1);
		//strcpy(start_url, needle);
		start_url = strdup(needle);
	}
	else{
		//start_url = malloc(strlen(argv[argc-1])*sizeof(char)+1);
		//strcpy(start_url, argv[argc-1]);
		start_url = strdup(argv[argc-1]);
	}
	printf("\n-->Initialized crawler with: port=%d, command port=%d, thread number=%d, save directory=%s, starting url=%s, host %s\n", port, cmd_port, thr_num, save_dir, start_url, host->h_name);

	int j, err, newsock, pos, msg_size, flag, count, n, code;	char c;
	pthread_t *cons;								//thread pool
	void *ret;
	time_t init_time, curr_time;
	init_time = time(NULL);

	//Clear already existing directory if it has files.
	DIR *dr; DIR *basic;
	struct dirent *entry; struct dirent *basic_entry;
	struct stat statbuf;
	pos=0;		char *temp; char *buffer;
	char *path_file = malloc(BUFF_SIZE*sizeof(char));
	char **arg = (char **)malloc(12*sizeof(char *));

	if((basic = opendir(save_dir)) == NULL){
		mkdir(save_dir, 0755);
		basic = opendir(save_dir);
	}
	while((basic_entry = readdir(basic)) != NULL){
		if(strcmp(basic_entry->d_name, ".") == 0 || strcmp(basic_entry->d_name, "..") == 0)		//avoid ., .. (same and parent directory)
			continue;
		buffer = malloc((strlen(save_dir)+strlen(basic_entry->d_name))*sizeof(char)+2);
		strcpy(buffer, save_dir);
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
					if(remove(temp) != 0)
						perror_exit("remove", 1);
				}
				free(temp);
			}
			closedir(dr);
			if(rmdir(buffer)<0)
				perror_exit("rmdir", 1);
		}
		free(buffer);
	}
	closedir(basic);

	signal(SIGINT, int_handler);
	signal(SIGPIPE, SIG_IGN);
	//for command port - for this, mycrawler will be the server.
	struct sockaddr_in c_server, c_client;
	struct sockaddr *c_serverptr = (struct sockaddr *)&c_server;
	struct sockaddr *c_clientptr = (struct sockaddr *)&c_client;
	socklen_t c_clientlen;

	//make sockets - reusable
	int enable = 1;
	if((c_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		perror_exit("socket", 1);

	if(setsockopt(c_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror_exit("setsockopt", 1);

	//initalize condition variables.
	pthread_cond_init(&nonempty, NULL);
	pthread_cond_init(&nonfull, NULL);

	//initiliaze poll, bind etc.
	uqueue_initialize(&urls);
	place(&urls, start_url);
	
	urls_found = h_New();

	cons = malloc(thr_num*sizeof(pthread_t));
	for(i=0; i<thr_num; i++){
		if(err = pthread_create(&cons[i], NULL, consumer_thr, (void *)save_dir))
			perror_exit("pthread_create", 2);
	}

	c_server.sin_family = AF_INET;
	c_server.sin_addr.s_addr = htonl(INADDR_ANY);
	c_server.sin_port = htons(cmd_port);
	if(bind(c_sock, c_serverptr, sizeof(c_server)) < 0)
		perror_exit("bind", 1);

	if(listen(c_sock, 5) < 0)
		perror_exit("listen", 3);
	printf("-Listening for connections in command port %d (socket %d)\n", cmd_port, c_sock);
	while(1){
		if(shutdown_f == 1)
			break;
		//waiting for anyone to connect to cmd_port
		if((newsock = accept(c_sock, c_clientptr, &c_clientlen)) < 0)
			perror_exit("accept", 2);
		
		printf("\n-Accepted a connection in command port\n");

		char *temp = malloc(BUFF_SIZE*sizeof(char));
		char *needle;
		if(read_cmd(newsock, 20, &temp) < 0){
			printf("Error in reading from socket.Connection closed.\n");
			continue;
		}

		//got command, execute.
		if(strcmp(temp, "STATS") == 0){
			curr_time = time(NULL);
			int dif = (long int)difftime(curr_time, init_time);
			int min = dif / 60;
			int secs = dif - (min*60);

			char *stats = malloc(BUFF_SIZE*sizeof(char));
			sprintf(stats, "Crawler up for %d:%d , downloaded %ld pages\n", min, secs, page_count-1);
			for(i=0; stats[i] != '\0'; i++){
				if(write(newsock, &stats[i], 1) < 0)
					perror_exit("write", 4);
			}
			free(stats);
		}
		else if(strcmp(temp, "SHUTDOWN") == 0){
			shutdown_f = 1;

			//inform about shutting down
			char *msg = strdup("Crawler shutting down.\n");
			for(i=0; msg[i] != '\0'; i++){
				if(write(newsock, &msg[i], 1) < 0)
					perror_exit("write", 4);
			}
			free(msg);

			//issue threads to exit
			end_flag = 1;
			pthread_cond_broadcast(&nonempty);

			//issue workers to exit
			code = EXIT_CODE;
			for(i=0; i<worker_c; i++){
				write(fds[i].fd_pin, &code, sizeof(int));
				kill(pids[i], SIGUSR2);
			}
			int pd, status;
			while((pd = wait(&status)) > 0)
				;
		}
		else if((needle = strstr(temp, "SEARCH")) != NULL){
			char *answer;	char *token;
			if(!uqueue_isEmpty(&urls)){
				answer = strdup("Crawling still in progress...\n");
				for(i=0; answer[i] != '\0'; i++){
					if(write(newsock, &answer[i], 1) < 0)
						perror_exit("write", 4);
				}
				free(answer);
			}
			else{
				char **arg = (char **)malloc(12*sizeof(char *));
				i=0;	token = strtok(temp, " ");
				while(token != NULL){
					token = strtok(NULL, " ");
					arg[i] = token;
					i++;											//will have i-1 arguments.
					if(i > 10){
						arg[i]=NULL;
						break;
					}
				}
				if(i==1){
					answer = strdup("Error: invalid arguments of SEARCH\n");
					for(i=0; answer[i] != '\0'; i++){
						if(write(newsock, &answer[i], 1) < 0)
							perror_exit("write", 4);
					}
					free(answer);
				}
				else{
					int arg_c = i-1;
					char *error_message = malloc(BUFF_SIZE*sizeof(char));
					sprintf(error_message, "Technical error occured in crawler.\n");		//message printed to client connected if error.

					//file "sites.txt" at save_dir is analyzed - workers will be given a site each to work!
					if(made_flag == 0){
						FILE *fp; int line_count=0;
						strcpy(path_file, save_dir);
						strcat(path_file, "/sites.txt");
						//open text
						if((fp = fopen(path_file, "w")) == NULL){
							for(k=0; error_message[k] != '\0'; k++){
								if(write(newsock, &error_message[k], 1) < 0)
									perror_exit("write", 4);
							}
							perror_exit("fopen", -2);
						}
						//open dirs etc.
						if((dr = opendir(save_dir)) == NULL){
							for(k=0; error_message[k] != '\0'; k++){
								if(write(newsock, &error_message[k], 1) < 0)
									perror_exit("write", 4);
							}
							perror_exit("opendir", -1);
						}
						while((entry = readdir(dr)) != NULL){
							if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, "sites.txt") != 0){
								char *temp = malloc(BUFF_SIZE*sizeof(char));
								sprintf(temp, "%s/%s", save_dir, entry->d_name);
								fprintf(fp, "%s\n", temp);
								line_count++;
								free(temp);
							}
						}
						closedir(dr);
						fclose(fp);
						made_flag = 1;

						//keep site names in memory
						char **map;
						if((map = makeMap(path_file, &line_count))==NULL){
							fprintf(stderr, "Error in making paths file, exiting...\n");
							for(k=0; error_message[k] != '\0'; k++){
								if(write(newsock, &error_message[k], 1) < 0)
									perror_exit("write", 4);
							}
							return 1;
						}

						//this 1st time, make Workers.
						worker_c = line_count;
						fds = malloc(worker_c*sizeof(struct FifoFd));					//array of filedescriptors of every fifo about to open.
						pids = malloc(worker_c*sizeof(pid_t));

						for(i=0; i<worker_c; i++){
							if((pids[i] = MakeWorker(i, &fds[i])) < 0){
								fprintf(stderr, "-Couldnt make worker due to system problem, searching cannot be done.\n");
								for(k=0; error_message[k] != '\0'; k++){
									if(write(newsock, &error_message[k], 1) < 0)
										perror_exit("write", 4);
								}
								exit(-3);
							}
						}
						sleep(1);
						pos=0;
						j=1;
						for(i=0; i<worker_c; i++){
							while(write(fds[i].fd_pin, &j, sizeof(int)) <= 0)
								;
							msg_size = strlen(map[pos])+1;
							write(fds[i].fd_pin, &msg_size, sizeof(int));					//write the message size
							write(fds[i].fd_pin, map[pos], msg_size);						//write the actual message
							pos++;
							kill(pids[i], SIGUSR1);											//signal workers to start working
						}
						flag=0; count=0;
						while(1){															//check codes sent from workers - if error due to system problem, end.
							for(i=0; i<worker_c; i++){
								if((n=read(fds[i].fd_pout, &code, sizeof(int))) > 0){
									count++;
									if(code==ERROR_CODE)
										flag=1;
								}
							}
							if(count==worker_c)
								break;
						}
						if(flag==1){
							fprintf(stderr, "Error code received, searching cannot be done.\n");
							for(k=0; error_message[k] != '\0'; k++){
								if(write(newsock, &error_message[k], 1) < 0)
									perror_exit("write", 4);
							}
							exit(-3);
						}
					}

					//!! searching
					int f_count=0, end_count=0, doc_id;
					code = SEARCH_CODE; 
					int *found = malloc(worker_c*sizeof(int));
					//give all the arguments to the workers
					for(i=0; i<worker_c; i++){
						while((n = write(fds[i].fd_pin, &code, sizeof(int))) <= 0)
							;
						kill(pids[i], SIGUSR2);
						while((n = write(fds[i].fd_pin, &arg_c, sizeof(int))) <= 0)
							;
						for(j=0; j<arg_c; j++){
							msg_size = strlen(arg[j])+1;
							while((n = write(fds[i].fd_pin, &msg_size, sizeof(int))) <= 0)
								;
							while((n = write(fds[i].fd_pin, arg[j], msg_size)) <= 0)
								;
						}
						found[i]=-1;
					}
					while(1){
						for(i=0; i<worker_c; i++){
							if((n = read(fds[i].fd_pout, &code, sizeof(int))) > 0){
								if(code == OK_CODE){
									if(found[i]<0)
										found[i] = 1;

									while((n = read(fds[i].fd_pout, &msg_size, sizeof(int))) <= 0)
										;
									char temp_name[msg_size];
									while((n = read(fds[i].fd_pout, temp_name, msg_size)) <= 0)
										;
									while((n = read(fds[i].fd_pout, &doc_id, sizeof(int))) <= 0)
										;
									while((n = read(fds[i].fd_pout, &msg_size, sizeof(int))) <= 0)
										;
									char temp[msg_size];
									while((n = read(fds[i].fd_pout, temp, msg_size)) <= 0)
										;
									answer = malloc(BUFF_SIZE*2*sizeof(char));
									sprintf(answer, "--File %s, line %d:\n", temp_name, doc_id);
									for(k=0; answer[k] != '\0'; k++){
										if(write(newsock, &answer[k], 1) < 0)
											perror_exit("write", 4);
									}
									sprintf(answer, "%s\n\n", temp);
									for(k=0; answer[k] != '\0'; k++){
										if(write(newsock, &answer[k], 1) < 0)
											perror_exit("write", 4);
									}
									free(answer);
								}
								else if(code == END_CODE)
									end_count++;
							}
						}
						if(end_count==worker_c)
							break;
					}
					f_count=0;
					for(i=0; i<worker_c; i++){
						if(found[i]>0)
							f_count++;
					}
					if(f_count <= worker_c)
						printf("\n-- %d out of %d workers answered\n", f_count, worker_c);
					free(found);

					free(error_message);
				}
			}
		}
		else{
			//unknown command
			char *error_m = strdup("Unknown command\n");
			for(i=0; error_m[i] != '\0'; i++){
				if(write(newsock, &error_m[i], 1) < 0)
					perror_exit("write", 4);
			}
			free(error_m);
		}
		free(temp);
		close(newsock);		
	}

	printf("\n-Crawler shutting down.\n");
	//join threads
	for(i=0; i<thr_num; i++){
		if(err = pthread_join(cons[i], &ret))
			perror_exit("pthread_join", 5);
	}

	pthread_cond_destroy(&nonempty);
	pthread_cond_destroy(&nonfull);	
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&c_mutex);
	pthread_mutex_destroy(&t_mutex);
	pthread_mutex_destroy(&e_mutex);

	uqueue_freeQ(&urls);
	h_DelTable(urls_found);
	free(cons);
	free(start_url);
	free(fds);
	free(save_dir);
	free(path_file);
	close(c_sock);
	CloseFIFOs(fds, worker_c);
	free(arg);
	exit(0);
}

//*---------------------------------------------------------------------------------------*

void *consumer_thr(void *arg)
{
	int msg_size, i, sock, enable;
	char *item;

	char *save_dir = (char *)arg;
	while(!uqueue_isEmpty(&urls) || end_flag==0){
		obtain(&urls, &item);
		if(item != NULL){
			pthread_cond_signal(&nonfull);

			//fix vars
			struct sockaddr_in server;
			struct sockaddr *serverptr = (struct sockaddr *)&server;
			server.sin_family = AF_INET;
			memcpy(&server.sin_addr, host->h_addr, host->h_length);
			server.sin_port = htons(port);
			//make socket - reusable
			if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
				perror_exit("socket", 1);

			if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
				perror_exit("setsockopt", 1);

			//open connection
			if(connect(sock, serverptr, sizeof(server)) < 0)
				perror_exit("connect", 3);
			printf("Connecting to server in port %d\n\n", port);

			//write request
			if(write_request(sock, item) < 0){
				fprintf(stderr, "Error in sending request to server\n");
				exit(6);
			}
			
			//read and manage response
			int err, lines=0, eric=0, marag=0, stel=0;
			err = read_and_manage(sock, t_mutex, item, save_dir, &lines, &page_count);
			if(err < 0)
				;//printf("#%ld: read_and_manage returned negative responses\n", pthread_self());
			else{
				//printf("#%ld: read_and_manage returned positive responses for %s, analyzing...(lines=%d)\n", pthread_self(), item, lines);
				char *name = malloc(BUFF_SIZE*sizeof(char));
				sprintf(name, "%s%s", save_dir, item);

				int size = BUFF_SIZE*2;
				FILE *fp; char buffer[size];
				if((fp = fopen(name, "r")) == NULL)
					perror_exit("fopen", 5);
				for(i=0; i<lines; i++){
					fgets(buffer, size, fp);
					if(buffer[0] == '<' && buffer[1] == 'a'){
						char *token = strtok(buffer, "'");
						token = strtok(NULL, "'");
						token += 2;

						//if url seen for the 1st time, insert in queue.
						pthread_mutex_lock(&e_mutex);
						char *temp = h_search(urls_found, token);
						if(temp != NULL){
							//printf("(%d)\t%ld, %s:url %s already used, will not insert\n", ++eric, pthread_self(), item, token);
						}
						if(temp == NULL){
							//printf("(%d)\t%ld, %s:url %s seen for 1st time, inserting in queue\n", ++eric, pthread_self(), item, token);
							h_insert(urls_found, token, token);

							place(&urls, token);
							pthread_cond_signal(&nonempty);
						}
						pthread_mutex_unlock(&e_mutex);
					}
				}
				printf("-%s ok\n", item);
				free(name);
			}
			//clear space
			free(item);
			close(sock);
		}
	}
	pthread_exit(NULL);
}

void obtain(url_queue *q, char **url)
{
	//function used by consumer threads, obtains a value from fd queue.
	char *data;

	pthread_mutex_lock(&mutex);
	if(end_flag == 1 && uqueue_isEmpty(q)){
		(*url) = NULL;
		return;
	}
	while(uqueue_isEmpty(q)){
		pthread_cond_wait(&nonempty, &mutex);
		if(end_flag == 1 && uqueue_isEmpty(q)){
			pthread_mutex_unlock(&mutex);
			pthread_cond_broadcast(&nonempty);
			(*url) = NULL;
			return;
		}
	}

	pthread_mutex_lock(&c_mutex);
	uqueue_remove(q, &data);
	pthread_mutex_unlock(&c_mutex);

	pthread_mutex_unlock(&mutex);
	(*url) = malloc(strlen(data)+1);
	strcpy((*url), data);
	free(data);
}

void place(url_queue *q, char *x)
{
	pthread_mutex_lock(&mutex);
	while(q->count >= MAX_SIZE)
		pthread_cond_wait(&nonfull, &mutex);
	uqueue_insert(q, x);
	pthread_mutex_unlock(&mutex);
}

void int_handler(int signum)
{
	//web crawler catches interrupt in order to close filedescriptors.
	printf("\n-Closing sockets...\n");
	//close(sock);
	close(c_sock);
	uqueue_freeQ(&urls);
	h_DelTable(urls_found);
	CloseFIFOs(fds, worker_c);
	exit(-10);
}
