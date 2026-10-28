#ifndef STR_MAP_H
#define STR_MAP_H

#include <stdbool.h>
#include <stddef.h>

/* A hash map from strings to ints, the C stand-in for
 * std::unordered_map<std::string, int>. Uses open addressing with linear probing
 * and grows automatically. The map keeps its own copy of each key. */
typedef struct {
    char *key; /* NULL marks an empty bucket. */
    int value;
} StrMapEntry;

typedef struct {
    StrMapEntry *entries;
    size_t capacity; /* Always a power of two. */
    size_t count;
} StrMap;

void str_map_init(StrMap *map);
void str_map_free(StrMap *map);

/* Stores the value for key in *out. Returns false if the key is absent. */
bool str_map_get(const StrMap *map, const char *key, int *out);

/* Inserts key, or overwrites its value if it is already present. */
void str_map_put(StrMap *map, const char *key, int value);

/* Removes key. Returns false if it was not present. */
bool str_map_remove(StrMap *map, const char *key);

#endif /* STR_MAP_H */
