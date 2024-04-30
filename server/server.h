#ifndef SERVER_H
#define SERVER_H
#include <sys/epoll.h>
#include "tcp_ip.h"


#define IB 'i'
#define TCP 't'

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

int create_epoll(void);
void create_pipe(int pipefd[2]);
void register_event(int epoll_fd, int registered_fd, enum fd_type type, void *ptr);
int poll_event(int epoll_fd, struct epoll_event *events);
socket_t create_server_socket(void);

#endif