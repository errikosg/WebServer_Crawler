#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mlist.h"

void mL_initialize(mainList *ml)
{
	ml->count=0;
	ml->head=NULL;
	ml->tail=NULL;
}

int mL_isEmpty(mainList *ml)
{
	return (ml->head==NULL);
}

void mL_insert(char *path, int id, mainList *ml, int doc)
{
	if(mL_isEmpty(ml)){
		ml->head = malloc(sizeof(struct mainListNode));
		ml->head->mapID = id;
		ml->head->pathName = malloc(strlen(path)*sizeof(char) + 1);
		strcpy(ml->head->pathName, path);
		ml->head->next = NULL;
		ml->tail = ml->head;
		
		pL_initialize(&ml->head->pl);
		pL_insert(doc, 1, &ml->head->pl);
	}
	else{
		ml->tail->next = malloc(sizeof(struct mainListNode));
		ml->tail->next->mapID = id;
		ml->tail->next->pathName = malloc(strlen(path)*sizeof(char)+1);
		strcpy(ml->tail->next->pathName, path);
		ml->tail->next->next = NULL;

		pL_initialize(&ml->tail->next->pl);
		pL_insert(doc, 1, &ml->tail->next->pl);
		ml->tail = ml->tail->next;
	}
	ml->count++;
}

postList *mL_search(int id, mainList *ml)
{
	lk temp = ml->head;
	while(temp != NULL){
		if(temp->mapID == id){
			return &temp->pl;
		}
		temp = temp->next;
	}
	return NULL;
}

int mL_getEntries(int id, mainList *ml)
{
	lk temp = ml->head;
	while(temp != NULL){
		if(temp->mapID == id){
			return temp->pl.count;
		}
		temp = temp->next;
	}
	return -1;
}

void mL_print(mainList *ml)
{
	if(mL_isEmpty(ml)){
		printf("mainList is empty!\n");
		return;
	}
	
	lk temp = ml->head;
	printf("*--------------------------------------------------------------*\n");
	while(temp != NULL){
		printf("File %d: path = %s.\n*) ", temp->mapID, temp->pathName);
		pL_print(&temp->pl);
		printf("\n");

		temp = temp->next;
	}
	printf("*--------------------------------------------------------------*\n");
}

void mL_freelist(mainList *ml)
{
	if(mL_isEmpty(ml)){
		//printf("Main list is empty\n");
		return;
	}

	lk curr = ml->head;
	lk temp = curr->next;
	while(temp != NULL){
		free(curr->pathName);
		pL_freelist(&curr->pl);
		free(curr);

		curr = temp;
		temp = curr->next;
	}
	free(curr);
	ml->head = NULL;
	ml->tail = NULL;
	ml->count = 0;
}
