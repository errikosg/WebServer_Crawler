#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "hashtable.h"


static h_entry* h_NewItem(char *k, char *v)
{
	h_entry *new = malloc(sizeof(h_entry));
	new->value = strdup(v);
	new->key = strdup(k);
}

hash_table* h_New()
{
	hash_table *ht = malloc(sizeof(hash_table));
	ht->size = TABLE_SIZE;
	ht->count = 0;
	//ht->items = malloc(TABLE_SIZE*sizeof(h_entry *));
	ht->items = calloc(ht->size, sizeof(h_entry *));
	return ht;
}

static void h_DelItem(h_entry *x)
{
	free(x->key);
	free(x->value);
	free(x);
}

void h_DelTable(hash_table *ht)
{
	int i;
	for(i=0; i<ht->size; i++){
		h_entry *itm = ht->items[i];
		if(itm != NULL)
			h_DelItem(itm);
	}
	free(ht->items);
	free(ht);
}

unsigned long hashfunction1(char* str){			//algorithm djb2 from:	www.cse.yorku.ca/~oz/hash.html
	unsigned long hash = 5381;
	int c;
	while(c = *str++)
		hash = ((hash<<5)+hash)+c;	
	return hash;
}

unsigned long hashfunction2(char *str)
{
	unsigned long hash = 0;
	int c;
	while (c = *str++)
		hash = c + (hash << 6) + (hash << 16) - hash;

	return hash;
}

static int getHash(char *str, int bucket_num, int att)
{
	//unsigned long v1 = hashfunction1(str)%bucket_num;
	//unsigned long v2 = hashfunction2(str);
	int v1 = (int)hashfunction1(str);
	int v2 = (int)hashfunction2(str);
	return (v1 + (att * (v2+1))) % bucket_num;
}

void h_insert(hash_table *ht, char *key, char *value)
{
	h_entry *itm = h_NewItem(key, value);
	int i=0, index;

	index = getHash(key, ht->size, i);
	index = abs(index);
	h_entry *curr = ht->items[index];
	while(curr != NULL){
		i++;
		if(i<20){
			index = getHash(key, ht->size, i);
			index = abs(index);
		}
		else
			index = (index+1)%ht->size;
		curr = ht->items[index];
	}
	ht->items[index] = itm;
	ht->count++;
}

char *h_search(hash_table *ht, char *key)
{
	int index, i=1, x;
	index = getHash(key, ht->size, 0);
	index = abs(index);
	h_entry *itm = ht->items[index];

	while(itm != NULL){
		if(strcmp(itm->key, key)==0){
			//printf("\n\t**EPISTREFW token %s se %d\n", itm->value, index);
			return itm->value;
		}
		if(i<20){
			index = getHash(key, ht->size, i);
			index = abs(index);
		}
		else{
			index = (index+1)%(ht->size);
		}
		//printf("%d.", index);
		itm = ht->items[index];
		i++;
	}
	//printf("\n\t**EPISTREFW NULL gia %d\n", index);
	return NULL;
}


