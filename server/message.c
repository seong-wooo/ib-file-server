#include <stdlib.h>
#include <string.h>
#include "message.h"

struct packet_s *create_response_packet(void *data) {
    struct packet_s *packet = malloc(sizeof(struct packet_s));
    memcpy(&packet->header, data, sizeof(struct packet_header_s));
    packet->body.data = (char *)malloc(packet->header.body_size);
    memcpy(packet->body.data, data + sizeof(struct packet_header_s), packet->header.body_size);
    
    return packet;
}

void free_packet(struct packet_s *packet) {
    if (packet->body.data != NULL) {
        free(packet->body.data);
    }
    free(packet);
}