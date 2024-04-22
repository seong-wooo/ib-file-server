#include <infiniband/verbs.h>
#include <fcntl.h>
#include "tcp_ip.h"

#define IB_PORT 1
#define QP_BUF_SIZE 4096
#define MR_BUF_SIZE (QP_BUF_SIZE * 1024)
#define MAX_WR 10
#define COMP_VECTOR 1

struct connection_data_s {
    uint64_t mr_addr;
    uint32_t rkey;
    uint32_t qp_num;
    uint16_t lid;
};

struct ib_handle_s {
    struct ibv_device **device_list;
    struct ibv_port_attr *port_attr;
    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_comp_channel *cq_channel;
    struct ibv_mr *mr;
};

struct ib_resources_s {
    struct ib_handle_s *ib_handle;
    struct ibv_qp *qp;
    struct connection_data_s *remote_props;
    socket_t sock;
    void *mr_addr;
};

struct ibv_device **create_device_list();
struct ibv_context *create_ibv_context(struct ibv_device **device_list);
struct ibv_pd *create_ibv_pd(struct ibv_context *ctx);
struct ibv_cq *create_ibv_cq(struct ibv_context *ctx, struct ibv_comp_channel *cq_channel);
char *create_buffer(size_t buffer_size);
struct ibv_mr *create_ibv_mr(struct ibv_pd *pd, char* mr_addr);
struct ibv_port_attr *create_port_attr(struct ibv_context *ctx);
struct ibv_qp *create_ibv_qp(struct ib_handle_s *ib_handle);
struct ibv_comp_channel *create_comp_channel(struct ibv_context *ctx);
void create_ib_handle(struct ib_handle_s *ib_handle);
struct ib_resources_s *create_init_ib_resources(struct ib_handle_s *ib_handle);
void modify_qp_to_init(struct ib_resources_s *ib_res);
void modify_qp_to_rtr(struct ib_resources_s *ib_res);
void modify_qp_to_rts(struct ib_resources_s *ib_res);
void send_qp_sync_data(struct ib_resources_s *ib_res);
int recv_qp_sync_data(struct ib_resources_s *ib_res);
struct ib_resources_s *connect_ib_server(struct ib_handle_s *ib_handle);
socket_t accept_socket(socket_t sock, struct sockaddr_in *client_addr);
struct ib_resources_s *accept_ib_client(socket_t sock, struct ib_handle_s *ib_handle);
void post_receive(struct ib_resources_s *ib_res);
void post_send(struct ib_resources_s *ib_res);
void destroy_qp(struct ibv_qp *qp);
void destroy_mr(struct ibv_mr *mr);
void destroy_cq(struct ibv_cq *cq);
void destroy_pd(struct ibv_pd *pd);
void destroy_ctx(struct ibv_context *ctx);
void destroy_device_list(struct ibv_device **device_list);
void free_buffer(void *buffer);
void close_socket(socket_t sock);
void destroy_ibv_port_attr(struct ibv_port_attr *port_attr);
void destroy_ib_resource(struct ib_resources_s *ib_res);
void destroy_ib_handle(struct ib_handle_s *ib_handle);
void poll_completion(struct ib_handle_s *ib_handle);
void poll_completion_for_client(struct ib_handle_s *ib_handle);


struct ibv_device **create_device_list() {
    struct ibv_device **device_list = ibv_get_device_list(NULL);
    if (!device_list)
    {
        perror("ibv_get_device_list");
        exit(EXIT_FAILURE);
    }
    return device_list;
}

struct ibv_context *create_ibv_context(struct ibv_device **device_list) {
    if (!device_list) 
        return NULL;
    struct ibv_context *ctx = ibv_open_device( device_list[0]);
    if (!ctx)
    {
        perror("ibv_open_device");
        exit(EXIT_FAILURE);
    }

    return ctx;
}

struct ibv_pd *create_ibv_pd(struct ibv_context *ctx) {
    if (!ctx) 
        return NULL;
    struct ibv_pd *pd = ibv_alloc_pd(ctx);
    if (!pd)
    {
        perror("ibv_alloc_pd");
        exit(EXIT_FAILURE);
    }
    return pd;
}

struct ibv_comp_channel *create_comp_channel(struct ibv_context *ctx) {
    if (!ctx) 
        return NULL;
    struct ibv_comp_channel *cq_channel = ibv_create_comp_channel(ctx);
    if (!cq_channel) {
        perror("ibv_create_comp_channel");
        exit(EXIT_FAILURE);
    }

