#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include "tcp_ip.h"

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

void connect_tcp_to_server(socket_t sock, char *ip, int port) {
    int rc;
    struct sockaddr_in serveraddr;

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(port);
    rc = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (rc == SOCKET_ERROR) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }
}