#include <stdio.h>
#include <stdlib.h>
#include "err_check.h"

bool is_null(void *ptr) {
    return ptr == NULL;
}

void check_null(void *ptr, char *msg) {
    if (is_null(ptr)) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void check_error(int rc, char *msg) {
    if (rc < 0) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}