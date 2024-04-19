#include <pthread.h>
#include <dirent.h>
#include "ib.h"
#include "log.h"
#include "message.h"

#define MAX_THREADS 8
#define HASH_SIZE 10

struct job_s {
    struct ib_resources_s *ib_res;
    char *data;
};

struct node_s {
    struct job_s *job;
    struct node_s *next;
};

struct queue_s {
    struct node_s *front;
    struct node_s *rear;
    pthread_cond_t empty_cond;
    pthread_mutex_t mutex;
};

struct pipe_response_s {
    struct ib_resources_s *ib_res;
    char* body;
};

int cthred_pipefd;

void enqueue(struct queue_s *queue, struct job_s* job) {
    struct node_s *newNode = (struct node_s *)malloc(sizeof(struct node_s));
    if (newNode == NULL) {
        return;
    }
    newNode->job = job;
    newNode->next = NULL;

    pthread_mutex_lock(&queue->mutex);

    if (queue->rear == NULL) {
        queue->front = newNode;
        queue->rear = newNode;
    }
    else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
    
    pthread_cond_signal(&queue->empty_cond);
    pthread_mutex_unlock(&queue->mutex);
}


struct job_s* dequeue(struct queue_s *queue) {
    struct job_s* job = NULL;
    pthread_mutex_lock(&queue->mutex);
    while (queue->front == NULL) {
        pthread_cond_wait(&queue->empty_cond, &queue->mutex);
    }
    
    struct node_s *temp = queue->front;
    queue->front = queue->front->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    job = temp->job;
    free(temp);
    

    pthread_mutex_unlock(&queue->mutex);

    return job;
}

void freeQueue(struct queue_s *queue) {
    struct node_s *current = queue->front;
    while (current != NULL) {
        struct node_s *temp = current;
        current = current->next;
        free(temp);
    }
    pthread_mutex_destroy(&(queue->mutex));
}

void write_flie(struct hash_map_s *body, char *response)
{
    char *filename = get(body, FILENAME);
    int offset = atoi(get(body, OFFSET));
    char *data = get(body, DATA);
    char log_message[256];

    FILE *fp = fopen(filename, "rb+");
    if (fp == NULL) {
        fp = fopen(filename, "wb");
        if (fp == NULL) {
            perror("Error opening file");
        }
        offset = 0;
    }

    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    if (offset > fileSize) {
        offset = fileSize;
    }

    fseek(fp, offset, SEEK_SET);
    fwrite(data, sizeof(char), strlen(data), fp);

    snprintf(log_message, 256, "option=%c, filename=%s, offset=%d, data=%s", WRITE, filename, offset, data);
    write_log(LOG_INFO, log_message);

    fclose(fp);
    strcpy(response, "Data written successfully");
}

void read_file(struct hash_map_s *body, char *response)
{
    char *filename = get(body, FILENAME);
    int offset = atoi(get(body, OFFSET));
    int length = atoi(get(body, LENGTH));

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        strcpy(response, "Error opening file");
    }
    else
    {
        fseek(fp, 0, SEEK_END);
        long fileSize = ftell(fp);
        if (offset > fileSize) {
            strcpy(response, "Offset is greater than file size");
        }
        else {
            fseek(fp, offset, SEEK_SET);
            length = (offset + length > fileSize) ? fileSize - offset : length;
            fread(response, sizeof(char), length, fp);
            response[length] = '\0';
        }
        fclose(fp);
    }
}

void delete_file(struct hash_map_s *body, char *response)
{
    char *filename = get(body, FILENAME);
    if (unlink(filename) == 0) {
        char log_message[256];
        snprintf(log_message, 256, "option=%c, filename=%s", DELETE, filename);
        write_log(LOG_INFO, log_message);
        strcpy(response, "File deleted successfully");
    }
    else {
        strcpy(response, "Error deleting file");
    }
}

void read_list(struct hash_map_s *body, char *response)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir(".");
    if (dir == NULL) {
        perror("Error opening dir");
    }

    while ((entry = readdir(dir)) != NULL) {
        strcat(response, entry->d_name);
        strcat(response, "\n");
    }
    strcat(response, "\0");

    closedir(dir);
}

void create_response(struct job_s *job) {
    struct hash_map_s *body = parse_message(HASH_SIZE, job->data);
    char *option = get(body, OPTION);
    char buf[QP_BUF_SIZE];
    switch (*option) {
    case WRITE:
        write_flie(body, buf);
        break;
    case READ:
        read_file(body, buf);
        break;
    case DELETE:
        delete_file(body, buf);
        break;

    case LIST:
        read_list(body, buf);
        break;

    default:
        printf("잘못된 옵션입니다.\n");
        break;
    }
    freeHashMap(body);
    
    char* pipe_body = (char *)malloc(strlen(buf) + 1);
    strcpy(pipe_body, buf);
    
    struct pipe_response_s *response = (struct pipe_response_s *)malloc(sizeof(struct pipe_response_s));
    response->ib_res = job->ib_res;
    response->body = pipe_body;
    
    write(cthred_pipefd, &response, sizeof(&response));
}

void *work(void *arg) { 
    struct queue_s *queue = (struct queue_s *)arg;
    while (1) {
        struct job_s *job = dequeue(queue);
        if (job == NULL) {
            continue;
        }
        create_response(job);
    }
}

void init_queue(struct queue_s *queue) {
    queue->front = NULL;
    queue->rear = NULL;
    pthread_cond_init(&queue->empty_cond, NULL);
    pthread_mutex_init(&queue->mutex, NULL);
}

void init_wthr_pool(struct queue_s *queue, int pipefd) {
    init_queue(queue);
    
    cthred_pipefd = pipefd;
    pthread_t threads[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, work, queue);
    }
}
