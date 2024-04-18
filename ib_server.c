#include "ib.h"

void bind_socket(SOCKET sock, int port);
void listen_socket(SOCKET sock, int max_connection);
SOCKET accept_socket(SOCKET sock, struct sockaddr_in *client_addr);
void print_connected_client(struct sockaddr_in *client_addr);
void print_disconnected_client(struct sockaddr_in *client_addr);

int main(int argc, char const *argv[])
{
    struct resources res;
    create_resources(&res);
    bind_socket(res.sock, SERVERPORT);
    listen_socket(res.sock, SOMAXCONN);

    int retval;

    SOCKET client_sock;
    struct sockaddr_in client_addr;

    while (1)
    {
        client_sock = accept_socket(res.sock, &client_addr);
        if (recv_qp_sync_data(client_sock, &res) == 0)
        {
            break;
        }
        modify_qp_to_init(&res);
        modify_qp_to_rtr(&res);
        modify_qp_to_rts(&res);
        post_receive(&res);

        send_qp_sync_data(client_sock, &res);

        poll_completion(&res);

        close(client_sock);
        print_disconnected_client(&client_addr);
    }

    destroy_resources(&res);
    printf("[TCP 서버] 서버 종료\n");

    return 0;
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

SOCKET accept_socket(SOCKET sock, struct sockaddr_in *client_addr)
{
    SOCKET client_sock;
    int addrlen = sizeof(client_addr);

    client_sock = accept(sock, (struct sockaddr *)client_addr, &addrlen);
    if (client_sock == INVALID_SOCKET)
    {
        err_display("accept()");
    }
    print_connected_client(client_addr);
    return client_sock;
}

void print_connected_client(struct sockaddr_in *client_addr)
{
    char client_ip[20];
    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip));
    printf("[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", client_ip, ntohs(client_addr->sin_port));
}

void print_disconnected_client(struct sockaddr_in *client_addr)
{
    char client_ip[20];
    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip));
    printf("[TCP 서버] 클라이언트 접속 종료: IP 주소=%s, 포트 번호=%d\n", client_ip, ntohs(client_addr->sin_port));
}