#ifndef QUEUE1_H
#define QUEUE1_H

//this is Queue implemented with linked list -->use to keep fds of sockets in server temporarily.
//(header of queue1.c)

typedef int itemType;

typedef struct QueueNode* linkk;

struct QueueNode{
	itemType item;
	linkk next;
};

typedef struct{
	linkk front;
	linkk rear;
	int count;
}Queue;

//functions
void queue_initialize(Queue *);
int queue_isEmpty(Queue *);
void queue_insert(Queue *, itemType);
void queue_remove(Queue *, itemType *);			//here we extract the item.
void queue_print(Queue *);
int queue_search(Queue *, itemType);
void queue_freeQ(Queue *);

#endif
