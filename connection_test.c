#include <stdlib.h>
#include <pthread.h>
#include "./client/tcp_ip.h"
#include "test.h"

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
    socket_t sock = connect_tcp_to_server(LOCAL_IP, DB_PORT);
    void *buffer = malloc(MESSAGE_SIZE);
    for (int i = 0; i < count; i++) {
        send_packet(sock, &test_packet, buffer);
        struct packet_s *packet = recv_packet(sock, buffer);
        free_packet(packet);
    }
    close_socket(sock);
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
    struct test_param_s param = {
        .count = 100000,
        .thread_count = 50
    };
    // test_func(tcp_one_thread_request_test, &param, "tcp_one_thread_request_test");
    // test_func(ib_one_thread_request_test, &param, "ib_one_thread_request_test");

    test_func(tcp_threads_request_test, &param, "tcp_threads_request_test");
    test_func(ib_threads_request_test, &param, "ib_threads_request_test");


    return 0;
}