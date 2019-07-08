#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "queue2.h"

void uqueue_initialize(url_queue *Q)
{
	Q->front = NULL;
	Q->rear = NULL;
	Q->count = 0;
}

int uqueue_isEmpty(url_queue *Q)
{
	return (Q->front==NULL);
}

void uqueue_insert(url_queue *Q, char *w)
{
	q_lnk temp;
	temp = (q_lnk)malloc(sizeof(struct QueueNodeType));
	temp->item = malloc(strlen(w)*sizeof(char)+1);
	strcpy(temp->item, w);
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

void uqueue_remove(url_queue *Q, char **w)
{
	q_lnk temp;
	if(Q->front==NULL){
		//printf("--Attempting to remove from empty queue\n");
		return;
	}
	else{
		(*w) = malloc(strlen(Q->front->item)+1);
		strcpy((*w), Q->front->item);
		free(Q->front->item);
		temp = Q->front;
		Q->front = temp->next;
		free(temp);
		if(Q->front==NULL){
			Q->rear = NULL;
		}
	}
	Q->count--;
}

void uqueue_print(url_queue *Q)
{
	if(uqueue_isEmpty(Q)){
		printf("print: Queue is empty!\n");
		return;
	}
	else{
		q_lnk curr = Q->front;
		printf("\nQueue:\n ");
		while(curr != NULL){
			printf("(%s) ", curr->item);
			curr = curr->next;
		}
		printf("\n");
	}
}

int uqueue_search(url_queue *Q, char *w)
{
	//return 1 if item found, 0 if not.
	if(uqueue_isEmpty(Q)){
		//printf("search: Queue is empty!\n");
		return 0;
	}
	else{
		q_lnk curr = Q->front;
		while(curr != NULL){
			if(strcmp(curr->item,w)==0){
				return 1;
			}
			curr = curr->next;
		}
	}
	return 0;
}

void uqueue_freeQ(url_queue *Q)
{
	char *temp;
	temp = malloc(516*sizeof(char));
	while(!uqueue_isEmpty(Q)){
		uqueue_remove(Q, &temp);
	}
	free(temp);
}

