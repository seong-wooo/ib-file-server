#ifndef IB_H
#define IB_H
#include <stdint.h>
#include <infiniband/verbs.h>
#include "tcp_ip.h"

#define IB_PORT 1
#define MAX_WR 64
#define MR_SIZE 4000004
#define COMP_VECTOR 1

struct connection_data_s {
    uint32_t qp_num;
    uint16_t lid;
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
};

struct ib_handle_s *create_ib_handle(void);
struct ib_resources_s *create_init_ib_resources(struct ib_handle_s *ib_handle, socket_t sock);
void modify_qp_to_init(struct ibv_qp *qp);
void modify_qp_to_rtr(struct ibv_qp *qp, struct connection_data_s remote_data);
void modify_qp_to_rts(struct ibv_qp *qp);
void send_qp_data(socket_t sock, struct connection_data_s conn_data);
struct connection_data_s recv_qp_data(socket_t sock);
struct ib_resources_s *connect_ib_server(struct ib_handle_s *ib_handle);
void post_receive(struct ibv_srq *srq, struct ibv_mr *mr);
void post_send(struct ibv_qp *qp, struct ibv_mr *mr);
void destroy_ib_resource(struct ib_resources_s *ib_res);
void destroy_ib_handle(struct ib_handle_s *ib_handle);
void notify_cq(struct ibv_cq *cq);
#endif