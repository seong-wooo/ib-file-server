#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include "tcp_ip.h"
#include "err_check.h"

socket_t create_socket(void) {
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    check_error(sock, "socket()");

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

void send_packet(socket_t sock, struct packet_s *packet) {
    struct iovec iov[2];
    iov[0].iov_base = &packet->header;
    iov[0].iov_len = sizeof(struct packet_header_s);
    iov[1].iov_base = packet->body.data;
    iov[1].iov_len = packet->header.body_size;

    int rc = writev(sock, iov, 2);
    check_error(rc, "writev()");
}

void recv_packet(socket_t sock, struct packet_s *packet) {
    int body_size = MESSAGE_SIZE - sizeof(struct packet_header_s);

    struct iovec iov[2];
    iov[0].iov_base = &packet->header;
    iov[0].iov_len = sizeof(struct packet_header_s);
    iov[1].iov_base = packet->body.data;
    iov[1].iov_len = body_size;

    int rc = readv(sock, iov, 2);
    check_error(rc, "readv()");
    packet->body.data[packet->header.body_size] = '\0';
}
