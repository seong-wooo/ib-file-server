#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 9000
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1

typedef int socket_t;
char *SERVER_IP = (char *) "10.10.10.13";

void print_connected_client(struct sockaddr_in *client_addr)
{
    char client_ip[20];
    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip));
    printf("[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", client_ip, ntohs(client_addr->sin_port));
}

void print_disconnected_client(socket_t sock)
{
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if (getpeername(sock, (struct sockaddr *)&client_addr, &addr_len) == -1) {
        perror("getpeername");
        exit(EXIT_FAILURE);
    }

    char client_ip[20];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    printf("[TCP 서버] 클라이언트 접속 종료: IP 주소=%s, 포트 번호=%d\n", client_ip, ntohs(client_addr.sin_port));
}

socket_t create_socket(void)
{
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
        err_quit("socket()");

    return sock;
}

socket_t accept_socket(socket_t sock, struct sockaddr_in *client_addr) {
    socket_t client_sock;
    int addrlen = sizeof(client_addr);

    client_sock = accept(sock, (struct sockaddr *)client_addr, &addrlen);
    if (client_sock == INVALID_SOCKET) {
        err_display("accept()");
    }
    print_connected_client(client_addr);
    return client_sock;
}

void bind_socket(socket_t sock)
{
    int rc;
    struct sockaddr_in serveraddr;

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVER_PORT);

    rc = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (rc == SOCKET_ERROR)
        err_quit("bind()");
}

void listen_socket(socket_t sock, int max_connection)
{
    int rc;

    rc = listen(sock, max_connection);
    if (rc == SOCKET_ERROR)
    {
        err_quit("listen()");
    }
}

socket_t create_server_socket(void)
{
    socket_t sock = create_socket();
    bind_socket(sock);
    listen_socket(sock, SOMAXCONN);

    return sock;
}

void connect_tcp_to_server(socket_t sock, char *ip, int port)
{
    int rc;
    struct sockaddr_in serveraddr;

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(port);
    rc = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (rc == SOCKET_ERROR)
        err_quit("connect()");
}