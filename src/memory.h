#ifndef _MEMORY_H
#define _MEMORY_H

#include <stdlib.h> // size_t

void *chk_malloc(size_t memsize);
void *chk_calloc(size_t num, size_t elem_size);
void chk_realloc(void **p, size_t memsize);
void chk_strcpy(char **dest, const char *src);
void chk_free(void *p);

#endif // _MEMORY_H