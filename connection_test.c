#include "./client/tcp_ip.h"
#include "./client/ib.h"
#include "test.h"
#include <pthread.h>

struct test_param_s {
    int count;
    int thread_count;
    struct packet_s *packet;
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
    struct test_param_s *params = (struct test_param_s *)arg;
    socket_t sock = connect_tcp_to_server(SERVER_IP, TCP_SERVER_PORT);
    for (int i = 0; i < params->count; i++) {
        send_packet(sock, params->packet);
        recv_packet(sock, params->packet);
    }
    close_socket(sock);
}

void *tcp_one_thread_request_test(void *arg) {
    tcp_request_test(arg);
}

void *ib_request_test(void *arg) {
    struct test_param_s *params = (struct test_param_s *)arg;
    struct ib_handle_s *ib_handle = create_ib_handle();
    struct ib_resources_s *ib_res = connect_ib_server(ib_handle);
    for (int i = 0; i < params->count; i++) {
        post_receive(ib_res->qp, ib_handle->mr);
        post_send(ib_handle->mr, ib_res->qp, params->packet);

        poll_completion(ib_handle, 2); 

        struct packet_s *packet = deserialize_packet(ib_handle->mr->addr);
        free_packet(packet);
    }
    destroy_ib_resource(ib_res);
    destroy_ib_handle(ib_handle);
}

void *ib_one_thread_request_test(void *arg) {
    ib_request_test(arg);
}

void *ib_threads_request_test(void *arg) {
    struct test_param_s *param = (struct test_param_s *)arg;
    pthread_t thread[param->thread_count];
    for (int i = 0; i < param->thread_count; i++) {
        pthread_create(&thread[i], NULL, ib_request_test, param);
    }
    for (int i = 0; i < param->thread_count; i++) {
        pthread_join(thread[i], NULL);
    }
}

void *tcp_threads_request_test(void *arg) {
    struct test_param_s *param = (struct test_param_s *)arg;
    pthread_t thread[param->thread_count];
    for (int i = 0; i < param->thread_count; i++) {
        pthread_create(&thread[i], NULL, tcp_request_test, param);
    }
    for (int i = 0; i < param->thread_count; i++) {
        pthread_join(thread[i], NULL);
    }
}

void *tcp_total_request_test(void *arg) {
    struct test_param_s *params = (struct test_param_s *)arg;
    for (int i = 0; i < params->count; i++) {
        socket_t sock = connect_tcp_to_server(SERVER_IP, TCP_SERVER_PORT);
        send_packet(sock, params->packet);
        recv_packet(sock, params->packet);
        close_socket(sock);
    }
}


void *ib_total_request_test(void *arg) {
    struct test_param_s *params = (struct test_param_s *)arg;
    for (int i = 0; i < params->count; i++) {
        struct ib_handle_s *ib_handle = create_ib_handle();
        struct ib_resources_s *ib_res = connect_ib_server(ib_handle);
    
        post_receive(ib_res->qp, ib_handle->mr);
        post_send(ib_handle->mr, ib_res->qp, params->packet);

        poll_completion(ib_handle, 2); 

        struct packet_s *packet = deserialize_packet(ib_handle->mr->addr);
        free_packet(packet);
        destroy_ib_resource(ib_res);
        destroy_ib_handle(ib_handle);
    }
}



int main(void) {
    struct packet_s test_packet = {
        .header = {
            .option = READ,
            .filename = "ib.c",
            .length = 14,
        }
    };

    test_packet.body.data = malloc(MESSAGE_SIZE - sizeof(struct packet_header_s));
    
    struct test_param_s param = {
        .count = 10000,
        .thread_count = 50,
        .packet = &test_packet,
    };

    // 1.  TCP 와 IB의 속도 차이 비교 
    test_func(tcp_one_thread_request_test, &param, "TCP 연결 후 데이터 통신 /50바이트");
    test_func(ib_one_thread_request_test, &param, "IB 연결 후 데이터 통신 / 50바이트");

    // 2. TCP 와 IB의 연결 해제 까지 합쳐서의 차이 비교
    test_func(tcp_total_request_test, &param, "TCP 연결 및 해제 및 데이터 통신 / 50바이트");
    test_func(ib_total_request_test, &param, "IB 연결 및 해제 및 데이터 통신 / 50바이트");

    // 3. 데이터 크기 2000바이트 
    test_packet.header.length = 2000;
    test_func(tcp_one_thread_request_test, &param, "TCP 연결 후 데이터 통신 / 2000바이트");
    test_func(ib_one_thread_request_test, &param, "TCP 연결 후 데이터 통신 / 2000바이트");
    
    test_func(tcp_total_request_test, &param, "TCP 연결 및 해제 및 데이터 통신 / 2000바이트");
    test_func(ib_total_request_test, &param, "IB 연결 및 해제 및 데이터 통신 / 2000바이트");

    // 3. 데이터 크기 변경 후 비교
    test_packet.header.length = 4059;
    test_func(tcp_one_thread_request_test, &param, "TCP 연결 후 데이터 통신 / 4096바이트");
    test_func(ib_one_thread_request_test, &param, "TCP 연결 후 데이터 통신 / 4096바이트");
    
    test_func(tcp_total_request_test, &param, "TCP 연결 및 해제 및 데이터 통신 / 4096바이트");
    test_func(ib_total_request_test, &param, "IB 연결 및 해제 및 데이터 통신 / 4096바이트");

    return 0;
}