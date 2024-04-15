#include "Common.h"

char *SERVERIP = (char *) "192.1.1.207";

#define MAX_WR 10
#define CQ_SIZE 10
#define IB_PORT 1

void connect_to_server(SOCKET sock, char* ip, int port);

int main()
{
    struct ibv_device **device_list;
    struct ibv_device *ib_device;

    struct ibv_context *ctx = NULL;
    struct ibv_pd *pd = NULL;
    struct ibv_cq *cq = NULL;
    struct ibv_qp *qp = NULL;
    
    struct ibv_qp_init_attr qp_init_attr;
    struct ibv_qp_attr qp_attr;
    struct ibv_qp_attr rtr_attr;
    struct ibv_qp_attr rts_attr;
    struct ibv_mr *mr;
    void *buffer;
    size_t buffer_size = 10000;

    char buf[BUFSIZE];

    // 사용 가능한 InfiniBand 디바이스 목록 가져오기
    device_list = ibv_get_device_list(NULL);
    if (!device_list)
    {
        perror("ibv_get_device_list");
        exit(EXIT_FAILURE);
    }

    // 첫 번째 디바이스 선택
    ib_device = device_list[0];
    printf("ibv_get_device_name: %s\n", ibv_get_device_name(ib_device));

    // InfiniBand 디바이스 열기
    ctx = ibv_open_device(ib_device);
    if (!ctx)
    {
        perror("ibv_open_device");
        exit(EXIT_FAILURE);
    }
    printf("'%s' 장치가 열렸습니다. \n ", ibv_get_device_name(ctx->device));

    // Completion Queue 생성
    cq = ibv_create_cq(ctx, CQ_SIZE, NULL, NULL, 0);
    if (!cq)
    {
        perror("ibv_create_cq");
        exit(EXIT_FAILURE);
    }
    printf("Completion Queue가 생성되었습니다.\n");

    // Protection Domain 생성
    pd = ibv_alloc_pd(ctx);
    if (!pd)
    {
        perror("ibv_alloc_pd");
        exit(EXIT_FAILURE);
    }
    printf("Protection Domain이 생성되었습니다.\n");

    buffer = malloc(buffer_size);
    if (!buffer)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    mr = ibv_reg_mr(pd, buffer, buffer_size, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    if (!mr)
    {
        perror("ibv_reg_mr");
        exit(EXIT_FAILURE);
    }
    printf("Memory Region이 생성되었습니다.\n");

    // Queue Pair 초기화 속성 설정
    memset(&qp_init_attr, 0, sizeof(qp_init_attr));
    qp_init_attr.send_cq = cq;    // Send 작업의 완료를 처리할 Completion Queue 지정
    qp_init_attr.recv_cq = cq;    // Receive 작업의 완료를 처리할 Completion Queue 지정
    qp_init_attr.cap.max_send_wr = MAX_WR; // 최대 Send Work Request 수
    qp_init_attr.cap.max_recv_wr = MAX_WR; // 최대 Receive Work Request 수
    qp_init_attr.cap.max_send_sge = 1;     // 한 번의 Send에 사용할 Scatter/Gather 엔트리 수
    qp_init_attr.cap.max_recv_sge = 1;     // 한 번의 Receive에 사용할 Scatter/Gather 엔트리 수
    qp_init_attr.sq_sig_all = 1;           // Send Queue의 모든 Work Request에 대해 완료 신호를 받음
    qp_init_attr.qp_type = IBV_QPT_RC;     // Queue Pair 유형 : RC

    // Queue Pair 생성
    qp = ibv_create_qp(pd, &qp_init_attr);
    if (!qp)
    {
        perror("ibv_create_qp");
        printf("Failed to create Queue Pair.\n");
        exit(EXIT_FAILURE);
    }
    printf("Queue Pair가 생성되었습니다.\n");

    qp_attr.qp_state = IBV_QPS_INIT;
    qp_attr.pkey_index = 0;
    qp_attr.port_num = IB_PORT;
    qp_attr.qp_access_flags = IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
    int ret = ibv_modify_qp(qp, &qp_attr, attr_mask);
    if (ret) {
        perror("ibv_modify_qp");
        exit(EXIT_FAILURE);
    }
    printf("Queue Pair 상태를 IBV_QPS_INIT으로 변경하였습니다.\n");


    struct ibv_port_attr port_attr;
    ret = ibv_query_port(ctx, IB_PORT, &port_attr);
    if (ret) {
        perror("ibv_query_port");
        exit(EXIT_FAILURE);
    } 
    printf("qp_num = %u\n", qp->qp_num);
    printf("lid = %u\n", port_attr.lid);

    SOCKET sock = create_socket();
    connect_to_server(sock, SERVERIP, SERVERPORT);
    sprintf(buf, "%s%s%c%s%s%s%u%s%s%s%u", OPTION, TOKEN_PARSER, RC, LINE_PARSER, QP_NUM, TOKEN_PARSER, qp->qp_num, LINE_PARSER, LID, TOKEN_PARSER, port_attr.lid);
    printf("buf: %s\n", buf);
    int retval = send(sock, buf, BUFSIZE, MSG_WAITALL);
    if (retval == SOCKET_ERROR) {
        err_display("send()");
    }



    retval = recv(sock, buf, BUFSIZE, MSG_WAITALL);
    if (retval == SOCKET_ERROR) {
        err_display("recv()");
    }
    buf[retval] = '\0';

    printf("buf: %s\n", buf);
    HashMap* hashMap = parse_message(HASH_SIZE, buf);

    uint32_t server_qp_num = atoi(get(hashMap, QP_NUM));
    uint16_t server_lid = atoi(get(hashMap, LID));

    memset(&rtr_attr, 0, sizeof(rtr_attr));
    rtr_attr.qp_state = IBV_QPS_RTR;
    rtr_attr.path_mtu = IBV_MTU_1024;
    rtr_attr.dest_qp_num = server_qp_num;
    rtr_attr.ah_attr.dlid = server_lid;
    rtr_attr.ah_attr.port_num = IB_PORT;
    attr_mask = IBV_QP_STATE|IBV_QP_AV|IBV_QP_PATH_MTU|IBV_QP_DEST_QPN|IBV_QP_RQ_PSN|IBV_QP_MAX_DEST_RD_ATOMIC|IBV_QP_MIN_RNR_TIMER;
    retval = ibv_modify_qp(qp, &rtr_attr, attr_mask);
    if (retval) {
        perror("ibv_modify_qp (RTR))");
        exit(EXIT_FAILURE);
    }
    printf("Queue Pair 상태를 IBV_QPS_RTR으로 변경하였습니다.\n");

    memset(&rtr_attr, 0, sizeof(rtr_attr));
    rts_attr.qp_state = IBV_QPS_RTS;
    rts_attr.retry_cnt = 7;
    rts_attr.rnr_retry = 7;
    attr_mask = IBV_QP_STATE|IBV_QP_TIMEOUT|IBV_QP_RETRY_CNT|IBV_QP_RNR_RETRY|IBV_QP_SQ_PSN|IBV_QP_MAX_QP_RD_ATOMIC;
    if (retval) {
        perror("ibv_modify_qp (RTS))");
        exit(EXIT_FAILURE);
    }
    printf("Queue Pair 상태를 IBV_QPS_RTS으로 변경하였습니다.\n");


    

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
