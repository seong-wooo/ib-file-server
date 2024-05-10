#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <time.h>
#include "tcp_client.h"

void send_packet(socket_t sock, struct packet_s *packet, void *buffer) {
    serialize_packet(packet, buffer);
    int rc = send(sock, buffer, sizeof(struct packet_header_s) + packet->header.body_size, 0);
    if (rc == SOCKET_ERROR) {
        perror("send()");
        exit(EXIT_FAILURE);
    }
}

struct packet_s *recv_packet(socket_t sock, void *buffer) {
    int rc = recv(sock, buffer, MESSAGE_SIZE, 0);
    if (rc == SOCKET_ERROR) {
        perror("recv()");
        exit(EXIT_FAILURE);
    }

    return deserialize_packet(buffer);
}

void tcp_client(void) {
    socket_t sock = create_socket();
    connect_tcp_to_server(sock, SERVER_IP, TCP_SERVER_PORT);
    void *buffer = malloc(MESSAGE_SIZE);

    while(1) {
        char option = get_option();
        if (option == QUIT) {
            break;
        }
        struct packet_s *request_packet = create_request_packet(option);
        send_packet(sock, request_packet, buffer);
        
        struct packet_s *response_packet = recv_packet(sock, buffer);
        printf("[받은 데이터]:\n%s\n", response_packet->body.data);
        
        free_packet(request_packet);
        free_packet(response_packet);
    }
    close_socket(sock);
}