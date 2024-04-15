#include "Common.h"
#include "ib.h"


void bind_socket(SOCKET sock, int port);
void listen_socket(SOCKET sock, int maxconn);
SOCKET accept_socket(SOCKET sock, struct sockaddr_in* client_addr);
void print_connected_client(struct sockaddr_in* client_addr);
void print_disconnected_client(struct sockaddr_in* client_addr);

int main(int argc, char const *argv[])
{
    int retval;
    
    SOCKET server_sock = create_socket();
    bind_socket(server_sock, SERVERPORT);
    listen_socket(server_sock, SOMAXCONN);

    SOCKET client_sock;
    struct sockaddr_in client_addr;
    char buf[BUFSIZE];
    FILE* fp;

    while (1) {
        client_sock = accept_socket(server_sock, &client_addr);
        print_connected_client(&client_addr);

        while(1) {
            retval = recv(client_sock, buf, BUFSIZE, MSG_WAITALL);
            if (retval == SOCKET_ERROR) {
                err_display("recv()");
                break;
            }
            
            else if (retval == 0) {
                break;
            }

            buf[retval] = '\0';

            printf("buf: %s\n", buf);
            HashMap* hashMap = parse_message(HASH_SIZE, buf);

            char* option = get(hashMap, OPTION);

            if (*option == RC) {
                uint32_t client_qp_num = atoi(get(hashMap, QP_NUM));
                uint16_t client_lid = atoi(get(hashMap, LID));


                struct ibv_device **device_list = create_device_list();
                struct ibv_context *ctx = create_ibv_context(device_list);
                struct ibv_pd *pd = create_ibv_pd(ctx);
                struct ibv_cq *cq = create_ibv_cq(ctx);
                struct ibv_mr *mr = create_ibv_mr(pd, 10000);
                struct ibv_qp *qp = create_ibv_qp_reset(ctx, pd, cq);
                modify_qp_to_init(qp);
                modify_qp_to_rtr(qp, client_qp_num, client_lid);
                modify_qp_to_rts(qp);
                QP_INFO *qp_info = get_qp_info(qp);
                
                sprintf(buf, "%s%s%c%s%s%s%u%s%s%s%u", OPTION, TOKEN_PARSER, RC, LINE_PARSER, QP_NUM, TOKEN_PARSER, qp_info->qp_num, LINE_PARSER, LID, TOKEN_PARSER, qp_info->lid);
                int retval = send(client_sock, buf, BUFSIZE, MSG_WAITALL);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                }
            }
            freeHashMap(hashMap);
        }

        close(client_sock);
        print_disconnected_client(&client_addr);
    }
    

    close(server_sock);
    printf("[TCP 서버] 서버 종료\n");
    
    return 0;
}


void bind_socket(SOCKET sock, int port) {
    int retval;
    struct sockaddr_in serveraddr;

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);
    
    retval = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");
}

void listen_socket(SOCKET sock, int maxconn) {
    int retval;

    retval = listen(sock, maxconn);
    if (retval == SOCKET_ERROR) {
        err_quit("listen()");
    }
}

SOCKET accept_socket(SOCKET sock, struct sockaddr_in* client_addr) {
    SOCKET client_sock;
    int addrlen = sizeof(client_addr);

    client_sock = accept(sock, (struct sockaddr *) client_addr, &addrlen);
    if (client_sock == INVALID_SOCKET) {
        err_display("accept()");
    }
    
    
    return client_sock;
}

void print_connected_client(struct sockaddr_in* client_addr) {
    char client_ip[20];
    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip));
    printf("[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", client_ip, ntohs(client_addr->sin_port));
}

void print_disconnected_client(struct sockaddr_in* client_addr) {
    char client_ip[20];
    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip));
    printf("[TCP 서버] 클라이언트 접속 종료: IP 주소=%s, 포트 번호=%d\n", client_ip, ntohs(client_addr->sin_port));
}