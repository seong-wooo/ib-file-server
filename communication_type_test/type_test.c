#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include "./client/ib_client.h"

struct timeval tv;
double begin, end;

struct multi_thread_arg_s {
    int data_size;
    int transmission_count;
    int type;
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


void single_thread_client_test(int data_size, int transmission_count, int type) {
    struct client_resources_s *client_res = create_client_resources(data_size, type);
    start_test();
    execute_request(client_res, data_size, type, transmission_count);
    end_test();
    destroy_client_resources(client_res);
}

void *multi_thread_client(void *arg) {
    struct multi_thread_arg_s *args = (struct multi_thread_arg_s *)arg;
    client(args->data_size, args->transmission_count, args->type);
}

void multi_thread_client_test(int data_size, int transmission_count, int type) {
    int threads_count = 50;
    pthread_t threads[threads_count];
    struct multi_thread_arg_s args = {
        .data_size = data_size,
        .transmission_count = transmission_count,
        .type = type,
    };
    
    start_test();
    for(int i = 0; i < threads_count; i++)
        pthread_create(&threads[i], NULL, multi_thread_client, &args);
    for(int i = 0; i < threads_count; i++)
        pthread_join(threads[i], NULL);
    end_test();
}



int main(void) {
    
    int transmission_count = 10000000;
    printf("single thread test start\n");
    for (int type = 3; type <= 4; type++) {
        for (int data_size = 64; data_size <= 4194304; data_size *= 2) {
            printf("Type: %d, Data size: %d\n", type, data_size);
            single_thread_client_test(data_size, transmission_count, type);
        }
    }
    printf("single thread test end\n");

    int transmission_count = 10000;
    printf("multi thread test start\n");
    for (int type = 1; type <= 4; type++) {
        for (int data_size = 64; data_size <= 4194304; data_size *= 2) {
            printf("Type: %d, Data size: %d\n", type, data_size);
            multi_thread_client_test(data_size, transmission_count, type);
        }
    }
    printf("multi thread test end\n");
}