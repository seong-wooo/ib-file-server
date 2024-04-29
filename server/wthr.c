#include <pthread.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include "wthr.h"
#include "ib.h"
#include "log.h"

int cthred_pipefd;
pthread_cond_t q_empty_cond;
pthread_mutex_t mutex;

void enqueue_job(struct queue_s *queue, struct job_s* job) {
    pthread_mutex_lock(&mutex);
    enqueue(queue, job);
    pthread_cond_signal(&q_empty_cond);
    pthread_mutex_unlock(&mutex);
}

struct job_s* dequeue_job(struct queue_s *queue) {
    struct job_s* job = NULL;
    pthread_mutex_lock(&mutex);
    while (is_empty(queue)) {
        pthread_cond_wait(&q_empty_cond, &mutex);
    }
    job = (struct job_s *)dequeue(queue);
    pthread_mutex_unlock(&mutex);

    return job;
}

void write_flie(struct packet_s *packet, char *response) {
    char *filename = packet->header.filename;
    int offset = packet->header.offset;
    char *data = packet->body.data;
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

void read_file(struct packet_s *packet, char *response) {
    char *filename = packet->header.filename;
    int offset = packet->header.offset;
    int length = packet->header.length;

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        strcpy(response, "Error opening file");
    }
    else {
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

void delete_file(struct packet_s *packet, char *response)
{
    char *filename = packet->header.filename;
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

void read_list(char *response)
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

struct job_s *create_response(struct job_s *job) {
    char *buf = (char *)calloc(1, MR_BUF_SIZE);
    
    switch (job->packet->header.option) {
    case WRITE:
        write_flie(job->packet, buf);
        break;
    case READ:
        read_file(job->packet, buf);
        break;
    case DELETE:
        delete_file(job->packet, buf);
        break;
    case LIST:
        read_list(buf);
        break;

    default:
        printf("잘못된 옵션입니다.\n");
        break;
    }
    
    job->packet->header.body_size = strlen(buf) + 1;
    job->packet->body.data = buf;

    return job;
}

void *work(void *arg) { 
    struct queue_s *queue = (struct queue_s *)arg;
    while (1) {
        struct job_s *request_job = dequeue_job(queue);
        if (request_job == NULL) {
            continue;
        }
        struct job_s *response_job = create_response(request_job);
        write(cthred_pipefd, &response_job, sizeof(&response_job));
    }
}

void init_wthr_pool(struct queue_s *queue, int pipefd) {
    pthread_cond_init(&q_empty_cond, NULL);
    pthread_mutex_init(&mutex, NULL);
    
    cthred_pipefd = pipefd;
    pthread_t threads[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, work, queue);
    }
}