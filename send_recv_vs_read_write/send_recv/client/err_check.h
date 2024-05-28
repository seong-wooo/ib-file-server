#ifndef ERR_CHECK_H
#define ERR_CHECK_H

#include <stdbool.h>

bool is_null(void *ptr);
void check_null(void *ptr, char *msg);
void check_error(int rc, char *msg);

#endif