#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <infiniband/verbs.h>
#include <stdlib.h>
#include <stdio.h>
#include "server.h"
#include "queue.h"
#include "ib.h"
#include "err_check.h"

struct ibv_device **create_device_list(void) {
    struct ibv_device **device_list = ibv_get_device_list(NULL);
    check_null(device_list, "ibv_get_device_list");
    
    return device_list;
}

struct ibv_context *create_ibv_context(void) {
    struct ibv_device **device_list = create_device_list();
    struct ibv_context *ctx = ibv_open_device(device_list[0]);
    check_null(ctx, "ibv_open_device");
    ibv_free_device_list(device_list);
    return ctx;
}

struct ibv_pd *create_ibv_pd(struct ibv_context *ctx) {
    check_null(ctx, "ibv_context is NULL");
    struct ibv_pd *pd = ibv_alloc_pd(ctx);
    check_null(pd, "ibv_alloc_pd");
    return pd;
}

struct ibv_srq *create_ibv_srq(struct ibv_pd *pd) {
    check_null(pd, "ibv_pd is NULL");
    struct ibv_srq_init_attr srq_init_attr = {
        .srq_context = NULL,
        .attr = {
            .max_wr = MAX_WR,
            .max_sge = 1,
            .srq_limit = 0,
        },
    };
    struct ibv_srq *srq = ibv_create_srq(pd, &srq_init_attr);
    check_null(srq, "ibv_create_srq");

    return srq;
}

struct ibv_comp_channel *create_comp_channel(struct ibv_context *ctx) {
    check_null(ctx, "ibv_context is NULL");
    
    struct ibv_comp_channel *cq_channel = ibv_create_comp_channel(ctx);
    check_null(cq_channel, "ibv_create_comp_channel");

    return cq_channel;

}

void notify_cq(struct ibv_cq *cq) {
    int rc = ibv_req_notify_cq(cq, 0);
    check_error(rc, "ibv_req_notify_cq");
}

struct ibv_cq *create_ibv_cq(struct ibv_context *ctx, struct ibv_comp_channel *cq_channel) {
    check_null(ctx, "ibv_context is NULL");
    
    struct ibv_cq *cq = ibv_create_cq(ctx, 100, NULL, cq_channel, COMP_VECTOR);
    check_null(cq, "ibv_create_cq");

    notify_cq(cq);

    return cq;
}

void *create_buffer(size_t buffer_size) {
    void *mr_addr = (void *)calloc(1, buffer_size);
    check_null(mr_addr, "malloc");

    return mr_addr;
}

struct ibv_mr *create_mr(struct ibv_pd *pd) {
    void *mr_addr = create_buffer(MESSAGE_SIZE);
    struct ibv_mr *mr = ibv_reg_mr(pd, mr_addr, MESSAGE_SIZE, IBV_ACCESS_LOCAL_WRITE);
    check_null(mr, "ibv_reg_mr");
    
    return mr;
}

void post_receive(struct ibv_srq *srq, struct ibv_mr *mr) {
    struct ibv_sge sge;
    struct ibv_recv_wr recv_wr;
    struct ibv_recv_wr *bad_wr;

    memset(&sge, 0, sizeof(sge));
    sge = (struct ibv_sge){
        .addr = (uint64_t)(uintptr_t)mr->addr,
        .length = MESSAGE_SIZE,
        .lkey = mr->lkey,
    };

    memset(&recv_wr, 0, sizeof(recv_wr));
    recv_wr = (struct ibv_recv_wr){
        .wr_id = (uint64_t)(uintptr_t)mr,
        .next = NULL,
        .sg_list = &sge,
        .num_sge = 1,
    };

    int rc = ibv_post_srq_recv(srq, &recv_wr, &bad_wr);
    if (rc) {
        perror("ibv_post_srq_recv");
        exit(EXIT_FAILURE);
    }
}

