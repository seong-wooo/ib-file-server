#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void err_quit(const char *msg) {
    char *msgbuf = strerror(errno);
    printf("[%s] %s \n", msg, msgbuf);
    exit(1);
};

void err_display(const char *msg) {
    char *msgbuf = strerror(errno);
    printf("[%s] %s\n", msg, msgbuf);
}
