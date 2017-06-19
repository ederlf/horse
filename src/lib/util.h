/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#ifndef UTIL_H
#define UTIL_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define UNUSED(x) (void)x;

void out_of_memory(void);
void file_read_error(void);
void *xmalloc(size_t size);
void *xrealloc(void *v, int size);
uint32_t random_at_most(uint32_t max);
char* file_to_string(const char * file_name, size_t *size);
int nlz(uint32_t x);
int ntz(uint32_t x);

#endif