void create_post_receive(struct ibv_srq *srq, struct ibv_pd *pd) {
    for (int i = 0; i < MAX_WR; i++) {
        struct ibv_mr *mr = create_mr(pd);
        post_receive(srq, mr);
    }
}

struct ibv_qp *create_ibv_qp(struct ib_handle_s *ib_handle) {
    struct ibv_qp_init_attr qp_init_attr = {
        .qp_type = IBV_QPT_RC,
        .sq_sig_all = 1,
        .qp_context = NULL,
        .send_cq = ib_handle->cq,
        .recv_cq = ib_handle->cq,
        .srq = ib_handle->srq,
        .cap = {
            .max_send_wr = MAX_WR,
            .max_recv_wr = 0,
            .max_send_sge = 1,
            .max_recv_sge = 0,
        },
    };
    struct ibv_qp *qp = ibv_create_qp(ib_handle->pd, &qp_init_attr);
    check_null(qp, "ibv_create_qp");

    return qp;
}

uint16_t get_lid(struct ibv_context *ctx) {
    check_null(ctx, "ibv_context is NULL");
    uint16_t lid;

    struct ibv_port_attr *port_attr = 
        (struct ibv_port_attr *)(malloc(sizeof(struct ibv_port_attr)));
    check_null(port_attr, "malloc");    
    
    int rc = ibv_query_port(ctx, IB_PORT, port_attr);
    if (rc) {
        perror("ibv_query_port");
        exit(EXIT_FAILURE);
    }
    lid = port_attr->lid;
    free(port_attr);
    return lid;
}

struct ib_handle_s *create_ib_handle(void) {
    struct ib_handle_s *ib_handle = (struct ib_handle_s *) calloc(1, sizeof(struct ib_handle_s));
    check_null(ib_handle, "malloc");

    ib_handle->ctx = create_ibv_context();
    ib_handle->pd = create_ibv_pd(ib_handle->ctx);
    ib_handle->srq = create_ibv_srq(ib_handle->pd);
    ib_handle->cq_channel = create_comp_channel(ib_handle->ctx);
    ib_handle->cq = create_ibv_cq(ib_handle->ctx, ib_handle->cq_channel);
    ib_handle->lid = get_lid(ib_handle->ctx);
    create_post_receive(ib_handle->srq, ib_handle->pd);

    return ib_handle;
}

struct ib_resources_s *create_init_ib_resources(struct ib_handle_s *ib_handle, socket_t sock) {
    check_null(ib_handle, "ib_handle is NULL");

    struct ib_resources_s *ib_res = (struct ib_resources_s *)(malloc(sizeof(struct ib_resources_s)));
    check_null(ib_res, "malloc");

    ib_res->ib_handle = ib_handle;
    ib_res->qp = create_ibv_qp(ib_handle);
    ib_res->remote_props = (struct connection_data_s *)(malloc(sizeof(struct connection_data_s)));
    check_null(ib_res->remote_props, "malloc");
    ib_res->sock = accept_socket(sock);
    
    return ib_res;
}

void modify_qp_to_init(struct ib_resources_s *ib_res) {
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_PKEY_INDEX | 
        IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
    struct ibv_qp_attr qp_attr;
    qp_attr = (struct ibv_qp_attr){
        .qp_state = IBV_QPS_INIT,
        .pkey_index = 0,
        .port_num = IB_PORT,
        .qp_access_flags = IBV_ACCESS_LOCAL_WRITE,
    };
    int rc = ibv_modify_qp(ib_res->qp, &qp_attr, attr_mask);
    if (rc) {
        perror("ibv_modify_qp");
        exit(EXIT_FAILURE);
    }
}

