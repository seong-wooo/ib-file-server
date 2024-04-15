#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <infiniband/verbs.h>
#include "hash.h"


#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1

#define SERVERPORT 9000
#define BUFSIZE 10000
#define HASH_SIZE 10

#define MAX_WR 10
#define CQ_SIZE 10
#define IB_PORT 1

#define READ 'r'
#define WRITE 'w'
#define DELETE 'd'
#define QUIT 'q'
#define LIST 'l'
#define RC 'c'

#define OPTION "option"
#define QP_NUM "qp_num"
#define LID "lid"

#define FILENAME "filename"
#define OFFSET "offset"
#define DATA "data"
#define LENGTH "length"

#define LINE_PARSER "\n"
#define TOKEN_PARSER ":"

typedef int SOCKET;

void err_quit(const char *msg) {
    char *msgbuf = strerror(errno);
    printf("[%s] %s \n", msg, msgbuf);
    exit(1);
};

void err_display(const char *msg) {
    char *msgbuf = strerror(errno);
    printf("[%s] %s\n", msg, msgbuf);
}

SOCKET create_socket(void) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        err_quit("socket()");
    }
    return sock;
}

HashMap* parse_message(int hash_size, char* buf) {
    HashMap* hashMap = createHashMap(hash_size);
    char* key;
    char* value;

    char* str = strtok(buf, TOKEN_PARSER);
    while (str != NULL) {
        key = str;
        str = strtok(NULL, LINE_PARSER);
        value = str;
        str = strtok(NULL, TOKEN_PARSER);
        put(hashMap, key, value);
    }

    return hashMap;
}
