#include <infiniband/verbs.h>
#include "tcp_ip.h"

#define IB_PORT 1
#define MR_BUFSIZE 4096
#define MAX_WR 10

#define MSG "hello"
#define MSG_SIZE (strlen(MSG) + 1)

struct connection_data
{
    uint64_t addr;
    uint32_t rkey;
    uint32_t qp_num;
    uint16_t lid;
};

struct resources
{
    struct ibv_device **device_list;
    struct ibv_port_attr *port_attr;
    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_mr *mr;
    struct ibv_qp *qp;
    char *buffer;
    SOCKET sock;
    struct connection_data remote_props;
};

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

struct ibv_cq *create_ibv_cq(struct ibv_context *ctx)
{
    struct ibv_cq *cq = ibv_create_cq(ctx, 100, NULL, NULL, 0);
    if (!cq)
    {
        perror("ibv_create_cq");
        exit(EXIT_FAILURE);
    }
    return cq;
}

char *create_buffer(size_t buffer_size)
{
    char *buffer = (char *)malloc(buffer_size);
    memset(buffer, 0, buffer_size);
    if (!buffer)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return buffer;
}

struct ibv_mr *create_ibv_mr(struct resources *res)
{
    struct ibv_mr *mr = ibv_reg_mr(res->pd, res->buffer, sizeof(res->buffer), IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    if (!mr)
    {
        perror("ibv_reg_mr");
        exit(EXIT_FAILURE);
    }
    return mr;
}

struct ibv_port_attr *create_port_attr(struct ibv_context *ctx)
{
    struct ibv_port_attr *port_attr = (struct ibv_port_attr *)(malloc(sizeof(struct ibv_port_attr)));
    if (ibv_query_port(ctx, IB_PORT, port_attr))
    {
        perror("ibv_query_port");
        exit(EXIT_FAILURE);
    }
    return port_attr;
}

struct ibv_qp *create_ibv_qp(struct resources *res)
{
    struct ibv_qp_init_attr qp_init_attr = {
        .qp_type = IBV_QPT_RC,
        .sq_sig_all = 1,
        .send_cq = res->cq,
        .recv_cq = res->cq,
        .cap = {
            .max_send_wr = MAX_WR,
            .max_recv_wr = MAX_WR,
            .max_send_sge = 1,
            .max_recv_sge = 1,
        },
    };
    struct ibv_qp *qp = ibv_create_qp(res->pd, &qp_init_attr);
    if (!qp)
    {
        perror("ibv_create_qp");
        exit(EXIT_FAILURE);
    }

