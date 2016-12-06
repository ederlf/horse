/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#include "util.h"

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