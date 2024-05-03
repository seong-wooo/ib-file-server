#ifndef TEST_CLIENT_H
#define TEST_CLIENT_H

#define TEST_REPS 100000
#define TEST_THREADS_COUNT 56
#define READ_BODY_SIZE 4000


struct test_args_s {
    int reps;
    struct packet_s *packet;
};

#endif