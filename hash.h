#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct HashNode {
    char* key;
    char* value;
    struct HashNode* next;
} HashNode;

typedef struct HashMap {
    int size;
    HashNode** buckets;
} HashMap;

HashMap* createHashMap(int size) {
    HashMap* hashMap = (HashMap*)malloc(sizeof(HashMap));
    if (hashMap == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    hashMap->size = size;
    hashMap->buckets = (HashNode**)malloc(sizeof(HashNode*) * size);
    if (hashMap->buckets == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < size; ++i) {
        hashMap->buckets[i] = NULL;
    }

    return hashMap;
}

int hash(HashMap* hashMap, char* key) {
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

void put(HashMap* hashMap, char* key, char* value) {
    int index = hash(hashMap, key);

    HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
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
        HashNode* temp = hashMap->buckets[index];
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newNode;
    }
}

char* get(HashMap* hashMap, char* key) {
    int index = hash(hashMap, key);

    HashNode* temp = hashMap->buckets[index];
    while (temp != NULL) {
        if (strcmp(temp->key, key) == 0) {
            return temp->value;
        }
        temp = temp->next;
    }

    return NULL;
}

void freeHashMap(HashMap* hashMap) {
    for (int i = 0; i < hashMap->size; ++i) {
        HashNode* node = hashMap->buckets[i];
        while (node != NULL) {
            HashNode* temp = node;
            node = node->next;
            free(temp->key);
            free(temp->value);
            free(temp);
        }
    }
    free(hashMap->buckets);
    free(hashMap);
}