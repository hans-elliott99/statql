#ifndef _LIST_H
#define _LIST_H

#include "array.h" //ArrayStruct

/*Doubly Linked List*/
struct DLNode {
    ArrayStruct *arr;
    struct DLNode *next;
    struct DLNode *prev;
};

struct DLList {
    struct DLNode *head;
    struct DLNode *tail;
    size_t len;
};


void dllist_append(struct DLList *list, struct DLNode *newnode);
void dllist_remove(struct DLList *list, struct DLNode *node);
void dllist_pop(struct DLList *list);

#endif // _LIST_H