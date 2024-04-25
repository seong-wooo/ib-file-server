#ifndef WTHR_H
#define WTHR_H
#include "message.h"
#include "queue.h"

#define MAX_THREADS 8

struct job_s {
    struct ib_resources_s *ib_res;
    struct packet_s *packet;
};

struct pipe_response_s {
    struct ib_resources_s *ib_res;
    struct packet_s packet;
};

void enqueue_job(struct queue_s *queue, struct job_s* job);
void init_wthr_pool(struct queue_s *queue, int pipefd);
#endif