#ifndef IB_H
#define IB_H
#include <stdint.h>
#include <infiniband/verbs.h>
#include "tcp_ip.h"
#include "message.h"

#define IB_PORT 1
#define IB_SERVER_PORT 9001
#define MAX_WR 64
#define COMP_VECTOR 1

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

struct ib_resources_s {
    struct ib_handle_s *ib_handle;
    struct ibv_qp *qp;
    struct connection_data_s *remote_props;
    socket_t sock;
};

struct ib_handle_s *create_ib_handle(void);
struct ib_resources_s *connect_ib_server(struct ib_handle_s *ib_handle);
void post_receive(struct ibv_qp *qp, struct ibv_mr *mr);
void post_send(struct ibv_mr *mr, struct ibv_qp *qp, struct packet_s *packet);
void poll_completion(struct ib_handle_s *ib_handle);
void destroy_ib_resource(struct ib_resources_s *ib_res);
void destroy_ib_handle(struct ib_handle_s *ib_handle);
#endif