#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hash.h"

#define READ 'r'
#define WRITE 'w'
#define DELETE 'd'
#define QUIT 'q'
#define LIST 'l'

#define OPTION "option"
#define FILENAME "filename"
#define OFFSET "offset"
#define DATA "data"
#define LENGTH "length"

#define LINE_PARSER "\n"
#define TOKEN_PARSER ":"

#define HASH_SIZE 10

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

char* get_data(void) {
    char temp[1000];
    char* data;

    printf("[쓸 내용]: ");
    fgets(temp, sizeof(temp), stdin);
    temp[strlen(temp) - 1] = '\0';
    data = (char *)malloc(strlen(temp) + 1);
    strcpy(data, temp);
    
    return data;
}

char* get_filename(void) {
    char temp[1000];
    char* filename;

    printf("[파일명]: ");
    fgets(temp, sizeof(temp), stdin);
    temp[strlen(temp) - 1] = '\0';
    filename = (char *)malloc(strlen(temp) + 1);
    strcpy(filename, temp);
    
    return filename;
}

void get_list(char* buf) {
    sprintf(buf, "%s%s%c", OPTION, TOKEN_PARSER, LIST);
}

void get_read(char* buf) {
    char* filename;
    int offset;
    int length;

    filename = get_filename();
    offset = get_offset();
    length = get_length();
    sprintf(buf, "%s%s%c%s%s%s%s%s%s%s%d%s%s%s%d", OPTION, TOKEN_PARSER, READ, LINE_PARSER, FILENAME, TOKEN_PARSER, filename, 
            LINE_PARSER, OFFSET, TOKEN_PARSER, offset, LINE_PARSER, LENGTH, TOKEN_PARSER, length);
    free(filename);
}

void get_write(char* buf) {
    char* filename;
    int offset;
    int length;
    char* data;

    filename = get_filename();
    offset = get_offset();
    data = get_data();
    sprintf(buf, "%s%s%c%s%s%s%s%s%s%s%d%s%s%s%s", OPTION, TOKEN_PARSER, WRITE, LINE_PARSER, FILENAME, TOKEN_PARSER, filename, 
            LINE_PARSER, OFFSET, TOKEN_PARSER, offset, LINE_PARSER, DATA, TOKEN_PARSER, data);
    
    free(filename);
    free(data);
}

void get_delete(char* buf) {
    char* filename;

    filename = get_filename();
    sprintf(buf, "%s%s%c%s%s%s%s", OPTION, TOKEN_PARSER, DELETE, LINE_PARSER, FILENAME, TOKEN_PARSER, filename);
    free(filename);
}

char *create_request(char option, char* buf) {
    switch (option) {
            case LIST:
                get_list(buf);
                break;
            case READ:
                get_read(buf);
                break;
            case WRITE:
                get_write(buf);
                break;
            case DELETE:
                get_delete(buf);
                break;
            default:
                break;
        }

    return NULL;
}

struct hash_map_s *parse_message(int hash_size, char *buf)
{
    struct hash_map_s *body = create_hash_map(hash_size);
    char *key;
    char *value;

    char *str = strtok(buf, TOKEN_PARSER);
    while (str != NULL) {
        key = str;
        str = strtok(NULL, LINE_PARSER);
        value = str;
        str = strtok(NULL, TOKEN_PARSER);
        put(body, key, value);
    }

    return body;
}
