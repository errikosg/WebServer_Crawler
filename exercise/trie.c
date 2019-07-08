#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"

edge NEW(int enf, char l);

//list implementation.
void chL_init(chList *cl)
{
	cl->head = NULL;
	cl->count = 0;
}

int chL_isEmpty(chList *cl)
{
	return(cl->head==NULL);
}

edge chL_insert(chList *cl, edge n)
{
	cl->count++;
	chListNode* temp;
	if(!chL_isEmpty(cl)){
		temp = cl->head;
		while(temp->next != NULL){
			temp = temp->next;
		}
		temp->next = (chListNode *)malloc(sizeof(chListNode));
		temp->next->e = NEW(n->isEnd_node, n->letter);
		temp->next->next = NULL;
		return temp->next->e;
	}
	else{
		temp = (chListNode *)malloc(sizeof(chListNode));
		temp->e = NEW(n->isEnd_node, n->letter);
		temp->next = NULL;
		cl->head = temp;
		return temp->e;
	}
}

edge chL_search(chList *cl, char l)
{
	if(chL_isEmpty(cl)){
		//fprintf(stderr, "List is empty!\n");
		return NULL;
	}
	else{
		chListNode *curr = cl->head;
		while(curr != NULL){
			if(curr->e->letter == l){
				return curr->e;
			}
			curr = curr->next;
		}
	}
	return NULL;
}

edge chL_returnpos(chList *cl, int pos)
{
	int i;
	if(pos<1 || pos>cl->count){
		return NULL;
	}
	else{
		chListNode *curr = cl->head;
		for(i=0; i<pos-1; i++){
			if(curr->next==NULL)
				break;
			curr = curr->next;
		}
		return curr->e;
	}
}

void chL_print(chList *cl)
{
	if(chL_isEmpty(cl)){
		//fprintf(stderr, "List is empty!\n");
	}
	else{
		chListNode *curr = cl->head;
		while(curr != NULL){
			printf("%c--->", curr->e->letter);
			curr = curr->next;
		}
	}
	printf("\n");
}

void chL_freelist(chList *cl)
{
	if(chL_isEmpty(cl)){
		//fprintf(stderr, "List is empty!\n");
		return;
	}
	else{
		chListNode *curr = cl->head;
		chListNode *temp = curr->next;
		while(temp != NULL){
			free(curr);
			curr = temp;
			temp = curr->next;
		}
		free(curr);
	}
	cl->head = NULL;
	cl->count=0;
}

//-----------------------------------------------------------------------------
//Trie implementation

edge NEW(int enf, char l)							//for making a new trie node.
{
	edge e = malloc(sizeof*e);
	e->isEnd_node = enf;
	e->letter = l;
	//e->isRoot = isr;

	chL_init(&e->cl);
	if(e->isEnd_node==1)
		mL_initialize(&e->ml);
	return e;
}

void trie_init(Trie *T)
{
	T->root = NULL;
}

int trie_isEmpty(Trie *T)
{
	return(T->root==NULL);
}


int trie_insertV1(Trie *T, char *w, char *path, int mapid, int doc)			//version 1 = classic
{
	//will be called if we know that word doesnt exist at all in Trie.

	if(strlen(w)==0 || w==NULL){
		//fprintf(stderr, "Error: string given is empty\n");
		return -1;
	}

	int i, index, flag=0;
	edge curr, temp;
	if(trie_isEmpty(T)){
		//is root
		T->root = NEW(0, '-');						//letter will be ingored
		curr = T->root;
		for(i=0; i<strlen(w)-1; i++){
			temp = NEW(0, w[i]);
			curr = chL_insert(&curr->cl, temp);
			free(temp);
		}
		temp = NEW(1, w[i]);
		curr = chL_insert(&curr->cl, temp);

		mL_insert(path, mapid, &curr->ml, doc);			//insert in mainlist info about the new file.
		free(temp);
	}
	else{
		curr = T->root;
		index = 0;
		while((temp = chL_search(&curr->cl, w[index])) != NULL){
			curr = temp;
			index++;
			if(index==strlen(w)){
				index--;
				break;
			}
		}
		if(index == (strlen(w)-1) && temp!=NULL){				//word ended.
			curr->isEnd_node = 1;
			mL_initialize(&curr->ml);
			mL_insert(path, mapid, &curr->ml, doc);
		}
		else{
			temp = NULL;
			for(i=index; i<strlen(w)-1; i++){
				temp = NEW(0, w[i]);
				curr = chL_insert(&curr->cl, temp);
				free(temp);
			}
			temp = NEW(1, w[i]);
			curr = chL_insert(&curr->cl, temp);

			mL_insert(path, mapid, &curr->ml, doc);
			free(temp);
		}
	}
	return 0;
}

