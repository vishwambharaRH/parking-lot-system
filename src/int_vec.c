// vectors, because they aren't available in C
#include "int_vec.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define INT_VEC_INITIAL_CAPACITY 8

void int_vec_free(IntVec *v) {
    free(v->data);
    v->data = NULL;
    v->size = 0;
    v->capacity = 0;
}

void int_vec_push(IntVec *v, int value) {
    if (v->size == v->capacity) {
        /* Doubling keeps the average cost of a push O(1). */
        size_t new_capacity = v->capacity > 0 ? v->capacity * 2 : INT_VEC_INITIAL_CAPACITY;
        v->data = xrealloc(v->data, new_capacity * sizeof *v->data);
        v->capacity = new_capacity;
    }
    v->data[v->size] = value;
    v->size++;
}

void int_vec_swap_remove(IntVec *v, size_t index) {
    assert(index < v->size);
    v->size--;
    v->data[index] = v->data[v->size];
}

static int compare_ints(const void *a, const void *b) {
    int x = *(const int *)a;
    int y = *(const int *)b;
    /* Not x - y, which can overflow for large values of opposite sign. */
    return (x > y) - (x < y);
}

int *int_vec_sorted_copy(const IntVec *v) {
    int *copy = xmalloc(v->size * sizeof *copy);
    /* memcpy from a NULL pointer is undefined even for 0 bytes, and an empty IntVec has data == NULL. */
    if (v->size > 0) {
        memcpy(copy, v->data, v->size * sizeof *copy);
    }
    qsort(copy, v->size, sizeof *copy, compare_ints);
    return copy;
}