    return qp;
}

void init_resources(struct resources *res)
{
    memset(res, 0, sizeof(res));
    res->sock = -1;
}

void create_resources(struct resources *res)
{
    init_resources(res);
    res->device_list = create_device_list();
    res->ctx = create_ibv_context(res->device_list);
    res->pd = create_ibv_pd(res->ctx);
    res->cq = create_ibv_cq(res->ctx);
    res->buffer = create_buffer(MR_BUFSIZE);
    res->mr = create_ibv_mr(res);
    res->port_attr = create_port_attr(res->ctx);
    res->qp = create_ibv_qp(res);
}

void modify_qp_to_init(struct resources *res)
{
    struct ibv_qp_attr qp_attr;
    qp_attr.qp_state = IBV_QPS_INIT;
    qp_attr.pkey_index = 0;
    qp_attr.port_num = IB_PORT;
    qp_attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
    int ret = ibv_modify_qp(res->qp, &qp_attr, attr_mask);
    if (ret)
    {
        perror("ibv_modify_qp");
        exit(EXIT_FAILURE);
    }
}

void modify_qp_to_rtr(struct resources *res)
{
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = IBV_MTU_4096;
    attr.dest_qp_num = res->remote_props.qp_num;
    attr.rq_psn = 0;
    attr.max_dest_rd_atomic = 1;
    attr.min_rnr_timer = 12;
    attr.ah_attr.is_global = 0;
    attr.ah_attr.dlid = res->remote_props.lid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = IB_PORT;

    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
    int retval = ibv_modify_qp(res->qp, &attr, attr_mask);
    if (retval)
    {
        perror("ibv_modify_qp (RTR)");
        exit(EXIT_FAILURE);
    }
}

void modify_qp_to_rts(struct resources *res)
{
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTS;
    attr.timeout = 12;
    attr.retry_cnt = 0;
    attr.rnr_retry = 0;
    attr.sq_psn = 0;
    attr.max_rd_atomic = 1;

    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
    int retval = ibv_modify_qp(res->qp, &attr, attr_mask);
    if (retval)
    {
        perror("ibv_modify_qp (RTS)");
        exit(EXIT_FAILURE);
    }
}


void send_qp_sync_data(struct resources *res)
{
    struct connection_data conn_data = {
        .addr = (uint64_t)(uintptr_t)res->buffer,
        .rkey = res->mr->rkey,
        .qp_num = res->qp->qp_num,
        .lid = res->port_attr->lid,
    };

    if (send(res->sock, &conn_data, sizeof(conn_data), MSG_WAITALL) == SOCKET_ERROR)
    {
        perror("send");
        exit(EXIT_FAILURE);
    }
}

int recv_qp_sync_data(struct resources *res)
{
    int retval;
    struct connection_data remote_data;
    retval = recv(res->sock, &remote_data, sizeof(remote_data), MSG_WAITALL);
    if (retval == SOCKET_ERROR)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    res->remote_props = remote_data;
    return retval;
}

void connect_ib_server(struct resources *res)
{
    create_resources(res);
    res->sock = create_socket();
    connect_tcp_to_server(res->sock, SERVERIP, SERVERPORT);
    send_qp_sync_data(res);
    recv_qp_sync_data(res);
    modify_qp_to_init(res);
    modify_qp_to_rtr(res);
    modify_qp_to_rts(res);
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

void accept_ib_client(SOCKET sock, struct resources *res)
{
    create_resources(res);

    struct sockaddr_in client_addr;
    SOCKET client_sock = accept_socket(sock, &client_addr);
    res->sock = client_sock;
    if (recv_qp_sync_data(res) < 0)
    {
        exit(EXIT_FAILURE);
    }
    modify_qp_to_init(res);
    modify_qp_to_rtr(res);
    modify_qp_to_rts(res);
    
    send_qp_sync_data(res);
}

void post_receive(struct resources *res)
{
    struct ibv_sge sge;
    struct ibv_recv_wr recv_wr, *bad_wr;

    memset(&sge, 0, sizeof(sge));
    sge.addr = (uintptr_t)res->buffer;
    sge.length = MSG_SIZE;
    sge.lkey = res->mr->lkey;

    memset(&recv_wr, 0, sizeof(recv_wr));
    recv_wr.next = NULL;
    recv_wr.wr_id = 1;
    recv_wr.sg_list = &sge;
    recv_wr.num_sge = 1;

    int retval = ibv_post_recv(res->qp, &recv_wr, &bad_wr);
    if (retval)
    {
        perror("ibv_post_recv");
        exit(EXIT_FAILURE);
    }
    printf("post recv wr\n");
}

void post_send(struct resources *res, int opcode)
{
    struct ibv_sge sge;
    struct ibv_send_wr send_wr, *bad_wr;
    memset(&sge, 0, sizeof(sge));
    sge.addr = (uintptr_t)res->buffer;
    sge.length = MSG_SIZE;
    sge.lkey = res->mr->lkey;

    memset(&send_wr, 0, sizeof(send_wr));
    send_wr.wr_id = 0;
    send_wr.next = NULL;
    send_wr.sg_list = &sge;
    send_wr.num_sge = 1;
    send_wr.opcode = opcode;
    send_wr.send_flags = IBV_SEND_SIGNALED;

    if (ibv_post_send(res->qp, &send_wr, &bad_wr))
    {
        perror("ibv_post_send");
        exit(EXIT_FAILURE);
    }
    printf("post send wr\n");
}

void destroy_qp(struct ibv_qp *qp)
{
    if (qp)
    {
        if (ibv_destroy_qp(qp))
        {
            perror("ibv_destroy_qp");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_mr(struct ibv_mr *mr)
{
    if (mr)
    {
        if (ibv_dereg_mr(mr))
        {
            perror("ibv_dereg_mr");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_cq(struct ibv_cq *cq)
{
    if (cq)
    {
        if (ibv_destroy_cq(cq))
        {
            perror("ibv_destroy_cq");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_pd(struct ibv_pd *pd)
{
    if (pd)
    {
        if (ibv_dealloc_pd(pd))
        {
            perror("ibv_dealloc_pd");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_ctx(struct ibv_context *ctx)
{
    if (ctx)
    {
        if (ibv_close_device(ctx))
        {
            perror("ibv_close_device");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_device_list(struct resources *res)
{
    if (res->device_list)
    {
        ibv_free_device_list(res->device_list);
    }
}

void free_buffer(void *buffer) {
    if (buffer) {
        free(buffer);
    }
}

void close_socket(SOCKET sock) {
    if (sock != INVALID_SOCKET) {
        print_disconnected_client(sock);
        close(sock);
    }
}

void destroy_resources(struct resources *res)
{
    destroy_qp(res->qp);
    destroy_mr(res->mr);
    destroy_cq(res->cq);
    destroy_pd(res->pd);
    destroy_ctx(res->ctx);
    destroy_device_list(res);
    free_buffer(res->buffer);
    close_socket(res->sock);
}

void poll_completion(struct resources *res)
{
    int poll_result;
    struct ibv_wc wc;
    do
    {
        poll_result = ibv_poll_cq(res->cq, 1, &wc);
    } while (poll_result == 0);
    if (poll_result < 0)
    {
        perror("ibv_poll_cq");
        exit(EXIT_FAILURE);
    }
    if (wc.status != IBV_WC_SUCCESS)
    {
        printf("Completion error: %s\n", ibv_wc_status_str(wc.status));
        exit(EXIT_FAILURE);
    }

    printf("poll recv wc: wr_id=0x%x\n", wc.wr_id);
    printf("poll recv message : %s\n", res->buffer);
}