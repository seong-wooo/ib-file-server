#ifndef IB_SERVER_H
#define IB_SERVER_H

#include <sys/epoll.h>
#include "server.h"
#include "ib.h"

#define IB_SERVER_PORT 7001

struct ib_meta_data_s {
    uint32_t qp_num;
    struct ibv_mr *mr;
};

struct ib_server_resources_s {
    int epoll_fd;
    socket_t sock;
    struct ib_handle_s *ib_handle;
    struct hash_map_s *qp_map;
};

void send_receive_server(void);
#endif