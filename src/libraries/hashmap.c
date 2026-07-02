#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../../include/hashmap.h"

// ================== HASH FUNCTION ==================

uint32_t fnv1a_32_str(char* key)  {
    uint32_t hash = FNV_32_OFFSET;
    
    while(*key) {
        hash ^= (uint32_t) (unsigned char) (*key);
        hash *= FNV_32_PRIME;
        key++;
    }
    
    return hash;
}

// ================== HASH FUNCTION ==================

// ================== HELPER FUNCTIONS ==================

bucket* init_bucket(char* key, void* value) {
    bucket* b = malloc(sizeof(bucket));

    if (!b) return NULL;

    b->key = strdup(key);
    if (!b->key) {
        free(b);
        return NULL;
    }

    b->value = value;
    b->next = NULL;

    return b;
}

void insert_existing_bucket(
    bucket** buckets,
    bucket* existing_bucket,
    int capacity
)
{
    uint32_t hash = fnv1a_32_str(existing_bucket->key);
    size_t idx = hash % capacity;

    bucket* b = buckets[idx];

    if (b == NULL) {
        buckets[idx] = existing_bucket;
        return;
    }

    bucket* prev = NULL;
    while(b != NULL) {
        prev = b;
        b = b-> next;
    }

    prev->next = existing_bucket;
}

bool insert_new_bucket(
    bucket** buckets,
    char* key,
    void* value,
    int capacity
) {
    // returns true if new key was added, otherwise false
    uint32_t hash = fnv1a_32_str(key);
    size_t idx = hash % capacity;

    bucket* b = buckets[idx];

    if (b == NULL) {
        buckets[idx] = init_bucket(key, value);
        return true;
    }

    bucket* prev = NULL;
    while(b != NULL) {
        if (strcmp(b->key, key) == 0) {
            free(b->value);
            b->value = value;
            return false;
        }

        prev = b;
        b = b-> next;
    }

    prev->next = init_bucket(key, value);
    return true;
}

void destroy_bucket(bucket* b) {
    b->next = NULL;
    free(b->value);
    free(b->key);
    free(b);
}

bucket** resize(hashmap* map) {
    int old_capacity = map->capacity;

    bucket** old_buckets = map->buckets;
    bucket** new_buckets = calloc(old_capacity * 2, sizeof(bucket*));

    for(int idx = 0; idx < old_capacity; idx++) {
        bucket* b = old_buckets[idx];

        bucket* next;

        while(b != NULL) {
            next = b->next; 
            b->next = NULL;
            insert_existing_bucket(new_buckets, b, old_capacity * 2);
            b = next;
        }
    }

    return new_buckets;
}
// ================== HELPER FUNCTIONS ==================

// ================== PUBLIC FUNCTIONS ==================
hashmap* hashmap_create() {
    hashmap* map = malloc(sizeof(hashmap));
    if(!map) return NULL;
    
    map->capacity = INITIAL_CAPACITY;
    map->size = 0;
    
    map->buckets = calloc(INITIAL_CAPACITY, sizeof(bucket*));
    if(!map->buckets) {
        free(map);
        return NULL;
    }
    
    return map;
}


void hashmap_put(hashmap* map, char* key, void* value) {
    float load_factor = (float) map->size / map->capacity;
    
    if (load_factor > 0.75) {
        bucket** old_buckets = map->buckets;
        bucket** new_buckets = resize(map);
        free(old_buckets);
        map->capacity *= 2;
        map->buckets = new_buckets;
    }
    
    bool new_key_was_added = insert_new_bucket(map->buckets, key, value, map->capacity);
    
    if(new_key_was_added) map->size++; 
}

void hashmap_destroy(hashmap* map) {
    if(map == NULL) return;

    for(int idx = 0; idx < map->capacity; idx++) {
        bucket* b = map->buckets[idx];
        
        bucket* temp;
        while(b != NULL) {
            temp = b->next;
            destroy_bucket(b);
            b = temp;
        }
    }
    
    free(map->buckets);
    free(map);
}

void* hashmap_get(hashmap* map, char* key) {
    uint32_t hash = fnv1a_32_str(key);
    size_t idx = hash % map->capacity;
    
    bucket* b = map->buckets[idx];
    
    while(b != NULL) {
        if (strcmp(b->key, key) == 0) return b->value;
        b = b->next;
    }
    
    return NULL;
}

bool hashmap_remove(hashmap* map, char* key) {
    uint32_t hash = fnv1a_32_str(key);
    size_t idx = hash % map->capacity;
    
    bucket* b = map->buckets[idx];
    
    bucket* prev = NULL;
    while(b != NULL) {
        if(strcmp(b->key, key) == 0) {
            if(prev == NULL) {
                map->buckets[idx] = b->next;
            }
            
            else {
                prev->next = b->next;
            }
            
            map->size--;
            destroy_bucket(b);
            
            return true;
        }
        
        prev = b;
        b = b->next;
    }
    
    return false;
}
