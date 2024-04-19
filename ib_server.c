#include "wthr.h"

void create_pipe(int* pipefd);
void free_response(struct pipe_response_s *response);

int main(int argc, char const *argv[])
{
    int pipefd[2];
    create_pipe(pipefd);

    struct queue_s queue;
    init_wthr_pool(&queue, pipefd[1]);

    struct ib_handle_s ib_handle;
    create_ib_handle(&ib_handle);
    socket_t server_sock = create_server_socket();
    struct ib_resources_s *ib_res = accept_ib_client(server_sock, &ib_handle);
    
    while (1) {
        post_receive(ib_res);
        poll_completion(&ib_handle);
        struct job_s job = {ib_res, ib_res->mr_addr};
        enqueue(&queue, &job);

        // 파이프에서 꺼내서 처리 -> epoll 방식으로 처리
        sleep(1);

        struct pipe_response_s *response;
        read(pipefd[0], &response, sizeof(&response));
        strcpy(ib_res->mr_addr, response->body);
        post_send(ib_res);
        poll_completion(&ib_handle);
        free_response(response);
    }
    destroy_resource(ib_res);
    destroy_handle(&ib_handle);
    close(server_sock);
    printf("[TCP 서버] 서버 종료\n");

    return 0;
}

void create_pipe(int* pipefd) {
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
}
void free_response(struct pipe_response_s *response) {
    free(response->body);
    free(response);
}