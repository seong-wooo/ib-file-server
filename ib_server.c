#include "ib.h"
#include "packet.h"
#include <dirent.h>

#define LOG_INFO "INFO"
#define LOGFILE "log.txt"
#define BUFSIZE 256

void write_flie(struct hash_map_s *body, char *response);
void read_file(struct hash_map_s *body, char *response);
void delete_file(struct hash_map_s *body, char *response);
void read_list(struct hash_map_s *body, char *response);
void write_log(const char* level, const char* message);
void create_response(struct ib_resources_s *ib_res);

int main(int argc, char const *argv[])
{
    struct ib_handle_s ib_handle;
    create_ib_handle(&ib_handle);
    socket_t server_sock = create_server_socket();
    struct ib_resources_s *ib_res = accept_ib_client(server_sock, &ib_handle);
    
    while (1) {
        post_receive(ib_res);
        poll_completion(&ib_handle);
        create_response(ib_res);
        post_send(ib_res);
        poll_completion(&ib_handle);
    }
    destroy_resource(ib_res);
    destroy_handle(&ib_handle);
    close(server_sock);
    printf("[TCP 서버] 서버 종료\n");

    return 0;
}

void create_response(struct ib_resources_s *ib_res) {
    struct hash_map_s *body = parse_message(HASH_SIZE, ib_res->mr_addr);
    char *option = get(body, OPTION);
    switch (*option) {
    case WRITE:
        write_flie(body, ib_res->mr_addr);
        break;
    case READ:
        read_file(body, ib_res->mr_addr);
        break;
    case DELETE:
        delete_file(body, ib_res->mr_addr);
        break;

    case LIST:
        read_list(body, ib_res->mr_addr);
        break;

    default:
        printf("잘못된 옵션입니다.\n");
        break;
    }
    freeHashMap(body);
}

void write_flie(struct hash_map_s *body, char *response)
{
    char *filename = get(body, FILENAME);
    int offset = atoi(get(body, OFFSET));
    char *data = get(body, DATA);
    char logMessage[BUFSIZE];

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

    snprintf(logMessage, BUFSIZE, "option=%c, filename=%s, offset=%d, data=%s", WRITE, filename, offset, data);
    write_log(LOG_INFO, logMessage);

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
        char logMessage[BUFSIZE];
        snprintf(logMessage, BUFSIZE, "option=%c, filename=%s", DELETE, filename);
        write_log(LOG_INFO, logMessage);
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

void write_log(const char* level, const char* message) {
    time_t now;
    time(&now);
    char time[20]; 
    struct tm *local_time = localtime(&now);
    strftime(time, sizeof(time), "%Y%m%d%H%M%S", local_time);

    FILE* fp = fopen(LOGFILE, "a");
    if (fp == NULL) {
        perror("Error opening log file");
        exit(1);
    }

    fprintf(fp, "%s [%s]: %s\n", time, level, message);
    fclose(fp);
}