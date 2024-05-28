#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include "ib_server.h"
#include "hash.h"
#include "err_check.h"

struct ib_server_resources_s *create_ib_server_resources(void) {
    struct ib_server_resources_s *res = 
        (struct ib_server_resources_s *)malloc(sizeof(struct ib_server_resources_s));
    check_null(res, "malloc()");
    
    res->epoll_fd = create_epoll();
    res->sock = create_server_socket(IB_SERVER_PORT);
    res->qp_map = create_hash_map(1000);
    res->ib_handle = create_ib_handle();

    register_event(res->epoll_fd, res->sock, SERVER_SOCKET, NULL);
    register_event(res->epoll_fd, res->ib_handle->cq_channel->fd, CQ, NULL);
    return res;
}

void accept_ib_client(struct ib_server_resources_s *res) {
    struct ib_resources_s *ib_res = create_init_ib_resources(res->ib_handle, res->sock);

    struct connection_data_s remote_data = recv_qp_data(ib_res->sock);
    modify_qp_to_init(ib_res->qp);
    modify_qp_to_rtr(ib_res->qp, remote_data);
    modify_qp_to_rts(ib_res->qp);

    struct connection_data_s conn_data = (struct connection_data_s) {
        .qp_num = ib_res->qp->qp_num,
        .lid = res->ib_handle->lid,
    };

    send_qp_data(ib_res->sock, conn_data);

    register_event(res->epoll_fd, ib_res->sock, CLIENT_SOCKET, ib_res);
    put(res->qp_map, ib_res->qp->qp_num, ib_res->qp);
}

void disconnect_client(struct fd_info_s *fd_info, struct hash_map_s *qp_map) {
    char buf[256];
    int rc = recv(fd_info->fd, buf, sizeof(buf), 0);
    if (rc == 0) {
        struct ib_resources_s *ib_res = (struct ib_resources_s *)fd_info->ptr;
        remove_key(qp_map, ib_res->qp->qp_num);
        destroy_ib_resource(fd_info->ptr);
        free(fd_info);
    }
}

void poll_completion(struct ib_server_resources_s *res) {
    struct ibv_cq *event_cq;
    void *event_cq_ctx;
    struct ibv_wc wc[MAX_WR];
    
    int rc = ibv_get_cq_event(res->ib_handle->cq_channel, &event_cq, &event_cq_ctx);
    if (rc) {
        perror("ibv_get_cq_event");
        exit(EXIT_FAILURE);
    }

    do {
        rc = ibv_poll_cq(event_cq, MAX_WR, wc);
        if (rc == 0) {
            continue;
        }
        check_error(rc, "ibv_poll_cq");
        
        for (int i = 0; i < rc; i++) { 
            if (wc[i].status != IBV_WC_SUCCESS) {
                perror("Completion error");
                exit(EXIT_FAILURE);
            }
        
            struct ibv_mr *mr = (struct ibv_mr *)wc[i].wr_id;
            switch (wc[i].opcode) {
                case IBV_WC_RECV:
                    post_send((struct ibv_qp *)get(res->qp_map, wc[i].qp_num), mr);
                    break;
                
                case IBV_WC_SEND:
                    post_receive(res->ib_handle->srq, mr);
                    break;

                default:
                    break;
            }
        }
    } while (rc);

    ibv_ack_cq_events(event_cq, 1);
    notify_cq(res->ib_handle->cq);
}

void destroy_res(struct ib_server_resources_s *res) {
    destroy_ib_handle(res->ib_handle);
    close(res->sock);
    free_hash_map(res->qp_map);
}

void send_receive_server(void) {
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
                
                case CLIENT_SOCKET:
                    disconnect_client(fd_info, res->qp_map);
                    break;
                default:
                    break;
            }
        }
    }
    destroy_res(res);
    printf("[IB 서버] 서버 종료\n");
}