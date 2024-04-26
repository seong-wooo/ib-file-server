#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#define READ 'r'
#define WRITE 'w'
#define DELETE 'd'
#define QUIT 'q'
#define LIST 'l'

struct packet_header_s {
    uint32_t qp_num;
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
struct packet_s *create_request_packet(void *data);
struct packet_s *create_response_packet(char option);
#endif