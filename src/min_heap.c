#include "min_heap.h"

#include <assert.h>
#include <stdlib.h>

#include "utils.h"

/* The heap is stored in an array: the children of index i are 2i+1 and 2i+2,
 * and its parent is (i-1)/2. Every parent is <= its children, so data[0] is the
 * smallest value. */

void min_heap_init(MinHeap *heap, size_t capacity) {
    heap->data = xmalloc(capacity * sizeof *heap->data);
    heap->size = 0;
    heap->capacity = capacity;
}

void min_heap_free(MinHeap *heap) {
    free(heap->data);
    heap->data = NULL;
    heap->size = 0;
    heap->capacity = 0;
}

static void swap_ints(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void min_heap_push(MinHeap *heap, int value) {
    assert(heap->size < heap->capacity);
    size_t i = heap->size;
    heap->data[i] = value;
    heap->size++;

    /* Sift up: swap with the parent while smaller than it. */
    while (i > 0) {
        size_t parent = (i - 1) / 2;
        if (heap->data[parent] <= heap->data[i]) {
            break;
        }
        swap_ints(&heap->data[parent], &heap->data[i]);
        i = parent;
    }
}

bool min_heap_pop(MinHeap *heap, int *out) {
    if (heap->size == 0) {
        return false;
    }
    *out = heap->data[0];
    heap->size--;
    heap->data[0] = heap->data[heap->size];

    /* Sift down: swap with the smaller child while larger than it. */
    size_t i = 0;
    for (;;) {
        size_t left = 2 * i + 1;
        size_t right = left + 1;
        size_t smallest = i;
        if (left < heap->size && heap->data[left] < heap->data[smallest]) {
            smallest = left;
        }
        if (right < heap->size && heap->data[right] < heap->data[smallest]) {
            smallest = right;
        }
        if (smallest == i) {
            break;
        }
        swap_ints(&heap->data[i], &heap->data[smallest]);
        i = smallest;
    }
    return true;
}
