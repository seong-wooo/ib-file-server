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
    check_error(sock, "socket()");

    return sock;
}

socket_t accept_socket(socket_t sock) {
    socket_t client_sock;
    struct sockaddr_in client_addr;
    int addrlen = sizeof(client_addr);

    client_sock = accept(sock, (struct sockaddr *)&client_addr, &addrlen);
    check_error(client_sock, "accept()");
    
    return client_sock;
}

void bind_socket(socket_t sock, int port) {
    struct sockaddr_in serveraddr;

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    int rc = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    check_error(rc, "bind()");
}

void listen_socket(socket_t sock, int max_connection) {
    int rc = listen(sock, max_connection);
    check_error(rc, "listen()");
}

socket_t create_server_socket(int port) {
    socket_t sock = create_socket();
    bind_socket(sock, port);
    listen_socket(sock, SOMAXCONN);

    return sock;
}

void close_socket(socket_t sock) {
    if (sock != INVALID_SOCKET) {
        close(sock);
    }
}