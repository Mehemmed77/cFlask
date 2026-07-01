#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdbool.h>

#define FNV_32_PRIME ((uint32_t)0x01000193)
#define FNV_32_OFFSET ((uint32_t)0x811C9DC5)
#define INITIAL_CAPACITY 8

typedef struct bucket_t {
    char* key;
    void* value;
    struct bucket_t* next;
} bucket;

typedef struct hashmap_t {
    int capacity;
    int size;
    bucket** buckets;
} hashmap;

hashmap* hashmap_create();
void hashmap_put(hashmap* map, char* key, void* value);
void hashmap_destroy(hashmap* map);
void* hashmap_get(hashmap* map, char* key);
bool hashmap_remove(hashmap* map, char* key);

#endif