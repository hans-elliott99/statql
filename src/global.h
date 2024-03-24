#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "list.h"

extern int mem;

extern struct DLList memstack;
void init_memstack(void);
void free_memstack(void);

#endif // _GLOBAL_H_