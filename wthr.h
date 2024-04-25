#ifndef WTHR_H
#define WTHR_H
#include "message.h"

#define MAX_THREADS 8
#define HASH_SIZE 10

struct job_s {
    struct ib_resources_s *ib_res;
    struct packet_s *packet;
};

struct node_s {
    struct job_s *job;
    struct node_s *next;
};

struct queue_s {
    struct node_s *front;
    struct node_s *rear;
};

struct pipe_response_s {
    struct ib_resources_s *ib_res;
    struct packet_s packet;
};

void enqueue(struct queue_s *queue, struct job_s* job);
void freeQueue(struct queue_s *queue);
void init_wthr_pool(struct queue_s *queue, int pipefd);
#endif