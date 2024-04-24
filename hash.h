#ifndef HASH_H
#define HASH_H
struct hash_node_s {
    void* key;
    void* value;
    struct hash_node_s* next;
};

struct hash_map_s {
    int size;
    struct hash_node_s** buckets;
};

struct hash_map_s* create_hash_map(int size);
void put(struct hash_map_s* hash_map, void* key, void* value);
void* get(struct hash_map_s* hash_map, void* key);
void free_hash_map(struct hash_map_s* hash_map);
#endif