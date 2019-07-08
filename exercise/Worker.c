#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include "trie.h"
#include "plist.h"
#include "mlist.h"
#include "functions.h"

//worker reads from fd_cin, writes in fd_cout.
//if USR1, reads info about base
//if USR2, reads commands

int cmd_flag=0;
int read_flag=0;
int end_flag=0;
int continue_flag;

void base_handler(int);
void cmd_handler(int);
void alrm_handler(int);

int main(int argc, char *argv[])
{
	int i, j, k, msg_size, fd_cin, fd_cout, path_count, file_c, pos, code, n;	char *buffer; char c;
	char **pathNames;	int *lines;										//here pathNames are the paths that will take from jobExecutor.
	char **names; char ***Maps;											//names are the pathnames of each file, Maps is a map of all the files.
	Trie T;
	int char_count=0, word_count=0, line_count=0;						//needed for wc command.

	int pid = getpid();
	fd_cin = atoi(argv[1]);
	fd_cout = atoi(argv[2]);
	signal(SIGUSR1, base_handler);
	signal(SIGUSR2, cmd_handler);

	printf("Worker %d: Hello, fds: %d, %d\n", getpid(), fd_cin, fd_cout);

	while(1){
		pause();
		if(read_flag == 1){
			//read paths from pipe - this case for building base only.
			read_flag = 0;

			while((n = read(fd_cin, &path_count, sizeof(int))) <= 0)			//reads the num of paths that will handle.
				;
			pathNames = malloc(path_count*sizeof(char *));
			for(i=0; i<path_count; i++){
				while((n = read(fd_cin, &msg_size, sizeof(int))) <= 0)			//reads the size of the message -- '\0' included
					;
				pathNames[i] = malloc(msg_size*sizeof(char));
				while((n = read(fd_cin, pathNames[i], msg_size)) <= 0)			//reads actual message.
					;
			}
			//printf("\nWorker %d: i got %d paths: ", getpid(), path_count);
			//printMap(pathNames, path_count);

			//manage paths - if generally error occurs, send error code.
			file_c = getFileCount_v2(pathNames, path_count);				//get file count.
			Maps = malloc(file_c*sizeof(char **));
			lines = malloc(file_c*sizeof(int));							//!! get line count of every map opened.
			names = malloc(file_c*sizeof(char *));						//!! get the actual pathname for every file
			//printf("\t%d: found %d files\n", getpid(), file_c);

			DIR *dr;
			struct dirent *entry;
			struct stat statbuf;
			pos=0;		char *temp;
			for(i=0; i<path_count; i++){								//for every path, open every file in it and make Map.
				if((dr = opendir(pathNames[i])) == NULL){
					perror("opendir failed");
					printf("\tfailed to open %s\n", pathNames[i]);
					continue;
				}	

				while((entry = readdir(dr)) != NULL){
					temp = malloc((strlen(entry->d_name) + strlen(pathNames[i]))*sizeof(char)+3);
					strcpy(temp, pathNames[i]);
					strcat(temp, "/");
					strcat(temp, entry->d_name);
					stat(temp, &statbuf);										//checking if file
					if((statbuf.st_mode & S_IFMT) == S_IFREG){
						Maps[pos] = makeMap_v2(temp, &lines[pos]);
						//printf("Worker %d, printing Map %d: \n", getpid(), pos+1);
						//printMap(Maps[pos], lines[pos]);

						names[pos] = malloc(strlen(temp)*sizeof(char)+1);
						strcpy(names[pos], temp);
						pos++;
					}
					free(temp);
				}
				closedir(dr);
			}

			//make trie for all the files together.
			T = makeTrie(&T, Maps, lines, names, file_c, &char_count, &word_count, &line_count);

			//write that all ok.
			code = OK_CODE;
			while((n = write(fd_cout, &code, sizeof(int))) <= 0)
				;
		}
		else if(cmd_flag == 1){
			//reads command to execute.
			cmd_flag = 0;

			n = read(fd_cin, &code, sizeof(int));
			if(code == EXIT_CODE){											//exit
				break;
			}
			else if(code == SEARCH_CODE){									//search
				int arg_c, errno, l, time, mytime;
				char **arg;
				postList *pl;
				continue_flag=1;

				//read arguments of search
				while((n = read(fd_cin, &arg_c, sizeof(int))) <= 0)
					;
				arg = malloc(arg_c*sizeof(char *));
				for(i=0; i<arg_c; i++){
					while((n = read(fd_cin, &msg_size, sizeof(int))) <= 0)
						;
					arg[i] = malloc(msg_size*sizeof(char));
					while((n = read(fd_cin, arg[i], msg_size)) <= 0)
						;
				}

				//main loop, write the right as long as continue flag=1;
				for(i=0; i<file_c; i++){
					if(continue_flag==0)
						break;

					Queue q;
					queue_initialize(&q);
					for(j=0; j<arg_c; j++){
						if((pl = trie_search(&T, arg[j], i, &errno)) == NULL){
							continue;
						}
						else if(pl != NULL){
							for(l=0; l<pl->count; l++){
								k = pL_getPos(l, pl);						//get every entry in posting list -> queue will have the unique doc_ids.
								if(!queue_search(&q, k)){
									queue_insert(&q, k);
									
									code = OK_CODE;
									while((n = write(fd_cout, &code, sizeof(int))) <= 0)		//write code
										;
									msg_size = strlen(names[i])+1;
									while((n = write(fd_cout, &msg_size, sizeof(int))) <= 0)	//write msge size
										;	
									while((n = write(fd_cout, names[i], msg_size)) <= 0)		//write filename
										;
									while((n = write(fd_cout, &k, sizeof(int))) <= 0)			//write doc id
										;
									msg_size = strlen(Maps[i][k])+1;
									while((n = write(fd_cout, &msg_size, sizeof(int))) <= 0)	//write msge size
										;
									while((n = write(fd_cout, Maps[i][k], msg_size)) <= 0)		//write document
										;
								}
							}
						}
					}
				}
				code = END_CODE;
				while((n = write(fd_cout, &code, sizeof(int))) <= 0)		//write code
					;
			}
			
		}
	}


	//clear place
	for(i=0; i<path_count; i++){
		free(pathNames[i]);
	}
	free(pathNames);
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
	trie_delete(&T);
	//printf("Worker %d BYE BYE\n", getpid());

	exit(0);
}

void base_handler(int signum)
{
	read_flag = 1;
}

void cmd_handler(int signum)
{
	cmd_flag = 1;
}
