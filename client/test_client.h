#ifndef TEST_CLIENT_H
#define TEST_CLIENT_H

#define TEST_REPS 1000
#define TEST_THREADS_COUNT 50
#define READ_BODY_SIZE 4000


struct test_args_s {
    int reps;
    struct packet_s *packet;
};

#endif