#include "list.h"
#include "memory.h"
#include <stdio.h>



void dllist_append(struct DLList *list, struct DLNode *newnode) {
    if (list->len == 0) {
        // first node
        list->head = newnode;
        list->tail = newnode;
        newnode->prev = NULL;
        newnode->next = NULL;
    } else {
        if (list->tail->next != NULL) {
            fprintf(stderr, "dllist_append: tail->next not NULL\n");
            exit(1);
        }
        list->tail->next = newnode;
        newnode->prev = list->tail;
        list->tail = newnode;
    }
    list->len++;
}

void dllist_remove(struct DLList *list, struct DLNode *node) {
    if (node->prev == NULL) { // if node is head
        list->head = node->next;
    } else {
        node->prev->next = node->next;
    }
    if (node->next == NULL) { // if node is tail
        list->tail = node->prev;
    } else {
        node->next->prev = node->prev;
    }
    free_arraystruct_data(node->arr);
    chk_free(node->arr);
    chk_free(node);
    list->len--;
}

void dllist_pop(struct DLList *list) {
    if (list->len == 0) {
        return;
    }
    struct DLNode *last = list->tail;
    if (list->len == 1) {
        list->head = NULL;
        list->tail = NULL;
    } else {
        list->tail = last->prev;
        list->tail->next = NULL;
    }
    free_arraystruct_data(last->arr);
    chk_free(last->arr);
    chk_free(last);
    list->len--;
}