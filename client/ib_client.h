#ifndef IB_CLIENT_H
#define IB_CLIENT_H
#include <stdint.h>
#include <infiniband/verbs.h>
#include "tcp_ip.h"
#include "message.h"
#include "test_client.h"

#define IB_PORT 1
#define IB_SERVER_PORT 9001
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
    struct ibv_mr *mr;
    struct ibv_srq *srq;
    struct ibv_cq *cq;
};

struct ib_resources_s {
    struct ib_handle_s *ib_handle;
    struct ibv_qp *qp;
    struct connection_data_s *remote_props;
    socket_t sock;
};

void ib_client(void);
void *ib_test_client(void *arg);

#endif