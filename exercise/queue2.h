#ifndef QUEUE2_H
#define QUEUE2_H


//this is Queue implemented with linked list -->use for url queue of webcrawler
//(header of queue2.c)

typedef struct QueueNodeType* q_lnk;

struct QueueNodeType{
	char *item;
	q_lnk next;
};

typedef struct{
	q_lnk front;
	q_lnk rear;
	int count;
}url_queue;

//functions
void uqueue_initialize(url_queue *);
int uqueue_isEmpty(url_queue *);
void uqueue_insert(url_queue *, char *);
void uqueue_remove(url_queue *, char **);			//here we extract the item.
void uqueue_print(url_queue *);
int uqueue_search(url_queue *, char *);
void uqueue_freeQ(url_queue *);

#endif
