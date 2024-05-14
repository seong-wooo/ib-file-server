#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include "db.h"

int main(int argc, char const *argv[]) {
    struct db_resources_s *res = create_init_db_resources();
    
    int ready_fd;
    struct epoll_event events[FD_SETSIZE];
    while (1) {
        ready_fd = poll_event(res->epoll_fd, events);
        for (int i = 0; i < ready_fd; i++) {
            struct fd_info_s *fd_info = (struct fd_info_s *)events[i].data.ptr;
            switch (fd_info->type) {
                case DB_SOCKET:
                    accept_client(res);
                    break;
                
                case CLIENT_SOCKET:
                    handle_client(fd_info, res);
                    break;

                case CQ:
                    poll_completion(res);
                    break;
                
                default:
                    break;
            }
        }
    }

    printf("[DB 서버] 서버 종료\n");
    return 0;
}
