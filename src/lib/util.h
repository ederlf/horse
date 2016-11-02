#ifndef UTIL_H
#define UTIL_H 1

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

void out_of_memory(void);
void *xmalloc(int size);
void *xrealloc(void *v, int size);


#endif