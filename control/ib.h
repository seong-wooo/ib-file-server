#ifndef IB_H
#define IB_H
#include <stdint.h>
#include <infiniband/verbs.h>
#include "tcp_ip.h"
#include "message.h"

#define IB_PORT 1
#define IB_SERVER_PORT 9001
#define MAX_WR 64
#define MAX_QP 50
#define COMP_VECTOR 1

struct ib_handle_s {
    struct ibv_device **device_list;
    struct ibv_port_attr *port_attr;
    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_comp_channel *cq_channel;
    struct queue_s *conn_res_pool;
};

struct conn_prop_s {
    uint32_t qp_num;
    uint16_t lid;
};

struct conn_resource_s {
    socket_t socket;
    struct ibv_qp *qp;
    struct ibv_mr *mr;
};

struct ib_handle_s *create_ib_handle(void);
struct ib_resources_s *connect_ib_server(struct ib_handle_s *ib_handle);
void post_receive(struct conn_resource_s *qp_mr);
void post_send(struct conn_resource_s *qp_mr, struct packet_s *packet);
struct ibv_mr *create_mr(struct ibv_pd *pd);
void notify_cq(struct ibv_cq *cq);
#endif