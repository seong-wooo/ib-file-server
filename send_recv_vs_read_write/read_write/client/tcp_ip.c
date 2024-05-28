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