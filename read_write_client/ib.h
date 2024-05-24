#ifndef IB_H
#define IB_H
#include <stdint.h>
#include <infiniband/verbs.h>
#include "tcp_ip.h"

#define IB_PORT 1
#define IB_SERVER_PORT 7001
#define MAX_WR 64
#define COMP_VECTOR 1
#define MR_SIZE 4000030

struct ib_handle_s {
    uint16_t lid;
    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_mr *mr;
    struct ibv_cq *cq;
};

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
    struct rdma_data_s rdma_data;
};

struct ib_resources_s {
    struct ibv_qp *qp;
    socket_t sock;
};

struct ib_handle_s *create_ib_handle(void);
struct ib_resources_s *connect_ib_server(struct ib_handle_s *ib_handle);
void post_receive(struct ibv_qp *qp, struct ibv_mr *mr);
void post_send(struct ibv_qp *qp, struct ibv_mr *mr);
void poll_completion(struct ib_handle_s *ib_handle, int count);
void destroy_ib_resource(struct ib_resources_s *ib_res);
void destroy_ib_handle(struct ib_handle_s *ib_handle);
#endif