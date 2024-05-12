#ifndef IB_H
#define IB_H
#include <stdint.h>
#include <infiniband/verbs.h>
#include "tcp_ip.h"
#include "message.h"

#define IB_PORT 1
#define MAX_WR 64
#define COMP_VECTOR 1

struct connection_data_s {
    uint32_t qp_num;
    uint16_t lid;
};

struct ib_handle_s {
    struct ibv_device **device_list;
    struct ibv_port_attr *port_attr;
    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_srq *srq;
    struct ibv_cq *cq;
    struct ibv_comp_channel *cq_channel;
    struct queue_s *mr_pool;
};

struct ib_resources_s {
    struct ib_handle_s *ib_handle;
    struct ibv_qp *qp;
    struct connection_data_s *remote_props;
    socket_t sock;
};

struct ib_handle_s *create_ib_handle(void);
struct ib_resources_s *create_init_ib_resources(struct ib_handle_s *ib_handle, socket_t sock);
void modify_qp_to_init(struct ib_resources_s *ib_res);
void modify_qp_to_rtr(struct ib_resources_s *ib_res);
void modify_qp_to_rts(struct ib_resources_s *ib_res);
void send_qp_sync_data(struct ib_resources_s *ib_res);
int recv_qp_sync_data(struct ib_resources_s *ib_res);
struct ib_resources_s *connect_ib_server(struct ib_handle_s *ib_handle);
void post_receive(struct ibv_srq *srq, struct ibv_mr *mr);
void post_send(struct ibv_mr *mr, struct ibv_qp *qp, struct packet_s *packet);
struct ibv_mr *get_mr(struct ib_handle_s *ib_handle);
void restore_mr(struct queue_s *mr_pool, struct ibv_mr *mr);
void destroy_mr(struct ibv_mr *mr);
void destroy_ib_resource(struct ib_resources_s *ib_res);
void destroy_ib_handle(struct ib_handle_s *ib_handle);
void notify_cq(struct ibv_cq *cq);
#endif