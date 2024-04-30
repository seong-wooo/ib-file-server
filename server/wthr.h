#ifndef WTHR_H
#define WTHR_H
#include "queue.h"

#define MAX_THREADS 8

struct job_s {
    void *meta_data;
    struct packet_s *packet;
};

void enqueue_job(struct queue_s *queue, struct job_s* job);
void init_wthr_pool(struct queue_s *queue, int pipefd);
#endif