postList *trie_search(Trie *T, char *w, int mapid, int *errno)
{
	//searches for word in trie, given a file in mapid.
	//if returns NULL it can: 1) set errno to -1 if word not found completely 2)set errno to -2 if word found
	//but not for the given file 3)return posting list if word found in trie for the specific file 4)set errno to -3 in case of error.

	if(strlen(w)==0 || w==NULL){
		//fprintf(stderr, "Error: string given is empty\n");
		*errno = -3;
		return NULL;
	}

	if(trie_isEmpty(T)){
		*errno = -1;
		return NULL;
	}
	int i, index=0;
	edge curr = T->root;

	for(i=0; i<strlen(w); i++){
		if((curr = chL_search(&curr->cl, w[i])) == NULL){
			//word not found.
			*errno = -1;
			return NULL;
		}
	}
	if(curr != NULL && curr->isEnd_node==1){
		//word found in trie.
		postList *pl;
		pl = mL_search(mapid, &curr->ml);
		if(pl == NULL){
			//word not in this file
			*errno = -2;
			return NULL;
		}
		else
			return pl;
	}
	else{
		//word not found
		*errno = -1;
		return NULL;
	}
}

mainList *trie_search_entries(Trie *T, char *w)
{
	if(strlen(w)==0 || w==NULL){
		//fprintf(stderr, "Error: string given is empty\n");
		return NULL;
	}

	if(trie_isEmpty(T)){
		return NULL;
	}
	int i, index=0;
	edge curr = T->root;

	for(i=0; i<strlen(w); i++){
		if((curr = chL_search(&curr->cl, w[i])) == NULL){
			//word not found.
			return NULL;
		}
	}
	if(curr != NULL && curr->isEnd_node==1){
		//word found in trie.
		return &curr->ml;
	}
	else{
		//word not found
		return NULL;
	}
}

int trie_insertV2(Trie *T, char *w, char *path, int mapid, int doc)			//version 1 = new
{	
	//will be called only in case we know word exist but not for given file.
	//returns -1 in case of error, 0 if all ok.

	if(strlen(w)==0 || w==NULL || trie_isEmpty(T)!=0){
		//fprintf(stderr, "Error: string given is empty\n");
		return -1;
	}

	int i, index=0;
	edge curr = T->root;
	
	for(i=0; i<strlen(w); i++){
		if((curr = chL_search(&curr->cl, w[i])) == NULL){
			//word not found
			return -1;
		}
	}
	if(curr != NULL && curr->isEnd_node==1){
		//word found
		mL_insert(path, mapid, &curr->ml, doc);
		return 0;
	}
	else
		return -1;
}


//// print functions.
void rec_print(edge e, char *s, int level)				//recursive print function of trie(secondary)
{
	if(e->isEnd_node==1){
		s[level] = e->letter;
		s[level+1] = '\0';
		printf("%s\n", s);
		//mL_print(&e->ml);
		//printf("\n");
	}

	int i;
	for(i=0; i<e->cl.count; i++){
		s[level] = e->letter;
		edge temp = chL_returnpos(&e->cl, i+1);
		rec_print(temp, s, level+1);
	}
}

void trie_print(Trie *T)
{
	if(trie_isEmpty(T)){
		//fprintf(stderr, "Trie is empty!\n");
		return;
	}
	char *s = malloc(300*sizeof(char));
	rec_print(T->root, s, 0);
	free(s);
}


//// delete functions
void rec_delete(edge e)
{
	if(e->isEnd_node==1){
		mL_freelist(&e->ml);
	}
	if(chL_isEmpty(&e->cl)){
		free(e);
		return;
	}
	else{
		int i;
		for(i=0; i<e->cl.count; i++){
			edge temp = chL_returnpos(&e->cl, i+1);
			rec_delete(temp);
		}
		chL_freelist(&e->cl);
		free(e);
	}
}

void trie_delete(Trie *T)
{
	if(trie_isEmpty(T)){
		//fprintf(stderr, "Trie is empty!\n");
		return;
	}
	rec_delete(T->root);
	T->root = NULL;
}




