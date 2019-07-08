#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include "functions.h"

#define PERMS 0644

//generally used functions.

void perror_exit(char *msg, int rv)
{
	perror(msg);
	exit(rv);
}

int accept_any(int s_sock, struct sockaddr **s_clientptr, socklen_t s_clientlen, int c_sock, struct sockaddr **c_clientptr, socklen_t c_clientlen, int *acc)
{
	//select() to see which is ready - s_sock or c_sock;
	fd_set readfds;
	int maxfd, status, i;

	FD_ZERO(&readfds);
	FD_SET(s_sock, &readfds);
	FD_SET(c_sock, &readfds);
	if(s_sock > c_sock)
		maxfd = s_sock;
	else
		maxfd = c_sock;

	status = select(maxfd+1, &readfds, NULL, NULL, NULL);

	if(status < 0){
		perror("select");
		return -1;
	}
	int fd = -1, x;
	if(FD_ISSET(s_sock, &readfds))
		fd = s_sock;
	if(FD_ISSET(c_sock, &readfds))
		fd = c_sock;

	if(fd == -1)
		return -1;
	else{
		*acc = fd;
		if(fd == s_sock){
			x = accept(s_sock, *s_clientptr, &s_clientlen);
			if(x == -1)
				perror("accept");
			return x;
		}
		else{
			x = accept(c_sock, *c_clientptr, &c_clientlen);
			if(x == -1)
				perror("accept");
			return x;
		}
	}
}

int getFileCount(char *root_dir)
{
	int i, count=0;
	DIR *dr; DIR *basic;
	struct dirent *entry; struct dirent *basic_entry;
	struct stat statbuf;
	char *buffer; char *temp;
	
	//open directory
	if((basic = opendir(root_dir)) == NULL){
		perror("opendir failed");
		return -1;
	}
	while((basic_entry = readdir(basic)) != NULL){
		if(strcmp(basic_entry->d_name, ".") == 0 || strcmp(basic_entry->d_name, "..") == 0)		//avoid ., ..
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
					count++;
				}
				free(temp);
			}
			closedir(dr);
		}
		free(buffer);
	}
	closedir(basic);

	return count;
}

char **makeMap(char *docfile, int *dn)
{
	FILE *fp;
	int i, counter, space_f, cc;
	unsigned char c;
	int doc_num=0;
	char **map;

	if((fp = fopen(docfile, "r")) == NULL){
		perror("makeMap: fopen");
		return NULL;
	}
	while(cc = fgetc(fp)){												//make doc_num AND check for empty lines.
		if(cc == EOF)
			break;
		c = (char)cc;
		if(c == '\n')
			doc_num++;
	}
	map = (char **)malloc(doc_num*sizeof(char *));
	rewind(fp);

	for(i=0; i<doc_num; i++){
		counter = 0;
		space_f=0;
		while((c = (char)fgetc(fp)) != '\n'){
			counter++;
		}
		if(counter==0){
			map[i] = (char *)malloc(3*sizeof(char));
			strcpy(map[i], "\n");
			continue;
		}
		counter++;
		fseek(fp, -counter, SEEK_CUR);
		map[i] = (char *)malloc(counter*sizeof(char)+1);
		fgets(map[i], counter, fp);
		map[i][counter-1] = '\0';
		fseek(fp, 1, SEEK_CUR);
	}	
	fclose(fp);
	*dn = doc_num;
	return map;
}


void printMap(char **map, int doc_num)
{
	int i;
	for(i=0; i<doc_num; i++){
		printf("%s\n", map[i]);
	}
	printf("\n");
}


char **read_header(int sock, int tm, int *h_lines, int *error)
{
	//reads header of HTTP/1.1 request coming through socket.
	//if error occurs, sets errno to correct value.

	char **message;	char c;
	int i, j, pos_x, pos_y, flag, ready, n;
	struct timeval timeout;
	
	message = malloc(10*sizeof(char *));			//this is a buffer, initialized in specific size.
	for(i=0; i<10; i++){
		message[i] = malloc(BUFF_SIZE*sizeof(char));
	}

	timeout.tv_sec = tm;    	// tm seconds
    timeout.tv_usec = 0;    	// 0 milliseconds
	fd_set input_set;
	FD_ZERO(&input_set );
    FD_SET(sock, &input_set);
	
	flag=0;
	pos_x=0;	pos_y=0;
	while(1){
		ready = select(sock+1, &input_set, NULL, NULL, &timeout);
		if(ready < 0){
			//clear space and return NULL.
			for(i=0; i<10; i++){
				free(message[i]);
			}
			free(message);
			*error = -1;
			return NULL;
		}
		
		if(ready){
			if((n = read(sock, &c, sizeof(char))) <= 0){
				perror("read");
				for(i=0; i<10; i++){
					free(message[i]);
				}
				free(message);
				*error = -2;
				return NULL;
			}
			if(c != '\n' && c !='\r'){
				message[pos_x][pos_y] = c;
				pos_y++;
				if(flag == 1)
					flag = 0;
			}
			else if(c == '\n' && flag==0){
				//normal line
				message[pos_x][pos_y] = '\0';
				pos_x++;	pos_y=0;
				flag = 1;
			}
			else if(c == '\n' && flag==1){
				//end of header
				*h_lines = pos_x;
				*error = 0;
				return message;
			}
		}
		else{
			*error = -3;
			for(i=0; i<10; i++){
				free(message[i]);
			}
			free(message);
			return NULL;
		}
	}
}

