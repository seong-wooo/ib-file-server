#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H
#include "message.h"
#include "test_client.h"

#define TCP_SERVER_PORT 9000
void tcp_client(void);
void *tcp_test_client(void *arg);

#endif
