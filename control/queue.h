#ifndef QUEUE_H
#define QUEUE_H

struct node_s {
    void *data;
    struct node_s *next;
};

struct queue_s {
    struct node_s *front;
    struct node_s *rear;
};

struct queue_s *create_queue(void);
void enqueue(struct queue_s *queue, void *data);
void *dequeue(struct queue_s *queue);
int is_empty(struct queue_s *queue);
void free_queue(struct queue_s *queue);

#endif