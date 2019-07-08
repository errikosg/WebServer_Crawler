#ifndef LIST1_H
#define LIST1_H

//this is a linked list -> represents the posting list.
//(header of list1.c)

typedef struct postListNode* lnk;
struct postListNode{
	int doc_id;									//line
	int freq;
	lnk next;
};

typedef struct{
	lnk head;
	lnk tail;
	int count;									//n(qi) --> will be given in mainLIst in entries
}postList;

//functions
void pL_initialize(postList *);
int pL_isEmpty(postList *);
void pL_insert(int, int, postList *);
void pL_increase(int, postList *);
void pL_remove(int *, int *, postList *);
int pL_search(int, postList *);					//gets doc_id, returns freq / -1 if not found.
int pL_findDoc(int, postList *);
int pL_getPos(int, postList *);
int pL_getTotalFreq(postList *);
void pL_print(postList *);
void pL_copy(postList *, postList **);
void pL_freelist(postList *);

#endif