int read_cmd(int sock, int tm, char **tmp)
{
	int i, ready, pos=0;
	char c;
	char *cmd = malloc(BUFF_SIZE*sizeof(char));
	struct timeval timeout;

	timeout.tv_sec = tm;    	// tm seconds
    timeout.tv_usec = 0;    	// 0 milliseconds
	fd_set input_set;
	FD_ZERO(&input_set );
    FD_SET(sock, &input_set);

	while(1){
		ready = select(sock+1, &input_set, NULL, NULL, &timeout);
		if(ready < 0)
			return -1;
		
		if(ready){
			if(read(sock, &c, sizeof(char)) <= 0)
				return -1;
			if(c == '\n'){
				cmd[pos] = '\0';
				strcpy(*tmp, cmd);
				free(cmd);
				return 0;
			}
			cmd[pos] = c;
			pos++;
		}
		else
			return -1;
	}
}

int msg_check(char **msg, int lines, char **url)
{
	//functions that checks if syntax of request given is ok.
	//returns -1 if not ok, which translates to server sending 400 Bad Request.

	if(lines < 2){
		//at least GET and Host must be given, error.
		//printf("error0\n");
		return -1;
	}

	int i, flag_g = 0, flag_h = 0, free_flag = 0, start_flag;
	char *temp; char *token; char *index;
	for(i=0; i<lines; i++){
		temp = strdup(msg[i]);

		token = strtok(temp, " ");		start_flag = 1;

		if(strcmp(token, "GET") != 0){								//checking that all start with [..]: ,except GET line
			if((index = strchr(token, ':')) == NULL){
			//printf("error1\n");
				return -1;
			}
		}
		while(token != NULL){
			if(strcmp(token, "Host:") == 0){
				if(start_flag != 1){
					//printf("error2\n");
					return -1;
				}
				token = strtok(NULL, " ");
				if(token != NULL){
					flag_h = 1;
				}
			}
			else if(strcmp(token, "GET")==0){
				if(start_flag != 1){
					//printf("error3\n");
					return -1;
				}
				token = strtok(NULL, " ");
				if(token != NULL){
					(*url) = strdup(token);
					free_flag = 1;												//i kept url, free if error next.
					token = strtok(NULL, " ");
					if(token != NULL && (strcmp(token, "HTTP/1.1\r")==0 || strcmp(token, "HTTP/1.1")==0)){
						flag_g = 1;
					}
				}
			}
			token = strtok(NULL, " ");
			start_flag = 0;
		}
		free(temp);
	}

	if(flag_h == 0 || flag_g == 0){
		//either GET or Host were not defined.
		if(free_flag == 1)
			free((*url));
		//printf("error4, flag_h=%d, flag_g=%d\n", flag_h, flag_g);
		return -1;
	}
	else
		return 0;
}

