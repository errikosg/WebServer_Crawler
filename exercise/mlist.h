#ifndef MLIST_H
#define MLIST_H

#include "plist.h"

//this is a linked list ---> represents the info for every unique file we keep in trie (every node has a list).
//header of mlist.c

typedef struct mainListNode *lk;			//link
struct mainListNode{
	char *pathName;					//pathName of file
	int mapID;						//position in maps(worker...)
	postList pl;					//posting list for this file
	//int entries;					//number of entries in posting list -^-

	lk next;
};

typedef struct{
	lk head;
	lk tail;
	int count;
}mainList;


//functions
void mL_initialize(mainList *);
int mL_isEmpty(mainList *);
void mL_insert(char *, int, mainList *, int);			//insert info, do pL_insert the last int(doc) + 1
postList *mL_search(int, mainList *);					//searches according to Map id.
int mL_getEntries(int, mainList *);						//returns the number of entries in the posting list of node, checks for a file only(id = mapID)
void mL_print(mainList *);
void mL_freelist(mainList *);

#endif
