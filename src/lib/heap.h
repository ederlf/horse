#ifndef HEAP_H
#define HEAP_H 1

#include "util.h"

struct heap_node {
    uint64_t priority;
    uint32_t idx;
};

struct heap {
    struct heap_node **array;
    size_t size;
    size_t allocated;
};

void heap_init();
uint32_t heap_parent(uint32_t idx);


#endif