    return cq_channel;

}

struct ibv_cq *create_ibv_cq(struct ibv_context *ctx, struct ibv_comp_channel *cq_channel) {
    if (!ctx) 
        return NULL;
    struct ibv_cq *cq = ibv_create_cq(ctx, 100, NULL, cq_channel, COMP_VECTOR);
    if (!cq)
    {
        perror("ibv_create_cq");
        exit(EXIT_FAILURE);
    }

    int rc = ibv_req_notify_cq(cq, 0);
    if (rc) {
        perror("ibv_req_notify_cq");
        exit(EXIT_FAILURE);
    }

    return cq;
}

char *create_buffer(size_t buffer_size) {
    char *mr_addr = (char *)malloc(buffer_size);
    if (!mr_addr) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return mr_addr;
}

struct ibv_mr *create_ibv_mr(struct ibv_pd *pd, char* mr_addr) {
    if (!pd || !mr_addr) 
        return NULL;
    struct ibv_mr *mr = ibv_reg_mr(pd, mr_addr, MR_BUF_SIZE, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    if (!mr) {
        perror("ibv_reg_mr");
        exit(EXIT_FAILURE);
    }
    return mr;
}

struct ibv_port_attr *create_port_attr(struct ibv_context *ctx) {
    if (!ctx) 
        return NULL;
    struct ibv_port_attr *port_attr = (struct ibv_port_attr *)(malloc(sizeof(struct ibv_port_attr)));
    if (!port_attr) 
        return NULL;
    if (ibv_query_port(ctx, IB_PORT, port_attr)) {
        perror("ibv_query_port");
        exit(EXIT_FAILURE);
    }
    return port_attr;
}

struct ibv_qp *create_ibv_qp(struct ib_handle_s *ib_handle) {
    struct ibv_qp_init_attr qp_init_attr = {
        .qp_type = IBV_QPT_RC,
        .sq_sig_all = 1,
        .send_cq = ib_handle->cq,
        .recv_cq = ib_handle->cq,
        .cap = {
            .max_send_wr = MAX_WR,
            .max_recv_wr = MAX_WR,
            .max_send_sge = 1,
            .max_recv_sge = 1,
        },
    };
    struct ibv_qp *qp = ibv_create_qp(ib_handle->pd, &qp_init_attr);
    if (!qp) {
        perror("ibv_create_qp");
        exit(EXIT_FAILURE);
    }

    return qp;
}

void create_ib_handle(struct ib_handle_s *ib_handle) {
    if (!ib_handle) {
        return;
    }
    char *mr_addr = create_buffer(MR_BUF_SIZE);

    memset(ib_handle, 0, sizeof(ib_handle));
    ib_handle->device_list = create_device_list();
    ib_handle->ctx = create_ibv_context(ib_handle->device_list);
    ib_handle->pd = create_ibv_pd(ib_handle->ctx);
    ib_handle->cq_channel = create_comp_channel(ib_handle->ctx);
    ib_handle->cq = create_ibv_cq(ib_handle->ctx, ib_handle->cq_channel);
    
    ib_handle->mr = create_ibv_mr(ib_handle->pd, mr_addr);
    ib_handle->port_attr = create_port_attr(ib_handle->ctx);
}

void alloc_mr_buffer(struct ib_resources_s *ib_res) {
    // TODO: 버퍼 할당 방식 변경
    ib_res->mr_addr = ib_res->ib_handle->mr->addr;
}

struct ib_resources_s *create_init_ib_resources(struct ib_handle_s *ib_handle) {
    if (!ib_handle) 
        return NULL;
    struct ib_resources_s *ib_res = (struct ib_resources_s *)(malloc(sizeof(struct ib_resources_s)));
    if (!ib_res) 
        return NULL;
    ib_res->ib_handle = ib_handle;
    ib_res->qp = create_ibv_qp(ib_handle);
    ib_res->remote_props = (struct connection_data_s *)(malloc(sizeof(struct connection_data_s)));
    if (!ib_res->remote_props) {
        destroy_ib_resource(ib_res);
        return NULL;
    }
    alloc_mr_buffer(ib_res);
    ib_res->sock = create_socket();

