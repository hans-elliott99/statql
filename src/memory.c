#include "global.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>


void *chk_malloc(size_t memsize) {
    void *p;
    if (memsize == 0)
        return NULL;
    p = malloc(memsize);
    if (!p) {
        fprintf(stderr, "chk_malloc: memory allocation failed!\n");
        exit(1);
    }
    mem++;
    // printf("mem: %d\n", mem);
    return p;
}
void *chk_calloc(size_t num, size_t elem_size) {
    void *p;
    if (num == 0)
        return NULL;
    p = calloc(num, elem_size);
    if (!p) {
        fprintf(stderr, "chk_calloc: memory allocation failed!\n");
        exit(1);
    }
    mem++;
    // printf("mem: %d\n", mem);
    return p;
}

void chk_realloc(void **p, size_t memsize) {
    if (memsize == 0) {
        chk_free(*p);
        return;
    }
    *p = realloc(*p, memsize);
    if (!*p) {
        fprintf(stderr, "chk_realloc: memory allocation failed!\n");
        exit(1);
    }
}

/*pass dest by ref, and make sure to free*/
void chk_strcpy(char **dest, const char *src) {
    if (src == NULL)
        return;
    size_t n = strlen(src) + 1;
    (*dest) = chk_malloc(n * sizeof(char)); // mem++
    strncpy(*dest, src, n);
}

void chk_free(void *p) {
    if (p) {
        free(p);
        mem--;
        // printf("mem: %d\n", mem);
    }
}
