#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "message.h"

struct packet_s *create_response_packet(void *data) {
    struct packet_s *packet = malloc(sizeof(struct packet_s));
    memcpy(&packet->header, data, sizeof(struct packet_header_s));
    packet->body.data = (char *)malloc(packet->header.body_size);
    memcpy(packet->body.data, data + sizeof(struct packet_header_s), packet->header.body_size);
    
    return packet;
}

char get_option(void) {
    char option;
    do {
        fseek(stdin, 0, SEEK_END);
        printf("[파일 리스트 조회: %c / 파일 읽기 : %c / 파일 쓰기 : %c / 파일 삭제 : %c / 종료 : %c]: ", LIST, READ, WRITE, DELETE, QUIT);
        option = getchar();
        getchar();
    } while (option != LIST && option != READ && option != WRITE && option != DELETE && option != QUIT);

    return option;
}

int get_offset(void) {
    int offset;
    printf("[오프셋]: ");
    scanf("%d", &offset);
    getchar();
    
    return offset;
}

int get_length(void) {
    int length;
    printf("[읽을 길이]: ");
    scanf("%d", &length);
    getchar();
    
    return length;
}

void get_data(char **data) {
    printf("[쓸 내용]: ");
    char temp[4096];
    fgets(temp, sizeof(temp), stdin);
    temp[strlen(temp) - 1] = '\0';

    *data = (char *)malloc(strlen(temp) + 1);
    strcpy(*data, temp);
}

void get_filename(char* filename) {
    printf("[파일명]: ");
    fgets(filename, 20, stdin);
    filename[strlen(filename) - 1] = '\0';
}

void get_list_packet(struct packet_s *packet) {
    packet->header.option = LIST;
}

void get_read_packet(struct packet_s *packet) {
    packet->header.option = READ;
    get_filename(packet->header.filename);
    packet->header.offset = get_offset();
    packet->header.length = get_length();
}

void get_write(struct packet_s *packet) {
    packet->header.option = WRITE;
    get_filename(packet->header.filename);
    packet->header.offset = get_offset();
    get_data(&packet->body.data);
    packet->header.body_size = strlen(packet->body.data) + 1;
}

void get_delete(struct packet_s *packet) {
    packet->header.option = DELETE;
    get_filename(packet->header.filename);
}

struct packet_s *create_request_packet(char option) {
    struct packet_s *packet = malloc(sizeof(struct packet_s));
    memset(&packet->header, 0, sizeof(packet->header));
    switch (option) {
        case LIST:
            get_list_packet(packet);
            break;
        case READ:
            get_read_packet(packet);
            break;
        case WRITE:
            get_write(packet);
            break;
        case DELETE:
            get_delete(packet);
            break;
        default:
            break;
    }

    return packet;
}