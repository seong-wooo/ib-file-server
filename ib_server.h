#ifndef IB_SERVER_H
#define IB_SERVER_H

#include <sys/epoll.h>
#include "wthr.h"
#include "hash.h"
#include "ib.h"

#define FD_SETSIZE 1024

enum fd_type {
    SERVER_SOCKET,
    CLIENT_SOCKET,
    PIPE,
    CQ,
};

struct fd_info_s {
    int fd;
    enum fd_type type;
    void *ptr;
};

struct server_resources_s {
    int epoll_fd;
    socket_t sock;
    struct ib_handle_s *ib_handle;
    int pipefd[2];
    struct queue_s *queue;
    struct hash_map_s *qp_map;
};

struct server_resources_s *create_server_resources(void);
int poll_event(int epoll_fd, struct epoll_event *events);
void accept_ib_client(struct server_resources_s *res);
void poll_completion(struct server_resources_s *res);
void send_response(struct fd_info_s *fd_info, struct hash_map_s *qp_map);
void disconnect_client(struct fd_info_s *fd_info, struct hash_map_s *qp_map);
void destroy_res(struct server_resources_s *res);
#endif