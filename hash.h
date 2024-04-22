#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct hash_node_s {
    void* key;
    void* value;
    struct hash_node_s* next;
} hash_node_s;

struct hash_map_s {
    int size;
    hash_node_s** buckets;
};


struct hash_map_s* createHashMap(int size) {
    struct hash_map_s* hashMap = (struct hash_map_s*)malloc(sizeof(struct hash_map_s));
    if (hashMap == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    hashMap->size = size;
    hashMap->buckets = (hash_node_s**)malloc(sizeof(hash_node_s*) * size);
    if (hashMap->buckets == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; ++i) {
        hashMap->buckets[i] = NULL;
    }

    return hashMap;
}

int hash(struct hash_map_s* hashMap, void* key) {
    if (key == NULL) {
        fprintf(stderr, "Invalid key.\n");
        exit(EXIT_FAILURE);
    }

    if (sizeof(key) == sizeof(uint32_t)) {
        uint32_t uint_key = *((uint32_t*)key);
        return uint_key % hashMap->size;
    } else if (sizeof(key) == sizeof(char*)) {
        char* str_key = (char*)key;
        unsigned long hashValue = 0;
        int len = strlen(str_key);
        for (int i = 0; i < len; ++i) {
            hashValue = (hashValue << 5) + str_key[i];
        }
        return hashValue % hashMap->size;
    } else {
        fprintf(stderr, "Unsupported key type.\n");
        exit(EXIT_FAILURE);
    }
}

void put(struct hash_map_s* hashMap, void* key, void* value) {
    int index = hash(hashMap, key);

    hash_node_s* newNode = (hash_node_s*)malloc(sizeof(hash_node_s));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    newNode->key = key;
    newNode->value = value;
    newNode->next = NULL;

    if (hashMap->buckets[index] == NULL) {
        hashMap->buckets[index] = newNode;
    } else {
        hash_node_s* temp = hashMap->buckets[index];
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newNode;
    }
}

char* get(struct hash_map_s* hashMap, void* key) {
    int index = hash(hashMap, key);

    hash_node_s* temp = hashMap->buckets[index];
    while (temp != NULL) {
        if (temp->key == key) {
            return temp->value;
        }
        temp = temp->next;
    }

    return NULL;
}

void freeHashMap(struct hash_map_s* hashMap) {
    if (hashMap == NULL) {
        return;
    }
    for (int i = 0; i < hashMap->size; ++i) {
        hash_node_s* node = hashMap->buckets[i];
        while (node != NULL) {
            hash_node_s* temp = node;
            node = node->next;
            free(temp);
        }
    }
    free(hashMap->buckets);
    free(hashMap);
}