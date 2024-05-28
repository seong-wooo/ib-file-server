#ifndef TCP_IP_H
#define TCP_IP_H

#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define SERVER_IP "10.10.10.13"

typedef int socket_t;

socket_t create_server_socket(int port);
socket_t accept_socket(socket_t sock);
void close_socket(socket_t sock);
#endif