#ifndef IB_CLIENT_H
#define IB_CLIENT_H
#include "ib.h"

#define TYPE_1 1
#define TYPE_2 2
#define TYPE_3 3
#define TYPE_4 4

struct client_resources_s {
    struct ib_resources_s *ib_res;
    struct ib_handle_s *ib_handle;
};

struct client_resources_s *create_client_resources(int data_size, int type);
void execute_request(struct client_resources_s *client_res, int data_size, int type, int transmission_count);
void destroy_client_resources(struct client_resources_s *client_res);
void client(int data_size, int transmission_count, int type);

#endif