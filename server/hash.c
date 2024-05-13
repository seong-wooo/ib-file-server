#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "hash.h"

struct hash_map_s* create_hash_map(int size) {
    struct hash_map_s* hash_map = (struct hash_map_s*)malloc(sizeof(struct hash_map_s));
    if (hash_map == NULL) {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    hash_map->size = size;
    hash_map->buckets = (struct hash_node_s**)malloc(sizeof(struct hash_node_s*) * size);
    if (hash_map->buckets == NULL) {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; ++i) {
        hash_map->buckets[i] = NULL;
    }

    return hash_map;
}

int hash(struct hash_map_s* hash_map, uint32_t key) {
    return key % hash_map->size;
}

void put(struct hash_map_s* hash_map, uint32_t key, void* value) {
    int index = hash(hash_map, key);
    struct hash_node_s* new_node = (struct hash_node_s*)malloc(sizeof(struct hash_node_s));
    if (new_node == NULL) {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }
    new_node->key = key;
    new_node->value = value;
    new_node->next = NULL;

    if (hash_map->buckets[index] == NULL) {
        hash_map->buckets[index] = new_node;
    } else {
        struct hash_node_s* temp = hash_map->buckets[index];
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_node;
    }
}

void* get(struct hash_map_s* hash_map, uint32_t key) {
    int index = hash(hash_map, key);

    struct hash_node_s* temp = hash_map->buckets[index];
    while (temp != NULL) {
        if (temp->key == key) {
            return temp->value;
        }
        temp = temp->next;
    }

    return NULL;
}

void remove_key(struct hash_map_s* hash_map, uint32_t key) {
    if (hash_map == NULL) {
        return;
    }

    int index = hash(hash_map, key);
    struct hash_node_s* current = hash_map->buckets[index];
    struct hash_node_s* prev = NULL;

    while (current != NULL) {
        if (current->key == key) {
            if (prev == NULL) {
                hash_map->buckets[index] = current->next;
            } 
            else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

void free_hash_map(struct hash_map_s* hash_map) {
    if (hash_map == NULL) {
        return;
    }
    for (int i = 0; i < hash_map->size; ++i) {
        struct hash_node_s* node = hash_map->buckets[i];
        while (node != NULL) {
            struct hash_node_s* temp = node;
            node = node->next;
            free(temp);
        }
    }
    free(hash_map->buckets);
    free(hash_map);
}