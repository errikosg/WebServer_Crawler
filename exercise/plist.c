#include <stdio.h>
#include <stdlib.h>
#include "plist.h"

//initialize
void pL_initialize(postList *l){
	l->head = NULL;
	l->tail = NULL;
	l->count = 0;
}


//is_Empty
int pL_isEmpty(postList *l){
	return (l->head==NULL);
}


//insert
void pL_insert(int di, int f, postList *l)
{
	if(pL_isEmpty(l)){
		l->head = (lnk)malloc(sizeof(struct postListNode));
		l->head->doc_id = di;
		l->head->freq = f;
		l->head->next = NULL;
		l->tail = l->head;
	}
	else{
		l->tail->next = (lnk)malloc(sizeof(struct postListNode));
		l->tail->next->doc_id = di;
		l->tail->next->freq = f;
		l->tail->next->next = NULL;
		l->tail = l->tail->next;
	}
	l->count++;
}

void pL_increase(int doc, postList *l)
{
	lnk temp = l->head;
	while(temp != NULL){
		if(temp->doc_id == doc){
			temp->freq+=1;
			break;
		}
		temp = temp->next;
	}
}


//remove
void pL_remove(int *v1, int* v2, postList *l)
{
	//remove from the front of the list

	int i;
	lnk temp = NULL;

	if(l->head->next == NULL){
		(*v1) = l->head->doc_id;
		(*v2) = l->head->freq;
		free(l->head);
		l->head = NULL;
		l->tail = NULL;
	}
	else{
		temp = l->head->next;
		(*v1) = l->head->doc_id;
		(*v2) = l->head->freq;
		free(l->head);
		l->head = temp;
	}
	l->count--;
}

//searches posting list for document d
int pL_search(int d, postList *l)
{
	lnk curr = l->head;
	while(curr != NULL){
		if(curr->doc_id == d)
			return curr->freq;
		curr = curr->next;
	}
	return -1;
}

int pL_findDoc(int d, postList *l)
{
	lnk curr = l->head;
	while(curr != NULL){
		if(curr->doc_id == d)
			return 1;
		curr = curr->next;
	}
	return 0;
}

int pL_getPos(int pos, postList *l)
{
	if(pos < 0 || pos > l->count-1)
		return -1;
	int i;
	lnk curr = l->head;
	for(i=0; i<pos; i++){
		curr = curr->next;
	}
	return curr->doc_id;
}

int pL_getTotalFreq(postList *l)
{
	if(pL_isEmpty(l)){
		return -1;
	}
	else{
		lnk curr = l->head;
		int total=0;

		while(curr != NULL){
			total += curr->freq;
			curr = curr->next;
		}
		return total;
	}
}

//copy values 2-->1
void pL_copy(postList *pl1, postList **pl2)
{
	if(pL_isEmpty((*pl2))){
		//fprintf(stderr, "pL: List is empty!\n");
		return;
	}
	else{
		int di, f;
		while(!pL_isEmpty((*pl2))){
			pL_remove(&di, &f, (*pl2));
			pL_insert(di, f, pl1);
		}
	}
}

//print the list
void pL_print(postList *l)
{
	if(pL_isEmpty(l)){
		fprintf(stderr, "List is empty!\n");
		return;
	}
	else{
		lnk cur = l->head;
		printf("Posting list:\n");
		while(cur != NULL){
			printf("[%d, %d]  ", cur->doc_id, cur->freq);
			cur = cur->next;
		}
		printf("\n");
	}
}

//free list
void pL_freelist(postList *l)
{
	if(pL_isEmpty(l)){
		//fprintf(stderr, "List is empty!\n");
		return;
	}
	else{
		lnk curr = l->head;
		lnk temp = curr->next;
		while(temp != NULL){
			free(curr);
			curr = temp;
			temp = curr->next;
		}
		free(curr);
	}
	l->head = NULL;
	l->tail = NULL;
	l->count = 0;
}
