#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include "tcp_ip.h"
#include "queue.h"
#include "ib.h"

struct ibv_device **create_device_list(void) {
    struct ibv_device **device_list = ibv_get_device_list(NULL);
    if (!device_list) {
        perror("ibv_get_device_list");
        exit(EXIT_FAILURE);
    }
    return device_list;
}

struct ibv_context *create_ibv_context(struct ibv_device **device_list) {
    if (!device_list) {
        return NULL;
    }

    struct ibv_context *ctx = ibv_open_device(device_list[0]);

    if (!ctx) {
        perror("ibv_open_device");
        exit(EXIT_FAILURE);
    }

    return ctx;
}

struct ibv_pd *create_ibv_pd(struct ibv_context *ctx) {
    if (!ctx) { 
        return NULL;
    }
    
    struct ibv_pd *pd = ibv_alloc_pd(ctx);
    
    if (!pd) {
        perror("ibv_alloc_pd");
        exit(EXIT_FAILURE);
    }
    return pd;
}

struct ibv_comp_channel *create_comp_channel(struct ibv_context *ctx) {
    if (!ctx) {
        return NULL;
    }
    struct ibv_comp_channel *cq_channel = ibv_create_comp_channel(ctx);
    if (!cq_channel) {
        perror("ibv_create_comp_channel");
        exit(EXIT_FAILURE);
    }

    return cq_channel;
}

void *create_buffer(size_t buffer_size) {
    void *mr_addr = (void *)calloc(1, buffer_size);
    
    if (!mr_addr) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    
    return mr_addr;
}

struct ibv_srq *create_ibv_srq(struct ibv_pd *pd) {
    if (!pd) 
        return NULL;
    struct ibv_srq_init_attr srq_init_attr = {
        .srq_context = NULL,
        .attr = {
            .max_wr = MAX_WR,
            .max_sge = 1,
            .srq_limit = 0,
        },
    };
    struct ibv_srq *srq = ibv_create_srq(pd, &srq_init_attr);
    if (!srq) {
        perror("ibv_create_srq");
        exit(EXIT_FAILURE);
    }

    return srq;
}

void notify_cq(struct ibv_cq *cq) {
    int rc = ibv_req_notify_cq(cq, 0);
    if (rc) {
        perror("ibv_req_notify_cq");
        exit(EXIT_FAILURE);
    }
}

struct ibv_cq *create_ibv_cq(struct ibv_context *ctx, struct ibv_comp_channel *cq_channel) {
    if (!ctx) 
        return NULL;
    struct ibv_cq *cq = ibv_create_cq(ctx, 100, NULL, cq_channel, COMP_VECTOR); // COMP VECTOR 확인하기
    if (!cq)
    {
        perror("ibv_create_cq");
        exit(EXIT_FAILURE);
    }

    notify_cq(cq);

    return cq;
}

struct ibv_port_attr *create_port_attr(struct ibv_context *ctx) {
    if (!ctx) {
        return NULL;
    }
    
    struct ibv_port_attr *port_attr = 
        (struct ibv_port_attr *)(malloc(sizeof(struct ibv_port_attr)));
    
    if (!port_attr) {
        return NULL;
    }
    
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
        .qp_context = NULL,
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

void send_qp_sync_data(socket_t sock, uint32_t qp_num, uint16_t lid) {
    struct conn_prop_s conn_data = {
        .qp_num = qp_num,
        .lid = lid,
    };

    if (send(sock, &conn_data, sizeof(conn_data), MSG_WAITALL) == SOCKET_ERROR) {
        perror("send");
        exit(EXIT_FAILURE);
    }
}

struct conn_prop_s *recv_qp_sync_data(socket_t sock) {
    int rc;
    struct conn_prop_s *remote_props = (struct conn_prop_s *)(malloc(sizeof(struct conn_prop_s)));
    rc = recv(sock, remote_props, sizeof(*remote_props), MSG_WAITALL);
    if (rc == SOCKET_ERROR) {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    return remote_props;
}

void modify_qp_to_init(struct ibv_qp *qp) {
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_PKEY_INDEX | 
        IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
    struct ibv_qp_attr qp_attr;
    qp_attr = (struct ibv_qp_attr){
        .qp_state = IBV_QPS_INIT,
        .pkey_index = 0,
        .port_num = IB_PORT,
        .qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | 
            IBV_ACCESS_REMOTE_WRITE,
    };
    int ret = ibv_modify_qp(qp, &qp_attr, attr_mask);
    if (ret) {
        perror("ibv_modify_qp");
        exit(EXIT_FAILURE);
    }
}

void modify_qp_to_rtr(struct ibv_qp *qp, struct conn_prop_s *remote_props) {
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | 
        IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr = (struct ibv_qp_attr){
        .qp_state = IBV_QPS_RTR,
        .path_mtu = IBV_MTU_4096,
        .dest_qp_num = remote_props->qp_num,
        .rq_psn = 0,
        .max_dest_rd_atomic = 1,
        .min_rnr_timer = 12,
        .ah_attr = {
            .is_global = 0,
            .dlid = remote_props->lid,
            .sl = 0,
            .src_path_bits = 0,
            .port_num = IB_PORT,
        },
    };

    int rc = ibv_modify_qp(qp, &attr, attr_mask);
    if (rc) {
        perror("ibv_modify_qp (RTR)");
        exit(EXIT_FAILURE);
    }
}

void modify_qp_to_rts(struct ibv_qp *qp) {
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | 
        IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
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

    int rc = ibv_modify_qp(qp, &attr, attr_mask);
    if (rc) {
        perror("ibv_modify_qp (RTS)");
        exit(EXIT_FAILURE);
    }
}

struct ibv_mr *create_mr(struct ibv_pd *pd) {
    void *mr_addr = create_buffer(MESSAGE_SIZE);
    struct ibv_mr *mr = ibv_reg_mr(pd, mr_addr, MESSAGE_SIZE, IBV_ACCESS_LOCAL_WRITE | 
        IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    if (!mr) {
        perror("ibv_reg_mr");
        exit(EXIT_FAILURE);
    }
    return mr;
}

struct queue_s *create_conn_res_pool(struct ib_handle_s *ib_handle) {
    socket_t sock = connect_tcp_to_server(SERVER_IP, IB_SERVER_PORT);

