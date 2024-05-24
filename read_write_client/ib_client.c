#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <infiniband/verbs.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ib_client.h"

void read_write_client(int data_size, int transmission_count) {

    struct ib_handle_s *ib_handle = create_ib_handle();
    struct ib_resources_s *ib_res = connect_ib_server(ib_handle);
    struct message_header_s header = (struct message_header_s) {
        .data_size = data_size,
        .rdma_data = {
            .rkey = ib_handle->mr->rkey,
            .remote_addr = (uint64_t)(uintptr_t)ib_handle->mr->addr
        },
    };
    memcpy(ib_handle->mr->addr, &header, sizeof(struct message_header_s));
    memset(ib_handle->mr->addr + sizeof(struct message_header_s), 'a', data_size);

    for (int i = 0; i < transmission_count; i++) {
        post_receive(ib_res->qp, ib_handle->mr);
        post_send(ib_res->qp, ib_handle->mr);
        poll_completion(ib_handle, 2);
    }

    destroy_ib_resource(ib_res);
}