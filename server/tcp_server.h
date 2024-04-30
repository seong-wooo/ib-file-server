#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "server.h"

struct tcp_server_resources_s {
    int epoll_fd;
    socket_t sock;
    int pipefd[2];
    struct queue_s *queue;
};

void tcp_server(void);

#endif