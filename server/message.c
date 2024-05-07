#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "message.h"

void serialize_packet(struct packet_s *packet, void *buffer) {
    memcpy(buffer, &packet->header, sizeof(struct packet_header_s));
    memcpy(buffer + sizeof(struct packet_header_s), packet->body.data, 
        packet->header.body_size);
}

struct packet_s *deserialize_packet(void *buffer) {
    struct packet_s *packet = (struct packet_s *) malloc(sizeof(struct packet_s));
    memcpy(&packet->header, buffer, sizeof(struct packet_header_s));
    packet->body.data = (char *)malloc(packet->header.body_size);
    memcpy(packet->body.data, buffer + sizeof(struct packet_header_s), 
        packet->header.body_size);
    return packet;
}

void free_packet(struct packet_s *packet) {
    if (packet->body.data != NULL) {
        free(packet->body.data);
    }
    free(packet);
}