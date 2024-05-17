#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include "tcp_ip.h"
#include "err_check.h"

socket_t create_socket(void) {
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    return sock;
}

void close_socket(socket_t sock) {
    if (sock != INVALID_SOCKET) {
        close(sock);
    }
}

socket_t connect_tcp_to_server(char *ip, int port) {
    int rc;
    struct sockaddr_in serveraddr;
    socket_t sock = create_socket();

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(port);
    rc = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    check_error(rc, "connect()");

    return sock;
}

void send_packet(socket_t sock, struct packet_s *packet, void *buffer) {
    serialize_packet(packet, buffer);
    int rc = send(sock, buffer, sizeof(struct packet_header_s) + packet->header.body_size, 0);
    check_error(rc, "send()");
}

struct packet_s *recv_packet(socket_t sock, void *buffer) {
    int rc = recv(sock, buffer, MESSAGE_SIZE, 0);
    check_error(rc, "recv()");

    return deserialize_packet(buffer);
}
