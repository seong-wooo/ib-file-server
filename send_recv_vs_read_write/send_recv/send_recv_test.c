#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include "./client/ib.h"

struct timeval tv;
double begin, end;

struct multi_thread_arg_s {
    int data_size;
    int transmission_count;
};

void start_test() {
    gettimeofday(&tv, NULL);
	begin = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
}

void end_test() {
    gettimeofday(&tv, NULL);
	end = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
    printf("Execution time %f\n", (end - begin) / 1000);
}


void single_thread_client_test(int data_size, int transmission_count) {
    struct ib_handle_s *ib_handle = create_ib_handle();
    struct ib_resources_s *ib_res = connect_ib_server(ib_handle);
    
    memcpy(ib_handle->mr->addr, &data_size, sizeof(int));
    memset(ib_handle->mr->addr + sizeof(int), 'a', data_size);
    
    start_test();
    for (int i = 0; i < transmission_count; i++) {
        post_receive(ib_res->qp, ib_handle->mr);
        post_send(ib_handle->mr, ib_res->qp, data_size);
        poll_completion(ib_handle, 2);
    }
    end_test();

    destroy_ib_resource(ib_res);
}

void *multi_thread_client(void *arg) {
    struct multi_thread_arg_s *args = (struct multi_thread_arg_s *)arg;
    struct ib_handle_s *ib_handle = create_ib_handle();
    struct ib_resources_s *ib_res = connect_ib_server(ib_handle);
    
    memcpy(ib_handle->mr->addr, &args->data_size, sizeof(int));
    memset(ib_handle->mr->addr + sizeof(int), 'a', args->data_size);
    
    for (int i = 0; i < args->transmission_count; i++) {
        post_receive(ib_res->qp, ib_handle->mr);
        post_send(ib_handle->mr, ib_res->qp, args->data_size);
        poll_completion(ib_handle, 2);
    }

    destroy_ib_resource(ib_res);
}

void multi_thread_client_test(int data_size, int transmission_count) {
    int threads_count = 50;
    pthread_t threads[threads_count];
    struct multi_thread_arg_s args = {
        .data_size = data_size,
        .transmission_count = transmission_count,
    };
    
    start_test();
    for(int i = 0; i < threads_count; i++)
        pthread_create(&threads[i], NULL, multi_thread_client, &args);
    for(int i = 0; i < threads_count; i++)
        pthread_join(threads[i], NULL);
    end_test();
}

int main(int argc, char const *argv[]) {
    int data_size = atoi(argv[1]);
    int transmission_count = atoi(argv[2]);
    
    printf("read/write 테스트 시작 (싱글 스레드)\n");
    transmission_count = 100000;
    for (data_size = 1; data_size <= 1000000; data_size *= 10) {
        printf("data_size: %d\n", data_size);
        single_thread_client_test(data_size, transmission_count);
    }
    data_size = 2000000;
    single_thread_client_test(data_size, transmission_count);
    
    data_size = 4000000;
    single_thread_client_test(data_size, transmission_count);

    transmission_count = 10000;
    printf("read/write 테스트 시작 (멀티 스레드) \n");
    for (data_size = 1; data_size <= 1000000; data_size *= 10) {
        printf("data_size : %d\n", data_size);
        multi_thread_client_test(data_size, transmission_count);
    }

    data_size = 2000000;
    multi_thread_client_test(data_size, transmission_count);

    data_size = 4000000;
    multi_thread_client_test(data_size, transmission_count);
}