/* IntVec, MinHeap and StrMap: small unit checks, then random operations compared
 * step by step against simple reference implementations. */

#include <stdbool.h>
#include <stdlib.h>

#include "int_vec.h"
#include "min_heap.h"
#include "str_map.h"
#include "test_framework.h"

static int compare_ints(const void *a, const void *b) {
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

static void test_int_vec_basics(void) {
    IntVec v = {0};
    int *empty = int_vec_sorted_copy(&v);
    CHECK(v.size == 0);
    free(empty);

    int_vec_push(&v, 30);
    int_vec_push(&v, 10);
    int_vec_push(&v, 20);
    int_vec_push(&v, 40);
    int_vec_swap_remove(&v, 1); /* Removes 10; 40 moves into index 1. */
    CHECK(v.size == 3);
    CHECK(v.data[0] == 30 && v.data[1] == 40 && v.data[2] == 20);

    int *sorted = int_vec_sorted_copy(&v);
    CHECK(sorted[0] == 20 && sorted[1] == 30 && sorted[2] == 40);
    CHECK(v.data[0] == 30); /* Original order untouched. */
    free(sorted);
    int_vec_free(&v);
}

static void test_int_vec_random(void) {
    enum { MAX = 5000 };
    static int reference[MAX];
    size_t n = 0;
    IntVec v = {0};
    srand(1);
    bool matches = true;
    for (int step = 0; step < 200000; step++) {
        if (n == 0 || (rand() % 3 != 0 && n < MAX)) {
            int x = rand() - RAND_MAX / 2;
            int_vec_push(&v, x);
            reference[n++] = x;
        } else {
            size_t i = (size_t)rand() % n;
            int_vec_swap_remove(&v, i);
            reference[i] = reference[--n];
        }
        if (v.size != n || (n > 0 && memcmp(v.data, reference, n * sizeof reference[0]) != 0)) {
            matches = false;
        }
    }
    CHECK(matches);
    int *sorted = int_vec_sorted_copy(&v);
    qsort(reference, n, sizeof reference[0], compare_ints);
    for (size_t i = 0; i < n; i++) {
        matches = matches && sorted[i] == reference[i];
    }
    CHECK(matches);
    free(sorted);
    int_vec_free(&v);
}

static void test_min_heap_basics(void) {
    MinHeap heap;
    min_heap_init(&heap, 5);
    int out = -1;
    CHECK(!min_heap_pop(&heap, &out));
    min_heap_push(&heap, 5);
    min_heap_push(&heap, 1);
    min_heap_push(&heap, 4);
    min_heap_push(&heap, 1);
    min_heap_push(&heap, 3);
    int expected[] = {1, 1, 3, 4, 5};
    for (int i = 0; i < 5; i++) {
        CHECK(min_heap_pop(&heap, &out) && out == expected[i]);
    }
    CHECK(!min_heap_pop(&heap, &out));
    min_heap_free(&heap);
}

static void test_min_heap_random(void) {
    enum { CAPACITY = 3000, RANGE = 500 };
    int counts[RANGE] = {0};
    size_t n = 0;
    MinHeap heap;
    min_heap_init(&heap, CAPACITY);
    srand(2);
    bool matches = true;
    for (int step = 0; step < 300000; step++) {
        if (n == 0 || (rand() % 2 && n < CAPACITY)) {
            int x = rand() % RANGE;
            min_heap_push(&heap, x);
            counts[x]++;
            n++;
        } else {
            int out;
            int smallest = 0;
            while (counts[smallest] == 0) {
                smallest++;
            }
            matches = matches && min_heap_pop(&heap, &out) && out == smallest;
            counts[smallest]--;
            n--;
        }
    }
    CHECK(matches);
    min_heap_free(&heap);
}

static void test_str_map_basics(void) {
    StrMap map;
    str_map_init(&map);
    int out = -1;
    CHECK(!str_map_get(&map, "missing", &out));
    CHECK(!str_map_remove(&map, "missing"));

    char key[] = "KA-01";
    str_map_put(&map, key, 7);
    key[0] = 'X'; /* The map keeps its own copy of the key. */
    CHECK(str_map_get(&map, "KA-01", &out) && out == 7);
    CHECK(!str_map_get(&map, "XA-01", &out));

    str_map_put(&map, "KA-01", 8); /* Overwrite. */
    CHECK(str_map_get(&map, "KA-01", &out) && out == 8);
    CHECK(map.count == 1);
    CHECK(str_map_remove(&map, "KA-01"));
    CHECK(!str_map_get(&map, "KA-01", &out));
    CHECK(map.count == 0);
    str_map_free(&map);
}

static void test_str_map_random(void) {
    enum { KEYS = 3000 };
    static int reference[KEYS];
    static bool present[KEYS];
    size_t n = 0;
    StrMap map;
    str_map_init(&map);
    srand(3);
    bool matches = true;
    char key[32];
    for (int step = 0; step < 400000; step++) {
        int k = rand() % KEYS;
        snprintf(key, sizeof key, "KA-%d", k);
        int op = rand() % 3;
        int out;
        if (op == 0) {
            int value = rand();
            str_map_put(&map, key, value);
            n += present[k] ? 0 : 1;
            present[k] = true;
            reference[k] = value;
        } else if (op == 1) {
            matches = matches && str_map_remove(&map, key) == present[k];
            n -= present[k] ? 1 : 0;
            present[k] = false;
        } else {
            bool found = str_map_get(&map, key, &out);
            matches = matches && found == present[k] && (!found || out == reference[k]);
        }
        matches = matches && map.count == n;
    }
    for (int k = 0; k < KEYS; k++) {
        int out;
        snprintf(key, sizeof key, "KA-%d", k);
        bool found = str_map_get(&map, key, &out);
        matches = matches && found == present[k] && (!found || out == reference[k]);
    }
    CHECK(matches);
    str_map_free(&map);
}

int main(void) {
    RUN_TEST(test_int_vec_basics);
    RUN_TEST(test_int_vec_random);
    RUN_TEST(test_min_heap_basics);
    RUN_TEST(test_min_heap_random);
    RUN_TEST(test_str_map_basics);
    RUN_TEST(test_str_map_random);
    TEST_SUMMARY();
}
