#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#define MESSAGE_SIZE 4096

#define READ 'r'
#define WRITE 'w'
#define DELETE 'd'
#define QUIT 'q'
#define LIST 'l'


struct packet_header_s {
    char option;
    char filename[20];
    int offset;
    int length;
    int body_size;
};

struct packet_body_s {
    char *data;
};

struct packet_s {
    struct packet_header_s header;
    struct packet_body_s body;
};

void serialize_packet(struct packet_s *packet, void *buffer);
struct packet_s *deserialize_packet(void *data);
void free_packet(struct packet_s *packet);
#endif