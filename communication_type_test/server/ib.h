#ifndef IB_H
#define IB_H
#include <stdint.h>
#include <infiniband/verbs.h>
#include "tcp_ip.h"

#define IB_PORT 1
#define MAX_WR 100
#define MR_SIZE 4194334
#define COMP_VECTOR 1

struct connection_data_s {
    uint32_t qp_num;
    uint16_t lid;
};

struct rdma_data_s {
    uint32_t rkey;
    uint64_t remote_addr;
};

struct client_conn_data_s {
    struct connection_data_s conn_data;
    struct rdma_data_s rdma_data;
};

struct message_header_s {
    int data_size;
    int type;
    struct rdma_data_s rdma_data;
};

struct ib_handle_s {
    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_srq *srq;
    struct ibv_cq *cq;
    struct ibv_comp_channel *cq_channel;
    uint16_t lid;
};

struct ib_resources_s {
    struct ibv_qp *qp;
    socket_t sock;
    struct client_conn_data_s *client_conn_data;
};

struct ib_handle_s *create_ib_handle(void);
struct ib_resources_s *create_init_ib_resources(struct ib_handle_s *ib_handle, socket_t sock);
void modify_qp_to_init(struct ibv_qp *qp);
void modify_qp_to_rtr(struct ib_resources_s *ib_res);
void modify_qp_to_rts(struct ibv_qp *qp);
void send_qp_data(socket_t sock, struct connection_data_s conn_data);
void recv_qp_data(struct ib_resources_s *ib_res);
struct ib_resources_s *connect_ib_server(struct ib_handle_s *ib_handle);
void post_receive(struct ibv_srq *srq, struct ibv_mr *mr);
void post_send(struct ibv_qp *qp, struct ibv_mr *mr, int resp_size);
void post_read(struct ibv_qp *qp, struct ibv_mr *mr, struct message_header_s *header);
void post_write(struct ibv_qp *qp, struct ibv_mr *mr, struct message_header_s *header);
void destroy_ib_resource(struct ib_resources_s *ib_res);
void destroy_ib_handle(struct ib_handle_s *ib_handle);
void notify_cq(struct ibv_cq *cq);
#endif