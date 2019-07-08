#ifndef TRIE_H
#define TRIE_H

#include "mlist.h"

//interface of Trie data structure + list of TrieNode pointers(used for children)
//(header of trie.c)


typedef struct TrieNode* edge;				//link


//---------------------------------------------------------
//->>list
typedef struct chListNode{
	edge e;
	struct chListNode *next;
}chListNode;

typedef struct{
	chListNode *head;
	int count;
}chList;

//funcs
void chL_init(chList *);
int chL_isEmpty(chList *);
edge chL_insert(chList *, edge);				// ->
edge chL_search(chList *, char);				// ->
edge chL_returnpos(chList *, int);				// -> all these return the edge that we want --> to be used by trie.
void chL_print(chList *);
void chL_freelist(chList *);


//---------------------------------------------------------

struct TrieNode{
	chList cl;					//list of children nodes
	int isEnd_node;				//is end node
	//int isRoot;					//flag to ignore roots letter.
	char letter;				//letter of node
	mainList ml;				//if end node, has list -- every node for every file.
};

typedef struct{
	edge root;
}Trie;

//funcs / trie
void trie_init(Trie *);
int trie_isEmpty(Trie *);
int trie_insertV1(Trie *, char *, char *, int, int);			//version 1
int trie_insertV2(Trie *, char *, char *, int, int);			//version 2
postList *trie_search(Trie *, char *, int, int *);
mainList *trie_search_entries(Trie *, char *);

void rec_print(edge , char *, int);
void trie_print(Trie *);

void rec_delete(edge);
void trie_delete(Trie *);

#endif