    return ib_res;
}

void modify_qp_to_init(struct ib_resources_s *ib_res) {
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
    struct ibv_qp_attr qp_attr;
    qp_attr = (struct ibv_qp_attr){
        .qp_state = IBV_QPS_INIT,
        .pkey_index = 0,
        .port_num = IB_PORT,
        .qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE,
    };
    int ret = ibv_modify_qp(ib_res->qp, &qp_attr, attr_mask);
    if (ret) {
        perror("ibv_modify_qp");
        exit(EXIT_FAILURE);
    }
}

void modify_qp_to_rtr(struct ib_resources_s *ib_res) {
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr = (struct ibv_qp_attr){
        .qp_state = IBV_QPS_RTR,
        .path_mtu = IBV_MTU_4096,
        .dest_qp_num = ib_res->remote_props->qp_num,
        .rq_psn = 0,
        .max_dest_rd_atomic = 1,
        .min_rnr_timer = 12,
        .ah_attr = {
            .is_global = 0,
            .dlid = ib_res->remote_props->lid,
            .sl = 0,
            .src_path_bits = 0,
            .port_num = IB_PORT,
        },
    };

    int rc = ibv_modify_qp(ib_res->qp, &attr, attr_mask);
    if (rc) {
        perror("ibv_modify_qp (RTR)");
        exit(EXIT_FAILURE);
    }
}

void modify_qp_to_rts(struct ib_resources_s *ib_res) {
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr = (struct ibv_qp_attr){
        .qp_state = IBV_QPS_RTS,
        .timeout = 12,
        .retry_cnt = 7,
        .rnr_retry = 7,
        .sq_psn = 0,
        .max_rd_atomic = 1,
    };

    int rc = ibv_modify_qp(ib_res->qp, &attr, attr_mask);
    if (rc) {
        perror("ibv_modify_qp (RTS)");
        exit(EXIT_FAILURE);
    }
}

void send_qp_sync_data(struct ib_resources_s *ib_res) {
    struct connection_data_s conn_data = {
        .mr_addr = (uint64_t)(uintptr_t)ib_res->mr_addr,
        .rkey = ib_res->ib_handle->mr->rkey,
        .qp_num = ib_res->qp->qp_num,
        .lid = ib_res->ib_handle->port_attr->lid,
    };

    if (send(ib_res->sock, &conn_data, sizeof(conn_data), MSG_WAITALL) == SOCKET_ERROR) {
        perror("send");
        exit(EXIT_FAILURE);
    }
}

int recv_qp_sync_data(struct ib_resources_s *ib_res) {
    int rc;
    rc = recv(ib_res->sock, ib_res->remote_props, sizeof(*ib_res->remote_props), MSG_WAITALL);
    if (rc == SOCKET_ERROR) {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    return rc;
}

struct ib_resources_s *connect_ib_server(struct ib_handle_s *ib_handle) {
    struct ib_resources_s *ib_res = create_init_ib_resources(ib_handle);

    connect_tcp_to_server(ib_res->sock, SERVER_IP, SERVER_PORT);
    send_qp_sync_data(ib_res);
    recv_qp_sync_data(ib_res);
    modify_qp_to_init(ib_res);
    modify_qp_to_rtr(ib_res);
    modify_qp_to_rts(ib_res);
    
    return ib_res;
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

struct ib_resources_s *accept_ib_client(socket_t sock, struct ib_handle_s *ib_handle) {
    struct ib_resources_s *ib_res = create_init_ib_resources(ib_handle);
    
    struct sockaddr_in client_addr;
    ib_res->sock = accept_socket(sock, &client_addr);
    if (recv_qp_sync_data(ib_res) < 0) {
        exit(EXIT_FAILURE);
    }
    modify_qp_to_init(ib_res);
    modify_qp_to_rtr(ib_res);
    modify_qp_to_rts(ib_res);
    
    post_receive(ib_res);
    send_qp_sync_data(ib_res);

    return ib_res;
}

void post_receive(struct ib_resources_s *ib_res)
{
    struct ibv_sge sge;
    struct ibv_recv_wr recv_wr;
    struct ibv_recv_wr *bad_wr;

    memset(&sge, 0, sizeof(sge));
    sge = (struct ibv_sge){
        .addr = (uintptr_t)ib_res->mr_addr,
        .length = QP_BUF_SIZE,
        .lkey = ib_res->ib_handle->mr->lkey,
    };

    memset(&recv_wr, 0, sizeof(recv_wr));
    recv_wr = (struct ibv_recv_wr){
        .wr_id = 1,
        .next = NULL,
        .sg_list = &sge,
        .num_sge = 1,
    };

    int rc = ibv_post_recv(ib_res->qp, &recv_wr, &bad_wr);
    if (rc) {
        perror("ibv_post_recv");
        exit(EXIT_FAILURE);
    }
}

void post_send(struct ib_resources_s *ib_res)
{
    struct ibv_sge sge;
    struct ibv_send_wr send_wr;
    struct ibv_send_wr *bad_wr;

    memset(&sge, 0, sizeof(sge));
    sge = (struct ibv_sge){
        .addr = (uintptr_t)ib_res->mr_addr,
        .length = strlen(ib_res->mr_addr) + 1,
        .lkey = ib_res->ib_handle->mr->lkey,
    };

    memset(&send_wr, 0, sizeof(send_wr));
    send_wr = (struct ibv_send_wr){
        .wr_id = 0,
        .next = NULL,
        .sg_list = &sge,
        .num_sge = 1,
        .opcode = IBV_WR_SEND,
        .send_flags = IBV_SEND_SIGNALED,
    };

    if (ibv_post_send(ib_res->qp, &send_wr, &bad_wr)) {
        perror("ibv_post_send");
        exit(EXIT_FAILURE);
    }
}

void destroy_qp(struct ibv_qp *qp)
{
    if (qp) {
        if (ibv_destroy_qp(qp)) {
            perror("ibv_destroy_qp");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_mr(struct ibv_mr *mr)
{
    if (mr) {
        if (ibv_dereg_mr(mr)) {
            perror("ibv_dereg_mr");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_cq(struct ibv_cq *cq)
{
    if (cq) {
        if (ibv_destroy_cq(cq)) {
            perror("ibv_destroy_cq");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_pd(struct ibv_pd *pd)
{
    if (pd) {
        if (ibv_dealloc_pd(pd)) {
            perror("ibv_dealloc_pd");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_ctx(struct ibv_context *ctx)
{
    if (ctx) {
        if (ibv_close_device(ctx))
        {
            perror("ibv_close_device");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_device_list(struct ibv_device **device_list)
{
    if (device_list) {
        ibv_free_device_list(device_list);
    }
}

void free_buffer(void *buffer) {
    if (buffer) {
        free(buffer);
    }
}

void close_socket(socket_t sock) {
    if (sock != INVALID_SOCKET) {
        print_disconnected_client(sock);
        close(sock);
    }
}

void destroy_ibv_port_attr(struct ibv_port_attr *port_attr) {
    if (port_attr) {
        free(port_attr);
    }
}

void destroy_ib_resource(struct ib_resources_s *ib_res) {
    if (!ib_res) {
        return;
    }
    destroy_qp(ib_res->qp);
    free(ib_res->remote_props);
    close_socket(ib_res->sock);
    free(ib_res);
}

void destroy_ib_handle(struct ib_handle_s *ib_handle) {
    if (!ib_handle)
        return;
    destroy_mr(ib_handle->mr);
    destroy_cq(ib_handle->cq);
    destroy_pd(ib_handle->pd);
    destroy_ctx(ib_handle->ctx);
    destroy_ibv_port_attr(ib_handle->port_attr);
    destroy_device_list(ib_handle->device_list);
    memset(ib_handle, 0, sizeof(struct ib_handle_s));
}

void poll_completion_for_client(struct ib_handle_s *ib_handle) {
    int rc;
    struct ibv_cq *event_cq;
    void *event_cq_ctx;
    struct ibv_wc wc;
    
    rc = ibv_req_notify_cq(ib_handle->cq, 0);
    if (rc) {
        perror("ibv_req_notify_cq");
        exit(EXIT_FAILURE);
    }

    rc = ibv_get_cq_event(ib_handle->cq_channel, &event_cq, &event_cq_ctx);
    if (rc) {
        perror("ibv_get_cq_event");
        exit(EXIT_FAILURE);
    }

    do {
        rc = ibv_poll_cq(event_cq, 1, &wc);
        if (rc < 0) {
            perror("ibv_poll_cq");
            exit(EXIT_FAILURE);
        }
        printf("[wc.opcode]]:%d\n",wc.opcode);
    } while (rc == 0);
    

    ibv_ack_cq_events(event_cq, 1);

    rc = ibv_req_notify_cq(ib_handle->cq, 0);
    if (rc) {
        perror("ibv_req_notify_cq");
        exit(EXIT_FAILURE);
    }
}