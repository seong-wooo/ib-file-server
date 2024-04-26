#ifndef TCP_IP_H
#define TCP_IP_H

#define SERVER_PORT 9000
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define SERVER_IP "10.10.10.13"

typedef int socket_t;

socket_t create_socket(void);
void close_socket(socket_t sock);
void connect_tcp_to_server(socket_t sock, char *ip, int port);
#endif