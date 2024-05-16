#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include "tcp_ip.h"
#include "queue.h"
#include "control.h"

int create_epoll(void) {
    int epoll_fd = epoll_create(1);
    if (epoll_fd < 0) {
        perror("epoll_create()");
        exit(EXIT_FAILURE);
    }
    return epoll_fd;
}

int poll_event(int epoll_fd, struct epoll_event *events) {
    int ready_fd = epoll_wait(epoll_fd, events, FD_SETSIZE, -1);
    if (ready_fd < 0) {
        perror("epoll_wait()");
        exit(EXIT_FAILURE);
    }
    return ready_fd;
}


socket_t create_control_socket(int port) {
    socket_t sock = create_socket();
    bind_socket(sock, port);
    listen_socket(sock, SOMAXCONN);

    return sock;
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

struct ctl_resources_s *create_init_ctl_resources(void) {
    struct ctl_resources_s *res = 
        (struct ctl_resources_s *)malloc(sizeof(struct ctl_resources_s));
    
    res->epoll_fd = create_epoll();
    res->sock = create_control_socket(DB_PORT);
    res->ib_handle = create_ib_handle();
    register_event(res->epoll_fd, res->sock, DB_SOCKET, NULL);
    register_event(res->epoll_fd, res->ib_handle->cq_channel->fd, CQ, NULL);   

    return res;
}

void accept_client(struct ctl_resources_s *res) {
    socket_t client_sock = accept_socket(res->sock);
    struct conn_resource_s *conn_res = 
        (struct conn_resource_s *) dequeue(res->ib_handle->conn_res_pool);
    conn_res->socket = client_sock;
    post_receive(conn_res);
    register_event(res->epoll_fd, client_sock, CLIENT_SOCKET, conn_res);
}

double calc_usage_memory_percent(void) {
    FILE *file;
    char buffer[256];
    unsigned long long total_memory, free_memory;
    file = fopen("/proc/meminfo", "r");
    if (file == NULL) {
        perror("Error opening /proc/meminfo");
        return 1;
    }
    fgets(buffer, sizeof(buffer), file);
    sscanf(buffer, "MemTotal: %llu kB", &total_memory);

    fgets(buffer, sizeof(buffer), file);
    sscanf(buffer, "MemFree: %llu kB", &free_memory);

    fclose(file);

    unsigned long long used_memory = total_memory - free_memory;
    return ((double)used_memory / total_memory) * 100;
}

void send_tcp_response(struct conn_resource_s *conn_res) {
    struct packet_s *packet = deserialize_packet(conn_res->mr->addr);

    int rc = send(conn_res->socket, conn_res->mr->addr, 
        sizeof(struct packet_header_s) + packet->header.body_size, 0);

    if (rc == SOCKET_ERROR) {
        perror("send()");
        exit(EXIT_FAILURE);
    } 
    free_packet(packet);
}

void handle_client(struct fd_info_s *fd_info, struct ctl_resources_s *res) {
    void *buf = malloc(MESSAGE_SIZE);
    int rc = recv(fd_info->fd, buf, MESSAGE_SIZE, 0);
    if (rc == SOCKET_ERROR) {
        perror("recv()");
        exit(EXIT_FAILURE);
    }
    else if (rc == 0) {
        epoll_ctl(res->epoll_fd, EPOLL_CTL_DEL, fd_info->fd, NULL);
        close_socket(fd_info->fd);
        enqueue(res->ib_handle->conn_res_pool, (struct conn_resource_s *)fd_info->ptr);
        free(fd_info);
    }
    else {
        struct conn_resource_s *conn_res = (struct conn_resource_s *)fd_info->ptr;
        struct packet_s *packet = (struct packet_s *) deserialize_packet(buf);
        post_send(conn_res, packet);
    }
}

void poll_completion(struct ctl_resources_s *res) {
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
        
        struct conn_resource_s *conn_res = (struct conn_resource_s *)wc.wr_id;
        switch (wc.opcode) {
            case IBV_WC_SEND:
                break;
            case IBV_WC_RECV:
                send_tcp_response(conn_res);
                post_receive(conn_res);
                break;
            default:
                break;
        }
    } while (rc);

    ibv_ack_cq_events(event_cq, 1);
    notify_cq(res->ib_handle->cq);
}