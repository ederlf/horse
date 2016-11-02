#include "heap.h"

void
heap_init(struct heap *h){
    heap->array = NULL;
    heap->size = 0;
    heap->allocated = 0;
}

/*  idx is the position of the children in the heap 
*   If the node is not in the root, the parent can be 
*   found in the position n/2.
*/
uint32_t 
heap_parent(uint32_t idx){
    return n == 1? -1 : idx/2;
}

uint32_t heap_young_child(uint32_t idx){
    return 2*n;
}