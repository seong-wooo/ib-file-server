#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "client.h"
#include "ib_client.h"
#include "tcp_client.h"

void check_time(time_t *current, const char *msg) {
    time(current);
    printf("%s: %s", msg, ctime(current));
}

struct packet_s *create_test_read_packet(void) {
    struct packet_s *packet = (struct packet_s *) calloc(1, sizeof(struct packet_s));
    packet->header.option = READ;
    strcpy(packet->header.filename, "ib.c");
    packet->header.length = READ_BODY_SIZE;
    packet->body.data = NULL;
    return packet;
}

void *test_client(void *func(void*), void* arg, char *msg) {
    time_t start, end;
    double diff;
    
    printf("%s 테스트 시작\n", msg);
    check_time(&start, "start");
    pthread_t threads[TEST_THREADS_COUNT];
    for (int i = 0; i < TEST_THREADS_COUNT; i++) {
        pthread_create(&threads[i], NULL, func, arg);
    }

    for (int i = 0; i < TEST_THREADS_COUNT; ++i) {
        int result_code = pthread_join(threads[i], NULL);
        if (result_code) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }
    check_time(&end, "end");
    diff = difftime(end, start);
    printf("(%s) 반복 횟수 %d, 걸린 시간: %f s\n", msg, TEST_REPS, diff);
}

int main(int argc, char const *argv[])
{   
    if (argc < 2) {
        printf("실행 시 인자를 입력해주세요. IB: %c, TCP: %c\n", IB, TCP);
        return 1;
    }
    if (*argv[1] == IB) {
        ib_client();
    }
    else if (*argv[1] == TCP) {
        tcp_client();
    }
    else if (*argv[1] == READ_TEST) {
        printf("READ_TEST\n");
        struct packet_s *packet = create_test_read_packet();
        struct test_args_s test_args = {TEST_REPS, packet};
        test_client(tcp_test_client, &test_args, "tcp");
        test_client(ib_test_client, &test_args, "ib");

        free_packet(packet);
    }
    else {
        printf("인자가 올바르지 않습니다.\n");
        return 1;
    }
    return 0;
}