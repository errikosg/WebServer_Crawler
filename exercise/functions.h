#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "queue1.h"
#include "trie.h"

#define BUFF_SIZE 516

#define END_CODE -3
#define NOT_FOUND_CODE -2
#define ERROR_CODE -1
#define OK_CODE 0
#define SEARCH_CODE 1
#define EXIT_CODE 5					//define codes -- will be used for Job Exec. and Workers communication.

typedef struct FifoFd{
	int fd_pin;
	int fd_pout;
	int fd_cin;
	int fd_cout;
}FifoFd;							//FIFO filedescriptors


//functions.
void perror_exit(char *, int rv);
void int_handler(int);
int accept_any(int, struct sockaddr **, socklen_t, int, struct sockaddr **, socklen_t, int *);
int getFileCount(char *);
char **makeMap(char *, int *);
void printMap(char **, int);
char **read_header(int, int, int *, int *);
int read_cmd(int, int, char **);
int msg_check(char **, int, char **);
int get_perms(char *);
int get_bytes(char **, int);
int file_exist(char *);
int write_answer(int, int, char **, int, long int *);
void c_handler(int);
void al_handler(int);
int write_request(int, char *);
int read_and_manage(int, pthread_mutex_t, char *, char *, int *, long int *);

//*-----------------------------------------------------------------------------------------*
int MakeWorker(int, FifoFd *);
void CloseFIFOs(FifoFd *, int);
char **makeMap_v2(char *, int *);
Trie makeTrie(Trie *T, char ***, int *, char **, int, int *, int *, int *);
int getFileCount_v2(char **, int);

#endif
