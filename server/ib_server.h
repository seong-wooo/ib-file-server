#ifndef IB_SERVER_H
#define IB_SERVER_H

#include <sys/epoll.h>
#include "wthr.h"
#include "hash.h"
#include "server.h"
#include "ib.h"

struct ib_server_resources_s {
    int epoll_fd;
    socket_t sock;
    struct ib_handle_s *ib_handle;
    int pipefd[2];
    struct queue_s *queue;
    struct hash_map_s *qp_map;
};

struct ib_server_resources_s *create_ib_server_resources(void);
void accept_ib_client(struct ib_server_resources_s *res);
void poll_completion(struct ib_server_resources_s *res);
void send_response(struct fd_info_s *fd_info, struct hash_map_s *qp_map);
void disconnect_client(struct fd_info_s *fd_info, struct hash_map_s *qp_map);
void destroy_res(struct ib_server_resources_s *res);
#endif