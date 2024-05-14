#include <sys/epoll.h>
#include "ib.h"

#define IB_SERVER_PORT 9001
#define DB_PORT 9002

enum fd_type {
    DB_SOCKET,
    CLIENT_SOCKET,
    CQ,
};

struct fd_info_s {
    int fd;
    enum fd_type type;
    void *ptr;
};

struct db_resources_s {
    int epoll_fd;
    socket_t sock;
    struct ib_handle_s *ib_handle;
};

int poll_event(int epoll_fd, struct epoll_event *events);
struct db_resources_s *create_init_db_resources(void);
void accept_client(struct db_resources_s *res);
void handle_client(struct fd_info_s *fd_info, struct db_resources_s *res);
void poll_completion(struct db_resources_s *res);