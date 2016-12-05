#include "heap.h"

void
heap_init(struct heap* h){
    h->array = NULL;
    h->size = 0;
    h->allocated = 0;
}

void heap_destroy(struct heap* h){
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
    while (min != idx) {
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

        if(min != idx) {
            heap_swap(h, idx, min);
            idx = min;
            min = -1;
        }   
    }
}

struct heap_node* 
heap_delete(struct heap *h)
{
    struct heap_node *removed;
    struct heap_node *temp = h->array[h->size--];
    /* Shrink? */
    // if ((h->size <= (h->allocated + 2)) && (h->allocated > initial_size))
    // {
    //     h->allocated -= 1;
    //     h->array = xrealloc(h->array, sizeof(int) * h->allocated);
    //     if (!h->array) exit(-1); // Exit if the memory allocation fails
    // }
    removed = h->array[1];
    h->array[1] = temp;
    printf("HA %p\n", removed);
    min_heapify(h, 1, h->size);
    return removed;
}

// static
// void heap_display(struct heap *h) {
//     size_t i;
//     for(i=1; i <= h->size; ++i) {
//         printf("|%ld|", h->array[i]->priority);
//     }
//     printf("\n");
// }

// int main(int argc, char const *argv[])
// {
//     struct heap *h = xmalloc(sizeof(struct heap));
//     int i;
//     for (i = 1; i < 10; ++i){
//         struct heap_node *node = xmalloc(sizeof(struct heap_node));
//         heap_insert(h, node, i);
//     }
//     // heap_display(h);
//     size_t s = h->size;
//     for (i = 1; i < s; ++i){
//         free(heap_delete(h));
//         heap_display(h);
//     }
//     printf("%lu\n", h->size);
    
//     // heap_delete(h);
//     // heap_display(h);
//     return 0;
// }