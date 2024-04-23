#include "wthr.h"
#include <sys/epoll.h>
#include <sys/select.h>

struct fd_info_s {
    int fd;
    void *ptr;
};

struct server_resources_s {
    int epoll_fd;
    socket_t sock;
    struct ib_handle_s ib_handle;
    int pipefd[2];
    struct queue_s queue;
    struct hash_map_s *qp_map;
};

int create_epoll(void);
void create_pipe(int pipefd[2]);
struct server_resources_s *create_server_resources(void);
void register_event(int epoll_fd, int registered_fd, void *fd_info);
int poll_event(int epoll_fd, struct epoll_event *events);
void poll_completion(struct server_resources_s *res);
void free_response(struct pipe_response_s *response);
void destroy_res(struct server_resources_s *res);

int main(int argc, char const *argv[])
{
    struct server_resources_s *res = create_server_resources();
    
    int ready_fd;
    struct epoll_event events[FD_SETSIZE];
    while (1) {
        ready_fd = poll_event(res->epoll_fd, events);
        for (int i = 0; i < ready_fd; i++) {
            struct fd_info_s *fd_info = (struct fd_info_s *)events[i].data.ptr;

            if (fd_info->fd == res->sock) {
                struct ib_resources_s *ib_res = accept_ib_client(res->sock, &res->ib_handle);
                register_event(res->epoll_fd, ib_res->sock, ib_res);
                put(res->qp_map, &ib_res->qp->qp_num, ib_res);
            }
            else if(fd_info->fd == res->ib_handle.cq_channel->fd) {
                poll_completion(res);
            }
            
            else if(fd_info->fd == res->pipefd[0]) {
                struct pipe_response_s *response;
                read(fd_info->fd, &response, sizeof(&response));
                strcpy(response->ib_res->mr_addr, response->body);
                post_send(response->ib_res);
                free_response(response);
            }
            else {
                char buf[256];
                int rc = recv(fd_info->fd, buf, sizeof(buf), 0);
                if (rc == 0) {
                    destroy_ib_resource(fd_info->ptr);
                    free(fd_info);
                }
            }
        }
    }
    destroy_res(res);
    printf("[TCP 서버] 서버 종료\n");

    return 0;
}

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

struct server_resources_s *create_server_resources(void) {
    struct server_resources_s *res = (struct server_resources_s *)malloc(sizeof(struct server_resources_s));
    res->epoll_fd = create_epoll();
    res->sock = create_server_socket();
    res->qp_map = create_hash_map(1000);
    create_pipe(res->pipefd);
    init_wthr_pool(&res->queue, res->pipefd[1]);
    create_ib_handle(&res->ib_handle);

    register_event(res->epoll_fd, res->sock, NULL);
    register_event(res->epoll_fd, res->pipefd[0], NULL);
    register_event(res->epoll_fd, res->ib_handle.cq_channel->fd, NULL);
    return res;
}

void register_event(int epoll_fd, int registered_fd, void *ptr) {
    struct fd_info_s *fd_info = (struct fd_info_s *)malloc(sizeof(struct fd_info_s));
    fd_info->fd = registered_fd;
    fd_info->ptr = ptr;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = fd_info;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, registered_fd, &ev) < 0) { 
        perror("epoll_ctl()");
        exit(EXIT_FAILURE);
    }
}

int poll_event(int epoll_fd, struct epoll_event *events) {
    int ready_fd = epoll_wait(epoll_fd, events, FD_SETSIZE, -1);
        if (ready_fd < 0) {
            perror("epoll_wait()");
            exit(EXIT_FAILURE);
        }
    return ready_fd;
}

void free_response(struct pipe_response_s *response) {
    free(response->body);
    free(response);
}

void poll_completion(struct server_resources_s *res) {
    int rc;
    struct ibv_cq *event_cq;
    void *event_cq_ctx;
    struct ibv_wc wc;
    
    rc = ibv_get_cq_event(res->ib_handle.cq_channel, &event_cq, &event_cq_ctx);
    if (rc) {
        perror("ibv_get_cq_event");
        exit(EXIT_FAILURE);
    }

    do {
        rc = ibv_poll_cq(event_cq, 1, &wc);
        if (rc < 0) {
            perror("ibv_poll_cq");
            exit(EXIT_FAILURE);
        }
        
        if (rc == 0) {
            continue;
        }

        if (wc.status != IBV_WC_SUCCESS) {
            fprintf(stderr, "Failed status %s (%d) for wr_id %d\n", ibv_wc_status_str(wc.status), wc.status, (int)wc.wr_id);
            exit(EXIT_FAILURE);
        }

        struct ib_resources_s *ib_res = (struct ib_resources_s *) get(res->qp_map, &wc.qp_num);
        if (ib_res == NULL) {
            fprintf(stderr, "Failed to get ib_res\n");
            exit(EXIT_FAILURE);
        }

        if (wc.opcode == IBV_WC_RECV) {
            struct job_s job = {ib_res, ib_res->mr_addr};
            enqueue(&res->queue, &job);
        }

        if (wc.opcode == IBV_WC_SEND) {
            post_receive(ib_res);
        }

    } while (rc == 0);

    ibv_ack_cq_events(event_cq, 1);

    rc = ibv_req_notify_cq(res->ib_handle.cq, 0);
    if (rc) {
        perror("ibv_req_notify_cq");
        exit(EXIT_FAILURE);
    }
}

void destroy_res(struct server_resources_s *res) {
    destroy_ib_handle(&res->ib_handle);
    close(res->sock);
    close(res->pipefd[0]);
    close(res->pipefd[1]);
    freeQueue(&res->queue);
    free(res);
}