#ifndef TCP_IP_H
#define TCP_IP_H

#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define SERVER_IP "10.10.10.13"
#define TCP_SERVER_PORT 9000

typedef int socket_t;

socket_t create_socket(void);
void bind_socket(socket_t sock, int port);
void listen_socket(socket_t sock, int max_connection);
void close_socket(socket_t sock);
socket_t accept_socket(socket_t sock);
socket_t connect_tcp_to_server(char *ip, int port);
#endif