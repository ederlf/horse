#ifndef HEAP_H
#define HEAP_H 1

#include "util.h"

struct heap_node {
    uint64_t priority;
    size_t idx;
};

struct heap {
    struct heap_node **array;
    size_t size;
    size_t allocated;
};

void heap_init(struct heap* h);
void heap_destroy(struct heap* h);
void heap_insert(struct heap* h, struct heap_node *node, uint64_t priority);
struct heap_node* heap_delete(struct heap *h);


/*  idx is the position of the children in the heap 
*   If the node is not in the root, the parent can be 
*   found in the position n/2.
*/
static inline size_t 
heap_parent(size_t idx)
{
    return idx/2;
}

static inline size_t 
heap_right_child(size_t idx)
{
    return 2*idx;
}

static inline size_t 
heap_left_child(size_t idx)
{
    return 2*idx + 1;
}

static inline void
add_node(struct heap *h, struct heap_node *node, size_t idx)
{
    node->idx = idx;
    h->array[idx] = node;
}

static inline void
heap_swap(struct heap *h, size_t i, size_t j)
{
    struct heap_node *tmp = h->array[i];
    add_node(h, h->array[j], i);
    add_node(h, tmp, j);
}

#endif