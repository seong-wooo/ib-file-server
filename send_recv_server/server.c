#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include "server.h"
#include "err_check.h"

int create_epoll(void) {
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    check_error(epoll_fd, "epoll_create()");

    return epoll_fd;
}

void register_event(int epoll_fd, int registered_fd, enum fd_type type, void *ptr) {
    struct fd_info_s *fd_info = (struct fd_info_s *)malloc(sizeof(struct fd_info_s));
    check_null(fd_info, "malloc()");

    fd_info->fd = registered_fd;
    fd_info->type = type;
    fd_info->ptr = ptr;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = fd_info;
    int rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, registered_fd, &ev);
    check_error(rc, "epoll_ctl()");
}

int poll_event(int epoll_fd, struct epoll_event *events) {
    int rc = epoll_wait(epoll_fd, events, FD_SETSIZE, -1);
    check_error(rc, "epoll_wait()");
    return rc;
}