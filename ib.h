#include <stdio.h>
#include <infiniband/verbs.h>
#include <stdlib.h>

#define IB_PORT 1
#define QP_NUM "qp_num"
#define LID "lid"

typedef struct QP_INFO
{
    uint32_t qp_num;
    uint16_t lid;
} QP_INFO;

struct ibv_device **create_device_list()
{
    struct ibv_device **device_list = ibv_get_device_list(NULL);
    if (!device_list)
    {
        perror("ibv_get_device_list");
        exit(EXIT_FAILURE);
    }
    return device_list;
}

struct ibv_context *create_ibv_context(struct ibv_device **device_list)
{
    struct ibv_context *ctx = (malloc(sizeof(struct ibv_context)));
    struct ibv_device *ib_device = device_list[0];
    ctx = ibv_open_device(ib_device);
    if (!ctx)
    {
        perror("ibv_open_device");
        exit(EXIT_FAILURE);
    }

    return ctx;
}

struct ibv_cq *create_ibv_cq(struct ibv_context *ctx)
{
    struct ibv_cq *cq = ibv_create_cq(ctx, 10, NULL, NULL, 0);
    if (!cq)
    {
        perror("ibv_create_cq");
        exit(EXIT_FAILURE);
    }
    return cq;
}

struct ibv_pd *create_ibv_pd(struct ibv_context *ctx)
{
    struct ibv_pd *pd = (struct ibv_pd *)(malloc(sizeof(struct ibv_pd)));
    pd = ibv_alloc_pd(ctx);
    if (!pd)
    {
        perror("ibv_alloc_pd");
        exit(EXIT_FAILURE);
    }
    return pd;
}

struct ibv_mr *create_ibv_mr(struct ibv_pd *pd, size_t buffer_size)
{
    void *buffer = malloc(buffer_size);
    if (!buffer)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    struct ibv_mr *mr = ibv_reg_mr(pd, buffer, buffer_size, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    if (!mr)
    {
        perror("ibv_reg_mr");
        exit(EXIT_FAILURE);
    }
    return mr;
}

struct ibv_qp *create_ibv_qp_reset(struct ibv_context *ctx, struct ibv_pd *pd, struct ibv_cq *cq)
{
    struct ibv_qp_init_attr qp_init_attr;
    memset(&qp_init_attr, 0, sizeof(qp_init_attr));
    qp_init_attr.send_cq = cq;             // Send 작업의 완료를 처리할 Completion Queue 지정
    qp_init_attr.recv_cq = cq;             // Receive 작업의 완료를 처리할 Completion Queue 지정
    qp_init_attr.cap.max_send_wr = MAX_WR; // 최대 Send Work Request 수
    qp_init_attr.cap.max_recv_wr = MAX_WR; // 최대 Receive Work Request 수
    qp_init_attr.cap.max_send_sge = 1;     // 한 번의 Send에 사용할 Scatter/Gather 엔트리 수
    qp_init_attr.cap.max_recv_sge = 1;     // 한 번의 Receive에 사용할 Scatter/Gather 엔트리 수
    qp_init_attr.sq_sig_all = 1;           // Send Queue의 모든 Work Request에 대해 완료 신호를 받음
    qp_init_attr.qp_type = IBV_QPT_RC;     // Queue Pair 유형 : RC

    struct ibv_qp *qp = ibv_create_qp(pd, &qp_init_attr);
    if (!qp)
    {
        perror("ibv_create_qp");
        printf("Failed to create Queue Pair.\n");
        exit(EXIT_FAILURE);
    }

    return qp;
}

void modify_qp_to_init(struct ibv_qp *qp)
{
    struct ibv_qp_attr qp_attr;
    qp_attr.qp_state = IBV_QPS_INIT;
    qp_attr.pkey_index = 0;
    qp_attr.port_num = IB_PORT;
    qp_attr.qp_access_flags = IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
    int ret = ibv_modify_qp(qp, &qp_attr, attr_mask);
    if (ret)
    {
        perror("ibv_modify_qp");
        exit(EXIT_FAILURE);
    }
}

QP_INFO *get_qp_info(struct ibv_qp *qp) {
    QP_INFO *qp_info = (QP_INFO *)malloc(sizeof(QP_INFO));
    qp_info->qp_num = qp->qp_num;
    struct ibv_port_attr port_attr;
    if (ibv_query_port(qp->context, IB_PORT, &port_attr)) {
        perror("ibv_query_port");
        exit(EXIT_FAILURE);
    }

    return qp_info;
}

void modify_qp_to_rtr(struct ibv_qp *qp, uint32_t server_qp_num, uint16_t server_lid) {
    struct ibv_qp_attr rtr_attr;
    memset(&rtr_attr, 0, sizeof(rtr_attr));
    rtr_attr.qp_state = IBV_QPS_RTR;
    rtr_attr.path_mtu = IBV_MTU_1024;
    rtr_attr.dest_qp_num = server_qp_num;
    rtr_attr.ah_attr.dlid = server_lid;
    rtr_attr.ah_attr.port_num = IB_PORT;
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
    int retval = ibv_modify_qp(qp, &rtr_attr, attr_mask);
    if (retval) {
        perror("ibv_modify_qp (RTR)");
        exit(EXIT_FAILURE);
    }
}

void modify_qp_to_rts(struct ibv_qp *qp) {
    struct ibv_qp_attr rts_attr;
    memset(&rts_attr, 0, sizeof(rts_attr));
    rts_attr.qp_state = IBV_QPS_RTS;
    rts_attr.retry_cnt = 7;
    rts_attr.rnr_retry = 7;
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
    int retval = ibv_modify_qp(qp, &rts_attr, attr_mask);
    if (retval) {
        perror("ibv_modify_qp (RTS)");
        exit(EXIT_FAILURE);
    }
}