int write_answer(int sock, int code, char **file, int lines, long int *pc)
{
	//for server
	//given an fd and a code, it writes the corresponding response to client-crawler

	int i, j, bytes;
	char **header;
	char *html_msg;
	header = malloc(5*sizeof(char *));

	//make date.
	time_t rawtime;
	time(&rawtime );
	struct tm *info = localtime(&rawtime);
	char *date = malloc(50*sizeof(char));
	char *ct = malloc(50*sizeof(char));
	strftime(ct, 50, "%c", info);
	sprintf(date, "Date: %s\r\n", ct);
	free(ct);

	switch(code){
		case 1:
			//HTTP/1.1 200 OK
			if(file == NULL)
				return -1;
			printf("-Returning 200 OK\n");
			header[0] = malloc(22*sizeof(char));
			strcpy(header[0], "HTTP/1.1 200 OK\r\n");
			bytes = get_bytes(file, lines);
			html_msg = NULL;
			break;
		case 2:
			//HTTP/1.1 404 Not Found
			printf("-Returning 404 Not Found\n");
			header[0] = malloc(30*sizeof(char));
			strcpy(header[0], "HTTP/1.1 404 Not Found\r\n");
			html_msg = malloc(54*sizeof(char));
			strcpy(html_msg, "<html>Sorry dude, could not find this file.</html>");
			bytes = strlen(html_msg);
			break;
		case 3:
			//HTTP/1.1 403 Forbidden
			printf("-Returning 403 Forbidden\n");
			header[0] = malloc(30*sizeof(char));
			strcpy(header[0], "HTTP/1.1 403 Forbidden\r\n");
			html_msg = malloc(75*sizeof(char));
			strcpy(html_msg, "<html>Trying to access this file but don’t think I can make it.</html>");
			bytes = strlen(html_msg);
			break;
		case 4:
			//HTTP/1.1 400 Bad Request.
			printf("-Returning 400 Bad Request.\n");
			header[0] = malloc(33*sizeof(char));
			strcpy(header[0], "HTTP/1.1 400 Bad Request\r\n");
			html_msg = malloc(60*sizeof(char));
			strcpy(html_msg, "<html>Request given had not the right syntax.</html>");
			bytes = strlen(html_msg);
			break;
	}
	//make rest of header.
	header[1] = strdup("Server: myhttpd/1.0.0 (Ubuntu64)\r\n");
	header[2] = malloc(35*sizeof(char));
	sprintf(header[2], "Content-Length: %d\r\n", bytes);
	header[3] = strdup("Content-Type: text/html\r\n");
	header[4] = strdup("Connection: Closed\r\n\n");

	//write header.
	for(j=0; j<5; j++){
		for(i=0; header[j][i] != '\0'; i++){
			if(write(sock, &header[j][i], 1) < 0)
				return -1;
		}
		if(j==0){
			for(i=0; date[i] != '\0'; i++){
				if(write(sock, &date[i], 1) < 0)
					return -1;
			}
		}
	}
	if(html_msg != NULL){
		//write html message informing client about the corresponding error - page and byte count unchanged
		for(i=0; html_msg[i] != '\0'; i++){
			if(write(sock, &html_msg[i], 1) < 0)
				return -1;
		}
	}
	else{
		//write actual file wanted
		for(i=0; i<lines; i++){
			for(j=0; file[i][j]!='\0'; j++){
				if(write(sock, &file[i][j], 1) < 0)
					return -1;
			}
			char c='\n';
			write(sock, &c, 1);
		}
		(*pc)++;
	}

	for(j=0; j<5; j++){
		free(header[j]);
	}
	free(header);
	free(date);
	if(html_msg != NULL)
		free(html_msg);
	return 0;
}


int file_exist(char *filename)
{
  struct stat buffer;   
  return (stat(filename, &buffer) == 0);
}

int get_perms(char *filename)
{
	struct stat buffer;
	stat(filename, &buffer);

	if(!(buffer.st_mode & S_IRUSR) || !(buffer.st_mode & S_IWUSR))
		return -1;
	else
		return 0;
}

int get_bytes(char **map, int lines)
{
	int i, count = 0;
	for(i=0; i<lines; i++){
		count += strlen(map[i]);
	}
	return count;
}

void c_handler(int signum)
{
	//handles SIGUSR1, threads is told to exit.
	printf("-thread %ld issued to close\n", pthread_self());
	pthread_exit(NULL);
}


int write_request(int sock, char *url)
{
	//function used by crawler to write header with given url
	int i, j;

	//write request.
	char **header;
	header = malloc(6*sizeof(char *));
	//header[0] = malloc((15+strlen(url))*sizeof(char));
	header[0] = malloc(BUFF_SIZE*sizeof(char));
	strcpy(header[0], "GET ");
	strcat(header[0], url);
	strcat(header[0], " HTTP/1.1\r\n");

	header[1] = strdup("User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\n");
	header[2] = strdup("Host: www.tutorialspoint.com\r\n");
	header[3] = strdup("Accept-Language: en-us\r\n");
	header[4] = strdup("Accept-Encoding: gzip, deflate\r\n");
	header[5] = strdup("Connection: Keep-Alive\r\n\n");
	
	printf("#%ld: Requesting url: %s\n", pthread_self(), url);

	for(j=0; j<6; j++){
		for(i=0; header[j][i] != '\0'; i++){
			if(write(sock , &header[j][i], 1) < 0)
				return -1;
		}
	}

	for(j=0; j<6; j++){
		free(header[j]);
	}
	free(header);
	return 0;
}

