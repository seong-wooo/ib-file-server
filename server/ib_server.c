#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include "ib_server.h"

struct ib_server_resources_s *create_ib_server_resources(void) {
    struct ib_server_resources_s *res = 
        (struct ib_server_resources_s *)malloc(sizeof(struct ib_server_resources_s));
    res->epoll_fd = create_epoll();
    res->sock = create_server_socket(IB_SERVER_PORT);
    res->qp_map = create_hash_map(1000);
    create_pipe(res->pipefd);
    res->queue = create_queue();
    init_wthr_pool(res->queue, res->pipefd[1]);
    res->ib_handle = create_ib_handle();

    register_event(res->epoll_fd, res->sock, SERVER_SOCKET, NULL);
    register_event(res->epoll_fd, res->pipefd[0], PIPE, NULL);
    register_event(res->epoll_fd, res->ib_handle->cq_channel->fd, CQ, NULL);
    return res;
}

void accept_ib_client(struct ib_server_resources_s *res) {
    struct ib_resources_s *ib_res = create_init_ib_resources(res->ib_handle, res->sock);

    recv_qp_sync_data(ib_res);
    modify_qp_to_init(ib_res);
    modify_qp_to_rtr(ib_res);
    modify_qp_to_rts(ib_res);
    send_qp_sync_data(ib_res);

    register_event(res->epoll_fd, ib_res->sock, CLIENT_SOCKET, ib_res);
    put(res->qp_map, ib_res->qp->qp_num, ib_res->qp);
}

void send_ib_response(struct fd_info_s *fd_info, struct hash_map_s *qp_map, struct queue_s *mr_pool) {
    struct job_s *job;
    read(fd_info->fd, &job, sizeof(&job));
    struct ib_meta_data_s *meta_data = (struct ib_meta_data_s *)job->meta_data;
    struct ibv_qp* qp = (struct ibv_qp *)get(qp_map, meta_data->qp_num);
    post_send(meta_data->mr, qp, job->packet);
    free(job);
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

void send_job(struct queue_s *queue, struct ibv_mr *mr, uint32_t qp_num) {
    struct job_s *job = (struct job_s *)malloc(sizeof(struct job_s));
    struct packet_s *packet = deserialize_packet(mr->addr);
    struct ib_meta_data_s *meta_data = 
        (struct ib_meta_data_s *)malloc(sizeof(struct ib_meta_data_s));
    meta_data->qp_num = qp_num;
    meta_data->mr = mr;

    job->meta_data = meta_data;
    job->packet = packet;
    enqueue_job(queue, job);
}

void poll_completion(struct ib_server_resources_s *res) {
    struct ibv_cq *event_cq;
    void *event_cq_ctx;
    struct ibv_wc wc;
    
    int rc = ibv_get_cq_event(res->ib_handle->cq_channel, &event_cq, &event_cq_ctx);
    if (rc) {
        perror("ibv_get_cq_event");
        exit(EXIT_FAILURE);
    }

    do {
        rc = ibv_poll_cq(event_cq, 1, &wc);
        if (rc == 0) {
            continue;
        }
        else if (rc < 0) {
            perror("ibv_poll_cq");
            exit(EXIT_FAILURE);
        }
        

        if (wc.status != IBV_WC_SUCCESS) {
            perror("Completion error");
            exit(EXIT_FAILURE);
        }
        
        struct ibv_mr *mr = (struct ibv_mr *)wc.wr_id;
        switch (wc.opcode) {
            case IBV_WC_RECV:
                send_job(res->queue, mr, wc.qp_num);
                break;
            
            case IBV_WC_SEND:
                post_receive(res->ib_handle->srq, mr);
                break;

            default:
                break;
        }

    } while (rc);

    ibv_ack_cq_events(event_cq, 1);
    notify_cq(res->ib_handle->cq);
}

void destroy_res(struct ib_server_resources_s *res) {
    destroy_ib_handle(res->ib_handle);
    close(res->sock);
    close(res->pipefd[0]);
    close(res->pipefd[1]);
    free_queue(res->queue);
    free_hash_map(res->qp_map);
}

void ib_server(void) {
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
                    send_ib_response(fd_info, res->qp_map, res->ib_handle->mr_pool);
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