void modify_qp_to_rtr(struct ib_resources_s *ib_res) {
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | 
        IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr = (struct ibv_qp_attr){
        .qp_state = IBV_QPS_RTR,
        .path_mtu = IBV_MTU_4096,
        .dest_qp_num = ib_res->remote_props->qp_num,
        .min_rnr_timer = 12,
        .ah_attr = {
            .dlid = ib_res->remote_props->lid,
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
    enum ibv_qp_attr_mask attr_mask = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | 
        IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr = (struct ibv_qp_attr){
        .qp_state = IBV_QPS_RTS,
        .timeout = 12,
        .retry_cnt = 7,
        .rnr_retry = 7,
        .max_rd_atomic = 1,
    };

    int rc = ibv_modify_qp(ib_res->qp, &attr, attr_mask);
    if (rc) {
        perror("ibv_modify_qp (RTS)");
        exit(EXIT_FAILURE);
    }
}

void send_qp_data(struct ib_resources_s *ib_res) {
    struct connection_data_s conn_data = {
        .qp_num = ib_res->qp->qp_num,
        .lid = ib_res->ib_handle->lid,
    };

    int rc = send(ib_res->sock, &conn_data, sizeof(conn_data), MSG_WAITALL);
    check_error(rc, "send");
}

int recv_qp_data(struct ib_resources_s *ib_res) {
    int rc = recv(ib_res->sock, ib_res->remote_props, 
        sizeof(*ib_res->remote_props), MSG_WAITALL);
    check_error(rc, "recv");

    return rc;
}

void post_send(struct ibv_mr *mr, struct ibv_qp *qp, struct packet_s *packet) {
    struct ibv_sge sge;
    struct ibv_send_wr send_wr;
    struct ibv_send_wr *bad_wr;

    size_t header_size = sizeof(struct packet_header_s);

    memset(&sge, 0, sizeof(sge));
    sge = (struct ibv_sge){
        .addr = (uint64_t)(uintptr_t)mr->addr,
        .length = header_size + packet->header.body_size,
        .lkey = mr->lkey,
    };

    memset(&send_wr, 0, sizeof(send_wr));
    send_wr = (struct ibv_send_wr){
        .wr_id = (uint64_t)(uintptr_t)mr,
        .next = NULL,
        .sg_list = &sge,
        .num_sge = 1,
        .opcode = IBV_WR_SEND,
        .send_flags = IBV_SEND_SIGNALED,
    };
    
    serialize_packet(packet, mr->addr);
    int rc = ibv_post_send(qp, &send_wr, &bad_wr);
    if (rc) {
        perror("ibv_post_send");
        exit(EXIT_FAILURE);
    }
    free_packet(packet);
}

void destroy_qp(struct ibv_qp *qp) {
    if (qp) {
        ibv_destroy_qp(qp);
    }
}

void destroy_ib_resource(struct ib_resources_s *ib_res) {
    if (ib_res) {
        destroy_qp(ib_res->qp);
        free(ib_res->remote_props);
        close_socket(ib_res->sock);
        free(ib_res);
    }
}

void destroy_cq(struct ibv_cq *cq) {
    if (cq) {
        int rc = ibv_destroy_cq(cq);
        if (rc) {
            perror("ibv_destroy_cq");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_cq_channel(struct ibv_comp_channel *cq_channel) {
    if (cq_channel) {
        int rc = ibv_destroy_comp_channel(cq_channel);
        if (rc) {
            perror("ibv_destroy_comp_channel");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_srq(struct ibv_srq *srq) {
    if (srq) {
        if (ibv_destroy_srq(srq)) {
            perror("ibv_destroy_srq");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_pd(struct ibv_pd *pd) {
    if (pd) {
        if (ibv_dealloc_pd(pd)) {
            perror("ibv_dealloc_pd");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_ctx(struct ibv_context *ctx) {
    if (ctx) {
        if (ibv_close_device(ctx))
        {
            perror("ibv_close_device");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_ib_handle(struct ib_handle_s *ib_handle) {
    if (ib_handle) {
        destroy_cq(ib_handle->cq);
        destroy_cq_channel(ib_handle->cq_channel);
        destroy_srq(ib_handle->srq);
        destroy_pd(ib_handle->pd);
        destroy_ctx(ib_handle->ctx);
    }
}