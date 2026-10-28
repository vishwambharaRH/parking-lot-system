#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#include <stdbool.h>
#include <stddef.h>

/* A fixed-capacity binary min-heap of ints: the smallest value is always on top.
 * Used for free slot ranks, so the top is always the nearest free slot. */
typedef struct {
    int *data;
    size_t size;
    size_t capacity;
} MinHeap;

void min_heap_init(MinHeap *heap, size_t capacity);
void min_heap_free(MinHeap *heap);

/* Pushing onto a full heap is a programming error (checked with assert). */
void min_heap_push(MinHeap *heap, int value);

/* Removes the smallest value into *out. Returns false if the heap is empty. */
bool min_heap_pop(MinHeap *heap, int *out);

#endif /* MIN_HEAP_H */
