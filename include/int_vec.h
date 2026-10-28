#ifndef INT_VEC_H
#define INT_VEC_H

#include <stddef.h>

/* A growable array of ints, the C stand-in for std::vector<int>.
 * Zero-initialising one (IntVec v = {0};) gives a valid empty vector. */
typedef struct {
    int *data;
    size_t size;
    size_t capacity;
} IntVec;

void int_vec_free(IntVec *v);

void int_vec_push(IntVec *v, int value);

/* Removes data[index] in O(1) by moving the last element into its place.
 * Order is not preserved: the element that used to be last is now at index. */
void int_vec_swap_remove(IntVec *v, size_t index);

/* Returns a malloc'd copy of the contents sorted ascending (caller frees).
 * The vector itself is left unchanged. */
int *int_vec_sorted_copy(const IntVec *v);

#endif /* INT_VEC_H */
