#include "Common.h"
#include "ib.h"

char *SERVERIP = (char *) "192.1.1.207";

#define MAX_WR 10
#define CQ_SIZE 10
#define IB_PORT 1

void connect_to_server(SOCKET sock, char* ip, int port);

int main()
{
    struct ibv_device **device_list = create_device_list();
    struct ibv_context *ctx = create_ibv_context(device_list);
    struct ibv_pd *pd = create_ibv_pd(ctx);
    struct ibv_cq *cq = create_ibv_cq(ctx);
    struct ibv_mr *mr = create_ibv_mr(pd, 10000);
    struct ibv_qp *qp = create_ibv_qp_reset(ctx, pd, cq);
    modify_qp_to_init(qp);
    QP_INFO *qp_info = get_qp_info(qp);

    struct ibv_qp_attr rtr_attr;
    struct ibv_qp_attr rts_attr;
    char buf[BUFSIZE];


    SOCKET sock = create_socket();
    connect_to_server(sock, SERVERIP, SERVERPORT);
    sprintf(buf, "%s%s%c%s%s%s%u%s%s%s%u", OPTION, TOKEN_PARSER, RC, LINE_PARSER, QP_NUM, TOKEN_PARSER, qp_info->qp_num, LINE_PARSER, LID, TOKEN_PARSER, qp_info->lid);
    int retval = send(sock, buf, BUFSIZE, MSG_WAITALL);
    if (retval == SOCKET_ERROR) {
        err_display("send()");
    }

    retval = recv(sock, buf, BUFSIZE, MSG_WAITALL);
    if (retval == SOCKET_ERROR) {
        err_display("recv()");
    }
    buf[retval] = '\0';

    HashMap* hashMap = parse_message(HASH_SIZE, buf);

    uint32_t server_qp_num = atoi(get(hashMap, QP_NUM));
    uint16_t server_lid = atoi(get(hashMap, LID));

    modify_qp_to_rtr(qp, server_qp_num, server_lid);
    modify_qp_to_rts(qp);


    // 자원 해제
    if (ibv_destroy_qp(qp)) {
        perror("ibv_destroy_qp");
        exit(EXIT_FAILURE);
    }

    if (ibv_dereg_mr(mr)) {
        perror("ibv_dereg_mr");
        exit(EXIT_FAILURE);
    }

    if (ibv_destroy_cq(cq))
    {
        perror("ibv_destroy_cq");
        exit(EXIT_FAILURE);
    }

    if (ibv_dealloc_pd(pd))
    {
        perror("ibv_dealloc_pd");
        exit(EXIT_FAILURE);
    }

    if (ibv_close_device(ctx))
    {
        perror("ibv_close_device");
        exit(EXIT_FAILURE);
    }

    ibv_free_device_list(device_list);

    return 0;
}

void connect_to_server(SOCKET sock, char* ip, int port) {
    int retval;
    struct sockaddr_in serveraddr;

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(port);
    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");
}