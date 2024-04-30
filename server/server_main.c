
#include <sys/epoll.h>
#include <sys/select.h>
#include "ib_server.h"
#include "server.h"

int main(int argc, char const *argv[]) {
    struct ib_server_resources_s *res = create_ib_server_resources();
    
    int ready_fd;
    struct epoll_event events[FD_SETSIZE];
    while (1) {
        ready_fd = poll_event(res->epoll_fd, events);
        for (int i = 0; i < ready_fd; i++) {
            struct fd_info_s *fd_info = (struct fd_info_s *)events[i].data.ptr;

            switch (fd_info->type) {
                case SERVER_SOCKET:
                    accept_ib_client(res);
                    break;

                case CQ:
                    poll_completion(res);
                    break;
                
                case PIPE:
                    send_response(fd_info, res->qp_map);
                    break;
                
                case CLIENT_SOCKET:
                    disconnect_client(fd_info, res->qp_map);
                    break;
                default:
                    break;
            }
        }
    }
    destroy_res(res);
    printf("[TCP 서버] 서버 종료\n");

    return 0;
}