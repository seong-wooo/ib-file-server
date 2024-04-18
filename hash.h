#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct hash_node_s {
    char* key;
    char* value;
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

int hash(struct hash_map_s* hashMap, char* key) {
    unsigned long hashValue = 0;
    int len = strlen(key);
    for (int i = 0; i < len; ++i) {
        hashValue = (hashValue << 5) + key[i];
    }
    return hashValue % hashMap->size;
}

char* strdup(const char* str) {
    size_t len = strlen(str) + 1;
    char* new_str = (char*)malloc(len);
    if (new_str == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(new_str, str);
    return new_str;
}

void put(struct hash_map_s* hashMap, char* key, char* value) {
    int index = hash(hashMap, key);

    hash_node_s* newNode = (hash_node_s*)malloc(sizeof(hash_node_s));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    newNode->key = strdup(key);
    newNode->value = strdup(value);
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

char* get(struct hash_map_s* hashMap, char* key) {
    int index = hash(hashMap, key);

    hash_node_s* temp = hashMap->buckets[index];
    while (temp != NULL) {
        if (strcmp(temp->key, key) == 0) {
            return temp->value;
        }
        temp = temp->next;
    }

    return NULL;
}

void freeHashMap(struct hash_map_s* hashMap) {
    for (int i = 0; i < hashMap->size; ++i) {
        hash_node_s* node = hashMap->buckets[i];
        while (node != NULL) {
            hash_node_s* temp = node;
            node = node->next;
            free(temp->key);
            free(temp->value);
            free(temp);
        }
    }
    free(hashMap->buckets);
    free(hashMap);
}