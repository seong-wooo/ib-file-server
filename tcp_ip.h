#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVERPORT 9000
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1

#define SERVERPORT 9000

typedef int SOCKET;
char *SERVERIP = (char *) "192.1.1.207";

SOCKET create_socket(void)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        err_quit("socket()");
    }
    return sock;
}

void bind_socket(SOCKET sock, int port)
{
    int retval;
    struct sockaddr_in serveraddr;

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    retval = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR)
        err_quit("bind()");
}

void listen_socket(SOCKET sock, int max_connection)
{
    int retval;

    retval = listen(sock, max_connection);
    if (retval == SOCKET_ERROR)
    {
        err_quit("listen()");
    }
}

SOCKET create_server_socket(void)
{
    SOCKET sock = create_socket();
    bind_socket(sock, SERVERPORT);
    listen_socket(sock, SOMAXCONN);

    return sock;
}

void connect_tcp_to_server(SOCKET sock, char *ip, int port)
{
    int retval;
    struct sockaddr_in serveraddr;

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(port);
    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR)
        err_quit("connect()");
}

void print_connected_client(struct sockaddr_in *client_addr)
{
    char client_ip[20];
    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip));
    printf("[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", client_ip, ntohs(client_addr->sin_port));
}

void print_disconnected_client(SOCKET sock)
{
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if (getpeername(sock, (struct sockaddr *)&client_addr, &addr_len) == -1)
    {
        perror("getpeername");
        exit(EXIT_FAILURE);
    }

    char client_ip[20];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    printf("[TCP 서버] 클라이언트 접속 종료: IP 주소=%s, 포트 번호=%d\n", client_ip, ntohs(client_addr.sin_port));
}