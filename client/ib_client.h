#ifndef IB_CLIENT_H
#define IB_CLIENT_H
#include "message.h"
#include "tcp_ip.h"
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <infiniband/verbs.h>
#include <stdlib.h>

void ib_client(void);

#endif