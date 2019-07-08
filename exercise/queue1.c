#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "queue1.h"

void queue_initialize(Queue *Q)
{
	Q->front = NULL;
	Q->rear = NULL;
	Q->count = 0;
}

int queue_isEmpty(Queue *Q)
{
	return (Q->front==NULL);
}

void queue_insert(Queue *Q, itemType w)
{
	linkk temp;
	temp = (linkk)malloc(sizeof(struct QueueNode));
	temp->item = w;
	temp->next = NULL;
	if(Q->rear==NULL){
		Q->front = temp;
		Q->rear = temp;
	}
	else{
		Q->rear->next=temp;
		Q->rear = temp;
	}
	Q->count++;
}

void queue_remove(Queue *Q, itemType *w)
{
	linkk temp;
	if(Q->front==NULL){
		//printf("--Attempting to remove from empty queue\n");
		return;
	}
	else{
		*w = Q->front->item;
		temp = Q->front;
		Q->front = temp->next;
		free(temp);
		if(Q->front==NULL){
			Q->rear = NULL;
		}
	}
	Q->count--;
}

void queue_print(Queue *Q)
{
	if(queue_isEmpty(Q)){
		//printf("print: Queue is empty!\n");
		return;
	}
	else{
		linkk curr = Q->front;
		printf("\nQueue:\n ");
		while(curr != NULL){
			printf("(%d) ", curr->item);
			curr = curr->next;
		}
		printf("\n");
	}
}

int queue_search(Queue *Q, itemType w)
{
	//return 1 if item found, 0 if not.
	if(queue_isEmpty(Q)){
		//printf("search: Queue is empty!\n");
		return 0;
	}
	else{
		linkk curr = Q->front;
		while(curr != NULL){
			if(curr->item == w){
				return 1;
			}
			curr = curr->next;
		}
	}
	return 0;
}

void queue_freeQ(Queue *Q)
{
	int temp;
	while(!queue_isEmpty(Q)){
		queue_remove(Q, &temp);
	}
}

