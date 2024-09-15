#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdint.h>
#include "list.h"

extern int mem;
extern uint32_t global_seed;

extern struct DLList memstack;
void init_memstack(void);
void free_memstack(void);

#endif // _GLOBAL_H_