int read_and_manage(int sock, pthread_mutex_t t_mutex, char *url, char *dir, int *lines, long int *pc)
{
	int line_c, r_errno, c_errno, i;
	char **msg;
	//get answer
	if((msg = read_header(sock, 30, &line_c, &r_errno)) != NULL){
		//printf("Lines %d, printing message:\n\n", line_c);
		//printMap(msg, line_c);
	}
	else{
		printf("# Header reading encountered problem:\n");
		switch(r_errno){
			case -1:
			printf("\tError in given input.\n");	break;
			case -2:
				printf("\tError occured in reading from socket\n");	break;
			case -3:
				printf("\tTimeout occured\n");	break;
		}
	}
	//analyze
	char *token; char buf[1];
	token = strtok(msg[0], " ");
	token = strtok(NULL, " ");
	if(strcmp(token, "200")==0){
		//it was an OK code.
		int status, num;
		char *temp;	int fd;;

		temp = strdup(url);
		token = strtok(temp, "/");
		char *name = malloc(BUFF_SIZE*sizeof(char));

		//make directory and files, (make test.txt)
		pthread_mutex_lock(&t_mutex);
		sprintf(name, "%s/%s", dir, token);
		if((status = mkdir(name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) < 0)
			;//directory exists

		sprintf(name, "%s%s", dir, url);
		if((fd = open(name, O_CREAT|O_RDWR ,PERMS))< 0)
			perror("open");
		while(read(sock, buf, 1) > 0){
			if(write(fd, &buf[0], 1) < 0)
				continue;
			if(buf[0] == '\n')
				(*lines)++;
		}
		(*pc)++;
		close(fd);
		pthread_mutex_unlock(&t_mutex);
		free(name); free(temp);
		
		//clear
		if(r_errno > -1){
			for(i=0; i<10; i++)
				free(msg[i]);
			free(msg);
		}
		return 0;
	}
	else{
		printf("# Got negative response,\n");
		while(read(sock, buf, 1) > 0){
			;
		}
		//clear
		if(r_errno > -1){
			for(i=0; i<10; i++)
				free(msg[i]);
			free(msg);
		}
		return -1;
	}
}


//*-----------------------------------------------------------------------------*
//code for communication and creation of Workers
int MakeWorker(int i, FifoFd *fds)
{
	//function that makes the two FIFOs for worker, make the worker and opens them, then makes the child exec Worker.c and returns the pid of the worker made.
	char *fifo_in = malloc(20*sizeof(char));
	char *fifo_out = malloc(20*sizeof(char));

	sprintf(fifo_in, "fifoin%d", i);				//make fifos with coresponding names, will only then keep the file descriptors.
	if(mkfifo(fifo_in, 0666) == -1){
		if(errno != EEXIST){
			perror("mkfifo fifo_in");
			return -1;
		}
	}
	sprintf(fifo_out, "fifoout%d", i);
	if(mkfifo(fifo_out, 0666) == -1){
		if(errno != EEXIST){
			perror("mkfifo fifo_out");
			return -1;
		}
	}	

	//made fifos, now open and fork.
	if((fds->fd_cin = open(fifo_in, O_RDONLY | O_NONBLOCK)) < 0){
		perror("open fd_cin");
		return -1;
	}
	if((fds->fd_pin = open(fifo_in, O_WRONLY | O_NONBLOCK)) < 0){
		perror("open fd_pin");
		return -1;
	}

	if((fds->fd_pout = open(fifo_out, O_RDONLY | O_NONBLOCK)) < 0){
		perror("open fd_pout");
		return -1;
	}
	if((fds->fd_cout = open(fifo_out, O_WRONLY | O_NONBLOCK)) < 0){
		perror("open fd_cout");
		return -1;
	}

	pid_t pid = fork();
	if(pid < 0){
		perror("fork");
		return -1;
	}
	else if(pid==0){
		char *cmd = "./Worker";
		char *args[4];
		args[0] = "./Worker";
		args[1] = malloc(2*sizeof(char));
		sprintf(args[1], "%d", fds->fd_cin);
		args[2] = malloc(2*sizeof(char));
		sprintf(args[2], "%d", fds->fd_cout);
		args[3] = NULL;

		execvp(cmd, args);
		perror("execvp");
		fprintf(stderr, "Error in making worker: execvp\n");
		return -1;
	}
	
	free(fifo_in);
	free(fifo_out);
	return pid;
}

void CloseFIFOs(FifoFd *fds, int work_c)
{
	int i;
	for(i=0; i<work_c; i++){
		close(fds[i].fd_pin);
		close(fds[i].fd_pout);
		close(fds[i].fd_cin);
		close(fds[i].fd_cout);
		char *buffer = malloc(20*sizeof(char));
		sprintf(buffer, "fifoin%d", i);
		unlink(buffer);
		sprintf(buffer, "fifoout%d", i);
		unlink(buffer);
		free(buffer);
	}
}

char **makeMap_v2(char *docfile, int *dn)
{
	FILE *fp;
	int i, counter, space_f, j, cc, flag=0;
	unsigned char c;
	int doc_num=0;
	char **map;

	if((fp = fopen(docfile, "r")) == NULL){
		perror("makeMap: fopen");
		return NULL;
	}
	while(cc = fgetc(fp)){												//make doc_num AND check for empty lines.
		if(cc == EOF)
			break;
		c = (char)cc;
		if(c == '\n'){
			if(flag != 1)
				doc_num++;
			flag=1;
		}
		else
			flag=0;
	}
	map = (char **)malloc(doc_num*sizeof(char *));
	rewind(fp);

	for(i=0; i<doc_num; i++){
		counter = 0;
		space_f=0;
		while((c = (char)fgetc(fp)) != '\n'){
			counter++;
		}
		if(counter==0){
			fprintf(stderr, "Empty line. Error\n");
			fclose(fp);
			return NULL;
		}
		counter++;
		fseek(fp, -counter, SEEK_CUR);
		map[i] = (char *)malloc(counter*sizeof(char)+1);
		fgets(map[i], counter, fp);						//?
		map[i][counter-1] = '\0';
		//c = (char)fgetc(fp);
		fseek(fp, 1, SEEK_CUR);
	}	
	fclose(fp);
	*dn = doc_num;
	return map;
}

Trie makeTrie(Trie *T, char ***Maps, int *lines, char **names, int file_c, int *cc, int *wc, int *lc)	//the maps, the lines of each map, the number of maps
{
	//the three int* optional - only for this exercise.
	postList *pl;
	char **temp;	int i, j, index=0, errno, x;
	char *token;

	trie_init(T);
	for(i=0; i<file_c; i++){
		//make temp array.
		temp = malloc(lines[i]*sizeof(char *));
		for(j=0; j<lines[i]; j++){
			temp[j] = malloc(strlen(Maps[i][j])*sizeof(char)+1);
			strcpy(temp[j], Maps[i][j]);
		}

		(*lc) += lines[i];											//fix line count
		for(j=0; j<lines[i]; j++){
			token = strtok(temp[j], " \t");
			while(token != NULL){
				(*wc)++;											//fix word count
				(*cc) += strlen(token);								//fix char count
				if((pl = trie_search(T, token, i, &errno)) == NULL){
					//word not found entirely or only for this file
					if(errno == -1){
						if((x = trie_insertV1(T, token, names[i], i, j)) < 0)
							printf("-makeTrie: trie_insertV1 failed to insert %s from file %s\n", token, names[i]);
					}
					else if(errno == -2){
						if((x = trie_insertV2(T, token, names[i], i, j)) < 0)
							printf("-makeTrie: trie_insertV2 failed to insert %s from file %s\n", token, names[i]);
					}
					else
						printf("makeTrie: There was problem in trie_search\n");
				}
				else if(pl != NULL){
					if(pL_findDoc(j, pl))
						pL_increase(j, pl);
					else
						pL_insert(j, 1, pl);
				}
				token = strtok(NULL, " \t");
			}
		}
		//free the temp array
		for(j=0; j<lines[i]; j++){
			free(temp[j]);
		}
		free(temp);
	}

	return *T;
}

int getFileCount_v2(char **paths, int path_count)
{
	int i, count=0;
	DIR *dr;
	struct dirent *entry;
	struct stat statbuf;
	char *buffer;

	for(i=0; i<path_count; i++){
		if((dr = opendir(paths[i])) == NULL){
			perror("opendir failed");
			continue;
		}
		while((entry = readdir(dr)) != NULL){
			buffer = malloc((strlen(paths[i])+strlen(entry->d_name))*sizeof(char)+2);
			strcpy(buffer, paths[i]);
			strcat(buffer, "/");
			strcat(buffer, entry->d_name);
			stat(buffer, &statbuf);
			if((statbuf.st_mode & S_IFMT) == S_IFREG){
				count++;
			}
	
			free(buffer);
		}
		closedir(dr);
	}
	return count;
}


