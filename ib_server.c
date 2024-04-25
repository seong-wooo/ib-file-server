#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include "ib_server.h"

int create_epoll(void) {
    int epoll_fd = epoll_create(1);
    if (epoll_fd < 0) {
        perror("epoll_create()");
        exit(EXIT_FAILURE);
    }
    return epoll_fd;
}

void create_pipe(int pipefd[2]) {
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
}

void register_event(int epoll_fd, int registered_fd, enum fd_type type, void *ptr) {
    struct fd_info_s *fd_info = (struct fd_info_s *)malloc(sizeof(struct fd_info_s));
    fd_info->fd = registered_fd;
    fd_info->type = type;
    fd_info->ptr = ptr;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = fd_info;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, registered_fd, &ev) < 0) { 
        perror("epoll_ctl()");
        exit(EXIT_FAILURE);
    }
}

struct server_resources_s *create_server_resources(void) {
    struct server_resources_s *res = (struct server_resources_s *)malloc(sizeof(struct server_resources_s));
    res->epoll_fd = create_epoll();
    res->sock = create_server_socket();
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

int poll_event(int epoll_fd, struct epoll_event *events) {
    int ready_fd = epoll_wait(epoll_fd, events, FD_SETSIZE, -1);
        if (ready_fd < 0) {
            perror("epoll_wait()");
            exit(EXIT_FAILURE);
        }
    return ready_fd;
}

void accept_ib_client(struct server_resources_s *res) {
    struct ib_resources_s *ib_res = create_init_ib_resources(res->ib_handle);
    
    ib_res->sock = accept_socket(res->sock);
    if (recv_qp_sync_data(ib_res) < 0) {
        exit(EXIT_FAILURE);
    }
    modify_qp_to_init(ib_res);
    modify_qp_to_rtr(ib_res);
    modify_qp_to_rts(ib_res);
    
    post_receive(ib_res);
    send_qp_sync_data(ib_res);

    register_event(res->epoll_fd, ib_res->sock, CLIENT_SOCKET, ib_res);
    put(res->qp_map, &ib_res->qp->qp_num, ib_res);
}

void send_response(struct fd_info_s *fd_info) {
    struct pipe_response_s *response;
    read(fd_info->fd, &response, sizeof(&response));

    post_send(response->ib_res, &response->packet);
    free(response);
}

void disconnect_client(struct fd_info_s *fd_info) {
    char buf[256];
    int rc = recv(fd_info->fd, buf, sizeof(buf), 0);
    if (rc == 0) {
        struct ib_resources_s *ib_res = (struct ib_resources_s *)fd_info->ptr;
        destroy_ib_resource(fd_info->ptr);
        free(fd_info);
    }
}

struct ib_resources_s *get_ib_resources(struct hash_map_s *qp_map, uint32_t qp_num) {
    struct ib_resources_s * ib_res = (struct ib_resources_s *) get(qp_map, &qp_num);
    if (ib_res == NULL) {
        fprintf(stderr, "Failed to get ib_res\n");
        exit(EXIT_FAILURE);
    }
    return ib_res;
}

void send_job(struct queue_s *queue, struct ib_resources_s *ib_res) {
    struct job_s *job = (struct job_s *)malloc(sizeof(struct job_s));
    struct packet_s *packet = create_response_packet(ib_res->mr->addr);
    
    job->ib_res = ib_res;
    job->packet = packet;
    enqueue_job(queue, job);
}

void poll_completion(struct server_resources_s *res) {
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
            fprintf(stderr, "Failed status %s (%d) for wr_id %d\n", ibv_wc_status_str(wc.status), wc.status, (int)wc.wr_id);
            exit(EXIT_FAILURE);
        }

        struct ib_resources_s *ib_res = get_ib_resources(res->qp_map, wc.qp_num);
        switch (wc.opcode) {
            case IBV_WC_RECV:
                send_job(res->queue, ib_res);
                break;
            
            case IBV_WC_SEND:
                post_receive(ib_res);
                break;

            default:
                break;
        }

    } while (rc == 0);

    ibv_ack_cq_events(event_cq, 1);
    notify_cq(res->ib_handle->cq);
}

void destroy_res(struct server_resources_s *res) {
    destroy_ib_handle(res->ib_handle);
    close(res->sock);
    close(res->pipefd[0]);
    close(res->pipefd[1]);
    free_queue(res->queue);
    free_hash_map(res->qp_map);
}