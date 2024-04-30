#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#define READ 'r'
#define WRITE 'w'
#define DELETE 'd'
#define QUIT 'q'
#define LIST 'l'

#define MESSAGE_SIZE 4096

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

char get_option(void);
struct packet_s *create_request_packet(char option);
void serialize_packet(struct packet_s *packet, void *buffer);
struct packet_s *deserialize_packet(void *buffer);
void free_packet(struct packet_s *packet);
#endif