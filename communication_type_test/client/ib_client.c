#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <infiniband/verbs.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ib_client.h"

struct client_resources_s *create_client_resources(int data_size, int type) {
    struct ib_handle_s *ib_handle = create_ib_handle();
    struct ib_resources_s *ib_res = connect_ib_server(ib_handle);
    struct message_header_s header = (struct message_header_s) {
        .data_size = data_size,
        .type = type,
        .rdma_data = {
            .rkey = ib_handle->mr->rkey,
            .remote_addr = (uint64_t)(uintptr_t)ib_handle->mr->addr
        },
    };
    memcpy(ib_handle->mr->addr, &header, sizeof(struct message_header_s));
    memset(ib_handle->mr->addr + sizeof(struct message_header_s), 'a', data_size);

    struct client_resources_s *client_res = (struct client_resources_s *)malloc(sizeof(struct client_resources_s));
    client_res->ib_handle = ib_handle;
    client_res->ib_res = ib_res;

    return client_res;
}

void execute_request(struct client_resources_s *client_res, int data_size, int type, int transmission_count) {
    struct ib_resources_s *ib_res = client_res->ib_res;
    struct ib_handle_s *ib_handle = client_res->ib_handle;
    
    int req_size = sizeof(struct message_header_s);

    if (type == TYPE_1 || type == TYPE_2) {
        req_size += data_size;
    } 

    for (int i = 0; i < transmission_count; i++) {
        post_receive(ib_res->qp, ib_handle->mr);
        post_send(ib_res->qp, ib_handle->mr, req_size);
        poll_completion(ib_handle, 2);
    }
}

void destroy_client_resources(struct client_resources_s *client_res) {
    destroy_ib_resource(client_res->ib_res);
    destroy_ib_handle(client_res->ib_handle);
    free(client_res);
}

void client(int data_size, int transmission_count, int type) {
    struct client_resources_s *client_res = create_client_resources(data_size, type);
    execute_request(client_res, data_size, type, transmission_count);
    destroy_client_resources(client_res);
}