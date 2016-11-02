#include "util.h"
#include <stdio.h>
#include <stdlib.h>

void out_of_memory()
{
    fprintf(stderr, "No memory available. Aborting.\n");
    abort();
}

void *
xmalloc(int size)
{
    void *v = malloc(size? size : 1);
    if (v == NULL){
        out_of_memory();
    }
    return v;
}

void *
xrealloc(void *v, int size)
{
    v = realloc(v, size? size : 1);
    if (v == NULL){
        out_of_memory();
    }    
    return v;
}