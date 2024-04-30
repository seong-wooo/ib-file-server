#include <stdlib.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include "tcp_server.h"
#include "wthr.h"
#include "message.h"
#include "queue.h"

struct tcp_server_resources_s *create_tcp_server_resources(void) {
    struct tcp_server_resources_s *res = (struct tcp_server_resources_s *)malloc(sizeof(struct tcp_server_resources_s));
    res->epoll_fd = create_epoll();
    res->sock = create_server_socket(TCP_SERVER_PORT);
    create_pipe(res->pipefd);
    res->queue = create_queue();
    init_wthr_pool(res->queue, res->pipefd[1]);

    register_event(res->epoll_fd, res->sock, SERVER_SOCKET, NULL);
    register_event(res->epoll_fd, res->pipefd[0], PIPE, NULL);

    return res;
}

void accept_tcp_client(struct tcp_server_resources_s *res) {
    socket_t client_sock = accept_socket(res->sock);
    void *buf = malloc(MESSAGE_SIZE);
    register_event(res->epoll_fd, client_sock, CLIENT_SOCKET, buf);
}

void handle_client(struct fd_info_s *fd_info, struct tcp_server_resources_s *res) {
    int rc = recv(fd_info->fd, fd_info->ptr, MESSAGE_SIZE, 0);
    if (rc == SOCKET_ERROR) {
        perror("recv()");
        exit(EXIT_FAILURE);
    }
    else if (rc == 0) {
        epoll_ctl(res->epoll_fd, EPOLL_CTL_DEL, fd_info->fd, NULL);
        close_socket(fd_info->fd);
        free(fd_info->ptr);
        free(fd_info);
    }
    else {
        struct job_s *job = (struct job_s *)malloc(sizeof(struct job_s));
        job->meta_data = fd_info;
        job->packet = deserialize_packet(fd_info->ptr);
        enqueue_job(res->queue, job);
    }
}


void send_tcp_response(struct fd_info_s *fd_info) {
    struct job_s *job;
    read(fd_info->fd, &job, sizeof(&job));
    struct packet_s *packet = (struct packet_s *)job->packet;
    
    struct fd_info_s *client_fd_info = (struct fd_info_s *)job->meta_data;
    serialize_packet(packet, client_fd_info->ptr);
    int rc = send(client_fd_info->fd, client_fd_info->ptr, sizeof(struct packet_header_s) + packet->header.body_size, 0);

    if (rc == SOCKET_ERROR) {
        perror("send()");
        exit(EXIT_FAILURE);
    } 
    free_packet(packet);
}


void tcp_server(void) {
    struct tcp_server_resources_s *res = create_tcp_server_resources();
    
    int ready_fd;
    struct epoll_event events[FD_SETSIZE];
    while (1) {
        ready_fd = poll_event(res->epoll_fd, events);
        for (int i = 0; i < ready_fd; i++) {
            struct fd_info_s *fd_info = (struct fd_info_s *)events[i].data.ptr;

            switch (fd_info->type) {
                case SERVER_SOCKET:
                    accept_tcp_client(res);
                    break;
                case CLIENT_SOCKET:
                    handle_client(fd_info, res);
                    break;
                case PIPE:
                    send_tcp_response(fd_info);
                    break;
                
                default:
                    break;
            }
        }
    }
    printf("[TCP 서버] 서버 종료\n");
}