    struct queue_s *conn_res_pool = create_queue();
    for (int i = 0; i < MAX_QP; i++) {
        struct ibv_qp *qp = create_ibv_qp(ib_handle);
        send_qp_sync_data(sock, qp->qp_num, ib_handle->port_attr->lid);
        struct conn_prop_s *remote_props = recv_qp_sync_data(sock);
        modify_qp_to_init(qp);
        modify_qp_to_rtr(qp, remote_props);
        modify_qp_to_rts(qp);
        
        struct ibv_mr *mr = create_mr(ib_handle->pd);
        struct conn_resource_s *conn_res = (struct conn_resource_s *)(malloc(sizeof(struct conn_resource_s)));
        conn_res->qp = qp;
        conn_res->mr = mr;

        enqueue(conn_res_pool, conn_res);
    }
    close(sock);

    return conn_res_pool;
}

struct ib_handle_s *create_ib_handle(void) {
    struct ib_handle_s *ib_handle = (struct ib_handle_s *)(calloc(1, sizeof(struct ib_handle_s)));
    ib_handle->device_list = create_device_list();
    ib_handle->ctx = create_ibv_context(ib_handle->device_list);
    ib_handle->pd = create_ibv_pd(ib_handle->ctx);
    ib_handle->cq_channel = create_comp_channel(ib_handle->ctx);
    ib_handle->cq = create_ibv_cq(ib_handle->ctx, ib_handle->cq_channel);
    ib_handle->port_attr = create_port_attr(ib_handle->ctx);
    ib_handle->conn_res_pool = create_conn_res_pool(ib_handle);

    return ib_handle;
}

void post_receive(struct conn_resource_s *conn_res) {
    struct ibv_sge sge;
    struct ibv_recv_wr recv_wr;
    struct ibv_recv_wr *bad_wr;

    memset(&sge, 0, sizeof(sge));
    sge = (struct ibv_sge){
        .addr = (uint64_t)(uintptr_t)conn_res->mr->addr,
        .length = MESSAGE_SIZE,
        .lkey = conn_res->mr->lkey,
    };

    memset(&recv_wr, 0, sizeof(recv_wr));
    recv_wr = (struct ibv_recv_wr){
        .wr_id = (uint64_t)(uintptr_t)conn_res,
        .next = NULL,
        .sg_list = &sge,
        .num_sge = 1,
    };

    int rc = ibv_post_recv(conn_res->qp, &recv_wr, &bad_wr);
    if (rc) {
        perror("ibv_post_recv");
        exit(EXIT_FAILURE);
    }
}

void post_send(struct conn_resource_s *conn_res, struct packet_s *packet) {
    struct ibv_sge sge;
    struct ibv_send_wr send_wr;
    struct ibv_send_wr *bad_wr;

    size_t header_size = sizeof(struct packet_header_s);

    memset(&sge, 0, sizeof(sge));
    sge = (struct ibv_sge){
        .addr = (uint64_t)(uintptr_t)conn_res->mr->addr,
        .length = header_size + packet->header.body_size,
        .lkey = conn_res->mr->lkey,
    };

    memset(&send_wr, 0, sizeof(send_wr));
    send_wr = (struct ibv_send_wr){
        .wr_id = (uint64_t)(uintptr_t)conn_res,
        .next = NULL,
        .sg_list = &sge,
        .num_sge = 1,
        .opcode = IBV_WR_SEND,
        .send_flags = IBV_SEND_SIGNALED,
    };
    
    serialize_packet(packet, conn_res->mr->addr);

    if (ibv_post_send(conn_res->qp, &send_wr, &bad_wr)) {
        perror("ibv_post_send");
        exit(EXIT_FAILURE);
    }
    free_packet(packet);
}
