/* Copyright (c) 2016-2017, Eder Leao Fernandes
 * All rights reserved.
 *
 * The contents of this file are subject to the license defined in
 * file 'doc/LICENSE'.
 *
 *
 * Author: Eder Leao Fernandes <e.leao@qmul.ac.uk>
 */

#include "heap.h"

void
heap_init(struct heap *h){
    h->array = NULL;
    h->size = 0;
    h->allocated = 0;
}

void heap_destroy(struct heap *h){
    if (h){
        free(h->array);
    }
}

void heap_insert(struct heap *h, struct heap_node *node, uint64_t priority)
{
    size_t idx, parent;
    if (h->size >= h->allocated){
        h->allocated = h->size == 0? 1 : 2 * h->size; 
        h->array = xrealloc(h->array, (h->allocated + 1) * sizeof(*h->array));
    }
    /* Add node to the last position */
    idx = ++h->size;
    add_node(h, node, idx);
    node->priority = priority;

    /* Bubble up */
    for(;idx > 1; idx = parent)
    {
        parent = heap_parent(idx);
        if (h->array[parent]->priority <= node->priority){ 
            break;
        }
        heap_swap(h, idx, parent);
    }
}

static void 
min_heapify(struct heap *h, size_t idx, size_t size) 
{
    size_t left, right, min;
    min = 0; 
    struct heap_node **arr = h->array;
    while (min < h->size) {
        min = idx;
        left = heap_left_child(idx);
        right = heap_right_child(idx);
        
        /* Compare with left child */
        if (left <= size && arr[left]->priority < arr[min]->priority) {
            min = left;
        } 
        /* Compare with right child */
        if (right <= size && arr[right]->priority < arr[min]->priority) {
            min = right;
        } 

        if(min == idx) {
            break;
        }
        heap_swap(h, idx, min);
        idx = min;
    }
}

struct heap_node* 
heap_delete(struct heap *h)
{
    struct heap_node *removed;
    struct heap_node *temp = h->array[h->size--];
    removed = h->array[1];
    h->array[1] = temp;
    min_heapify(h, 1, h->size);
    return removed;
}

struct heap_node* heap_retrieve(struct heap *h)
{
    return h->array[1];
}
