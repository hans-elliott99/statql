#include "global.h"
#include "list.h"
#include <stdio.h>

struct DLList memstack = {/*head=*/NULL, /*tail=*/NULL, /*len=*/0};
int mem = 0;


void init_memstack(void) {
    if (atexit(free_memstack)) {
        fprintf(stderr, "Failed to register 'free_memstack'\n");
        exit(1);
    }
}


// atexit free all consumed memory
void free_memstack(void) {
    for (struct DLNode *n = memstack.tail; n != NULL; n = n->prev) {
        dllist_remove(&memstack, n);
    }
    printf("\n  ~Final memstack len: %zu\n", memstack.len);
    printf("  ~Memory leaks: %d\n", mem);
}