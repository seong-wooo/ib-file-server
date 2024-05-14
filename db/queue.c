#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "queue.h"

struct queue_s *create_queue(void) {
    struct queue_s *queue = (struct queue_s *)malloc(sizeof(struct queue_s));
    queue->front = NULL;
    queue->rear = NULL;

    return queue;
}

struct node_s *create_new_node(void *data) {
    struct node_s *new_node = (struct node_s *)malloc(sizeof(struct node_s));
    if (new_node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    new_node->data = data;
    new_node->next = NULL;

    return new_node;
}

void enqueue(struct queue_s *queue, void *data) {
    struct node_s *new_node = create_new_node(data);
    if (queue->front == NULL) {
        queue->front = new_node;
        queue->rear = new_node;
    } else {
        queue->rear->next = new_node;
        queue->rear = new_node;
    }
}

int is_empty(struct queue_s *queue) {
    return queue->front == NULL;
}

void *dequeue(struct queue_s *queue) {
    if (queue->front == NULL) {
        return NULL;
    }
    struct node_s *temp = queue->front;
    void *data = temp->data;
    queue->front = queue->front->next;
    free(temp);

    if (queue->front == NULL) {
        queue->rear = NULL;

    }
    return data;
}

void free_queue(struct queue_s *queue) {
    struct node_s *current = queue->front;
    while (current != NULL) {
        struct node_s *temp = current;
        current = current->next;
        free(temp);
    }
}
