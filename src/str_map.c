#include "str_map.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define STR_MAP_INITIAL_CAPACITY 16

/* FNV-1a: a simple, well-spread hash for short strings. */
static size_t hash_string(const char *s) {
    uint64_t hash = 14695981039346656037ULL;
    for (; *s != '\0'; s++) {
        hash ^= (unsigned char)*s;
        hash *= 1099511628211ULL;
    }
    return (size_t)hash;
}

/* Returns the bucket holding key, or the empty bucket where it would be inserted.
 * Capacity is a power of two, so "& mask" is a cheap "% capacity". The map is
 * never full, so the loop always reaches an empty bucket. */
static size_t find_bucket(const StrMapEntry *entries, size_t capacity, const char *key) {
    size_t mask = capacity - 1;
    size_t i = hash_string(key) & mask;
    while (entries[i].key != NULL && strcmp(entries[i].key, key) != 0) {
        i = (i + 1) & mask;
    }
    return i;
}

void str_map_init(StrMap *map) {
    map->capacity = STR_MAP_INITIAL_CAPACITY;
    map->count = 0;
    map->entries = xcalloc(map->capacity, sizeof *map->entries);
}

void str_map_free(StrMap *map) {
    for (size_t i = 0; i < map->capacity; i++) {
        free(map->entries[i].key);
    }
    free(map->entries);
    map->entries = NULL;
    map->capacity = 0;
    map->count = 0;
}

static void grow(StrMap *map) {
    size_t new_capacity = map->capacity * 2;
    StrMapEntry *new_entries = xcalloc(new_capacity, sizeof *new_entries);
    /* Re-insert every entry. The key pointers move over as-is, nothing is copied. */
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].key != NULL) {
            size_t j = find_bucket(new_entries, new_capacity, map->entries[i].key);
            new_entries[j] = map->entries[i];
        }
    }
    free(map->entries);
    map->entries = new_entries;
    map->capacity = new_capacity;
}

bool str_map_get(const StrMap *map, const char *key, int *out) {
    size_t i = find_bucket(map->entries, map->capacity, key);
    if (map->entries[i].key == NULL) {
        return false;
    }
    *out = map->entries[i].value;
    return true;
}

void str_map_put(StrMap *map, const char *key, int value) {
    size_t i = find_bucket(map->entries, map->capacity, key);
    if (map->entries[i].key != NULL) {
        map->entries[i].value = value;
        return;
    }
    /* Keep the map at most half full so probe chains stay short. */
    if ((map->count + 1) * 2 > map->capacity) {
        grow(map);
        i = find_bucket(map->entries, map->capacity, key);
    }
    map->entries[i].key = xstrdup(key);
    map->entries[i].value = value;
    map->count++;
}

bool str_map_remove(StrMap *map, const char *key) {
    size_t mask = map->capacity - 1;
    size_t hole = find_bucket(map->entries, map->capacity, key);
    if (map->entries[hole].key == NULL) {
        return false;
    }
    free(map->entries[hole].key);
    map->entries[hole].key = NULL;
    map->count--;

    /* Backward-shift deletion. Simply emptying the bucket would break lookups for
     * keys further along the same probe chain, because find_bucket stops at the
     * first empty bucket. So walk the rest of the chain and move each entry back
     * into the hole if its probe sequence passes through the hole on the way to
     * where it sits now. */
    size_t i = (hole + 1) & mask;
    while (map->entries[i].key != NULL) {
        size_t home = hash_string(map->entries[i].key) & mask;
        size_t home_to_i = (i - home) & mask;
        size_t hole_to_i = (i - hole) & mask;
        if (hole_to_i <= home_to_i) {
            map->entries[hole] = map->entries[i];
            map->entries[i].key = NULL;
            hole = i;
        }
        i = (i + 1) & mask;
    }
    return true;
}
