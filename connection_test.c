#include "./client/tcp_ip.h"
#include "./client/ib.h"
#include "test.h"
#include <pthread.h>

struct test_param_s {
    int count;
    int thread_count;
};

struct packet_s test_packet = {
    .header = {
        .option = READ,
        .filename = "testfile",
        .length = 4000,
    },
    .body = {
        .data = NULL
    }
};

void *tcp_one_thread_connection_test(void *arg) {
    int count = *(int *)arg;
    printf("TCP 연결 %d번 반복\n", count);
    for (int i = 0; i < count; i++) {
        socket_t socket = connect_tcp_to_server(SERVER_IP, TCP_SERVER_PORT);
        close_socket(socket);
    }
}

void *ib_one_thread_connection_test(void *arg) {
    int count = *(int *)arg;
    printf("IB 연결 %d번 반복\n", count);
    for (int i = 0; i < count; i++) {
        struct ib_handle_s *ib_handle = create_ib_handle();
        struct ib_resources_s *ib_res = connect_ib_server(ib_handle);

        destroy_ib_resource(ib_res);
        destroy_ib_handle(ib_handle);
    }
}

void *tcp_request_test(void *arg) {
    int count = *(int *)arg;
    socket_t sock = connect_tcp_to_server(SERVER_IP, TCP_SERVER_PORT);
    void *buffer = malloc(MESSAGE_SIZE);
    for (int i = 0; i < count; i++) {
        send_packet(sock, &test_packet, buffer);
        struct packet_s *packet = recv_packet(sock, buffer);
        free_packet(packet);
    }
    close_socket(sock);
}

void *tcp_one_thread_request_test(void *arg) {
    struct test_param_s *param = (struct test_param_s *)arg;
    tcp_request_test(&param->count);
}

void *ib_request_test(void *arg) {
    int count = *(int *)arg;
    struct ib_handle_s *ib_handle = create_ib_handle();
    struct ib_resources_s *ib_res = connect_ib_server(ib_handle);
    for (int i = 0; i < count; i++) {
        post_receive(ib_res->qp, ib_handle->mr);
        post_send(ib_handle->mr, ib_res->qp, &test_packet);

        poll_completion(ib_handle); 
        poll_completion(ib_handle);

        struct packet_s *packet = deserialize_packet(ib_handle->mr->addr);
        free_packet(packet);
    }
    destroy_ib_resource(ib_res);
    destroy_ib_handle(ib_handle);
}

void *ib_one_thread_request_test(void *arg) {
    struct test_param_s *param = (struct test_param_s *)arg;
    ib_request_test(&param->count);
}

void *ib_threads_request_test(void *arg) {
    struct test_param_s *param = (struct test_param_s *)arg;
    pthread_t thread[param->thread_count];
    for (int i = 0; i < param->thread_count; i++) {
        pthread_create(&thread[i], NULL, ib_request_test, &param->count);
    }
    for (int i = 0; i < param->thread_count; i++) {
        pthread_join(thread[i], NULL);
    }
}

void *tcp_threads_request_test(void *arg) {
    struct test_param_s *param = (struct test_param_s *)arg;
    pthread_t thread[param->thread_count];
    for (int i = 0; i < param->thread_count; i++) {
        pthread_create(&thread[i], NULL, tcp_request_test, &param->count);
    }
    for (int i = 0; i < param->thread_count; i++) {
        pthread_join(thread[i], NULL);
    }
}

int main(void) {
    int count = 500;
    test_func(tcp_one_thread_connection_test, &count, "tcp connection test");
    test_func(ib_one_thread_connection_test, &count, "qp connection test");

    struct test_param_s param = {
        .count = 100000,
        .thread_count = 50
    };
    test_func(tcp_one_thread_request_test, &param, "tcp_one_thread_request_test");
    test_func(ib_one_thread_request_test, &param, "ib_one_thread_request_test");

    test_func(tcp_threads_request_test, &param, "tcp_threads_request_test");
    test_func(ib_threads_request_test, &param, "ib_threads_request_test");


    return 0;
}