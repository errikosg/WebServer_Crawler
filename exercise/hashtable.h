#ifndef HASHTABLE_H
#define HASHTABLE_H

#define TABLE_SIZE 145

typedef struct{
	char *value;
	char *key;
}h_entry;

typedef struct{
	int size;
	int count;
	h_entry **items;
}hash_table;


//funcs
static h_entry* h_NewItem(char *, char *);
hash_table* h_New();
static void h_DelItem(h_entry *);
void h_DelTable(hash_table *);
unsigned long hashfunction1(char *);
unsigned long hashfunction2(char *);
static int getHash(char *, int, int);
void h_insert(hash_table *, char *, char *);
char *h_search(hash_table *, char *);
//delete?


#endif
