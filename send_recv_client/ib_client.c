#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <infiniband/verbs.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ib_client.h"

void send_receive_client(int data_size, int transmission_count) {

    struct ib_handle_s *ib_handle = create_ib_handle();
    struct ib_resources_s *ib_res = connect_ib_server(ib_handle);
    
    memcpy(ib_handle->mr->addr, &data_size, sizeof(int));
    memset(ib_handle->mr->addr + sizeof(int), 'a', data_size);
    
    for (int i = 0; i < transmission_count; i++) {
        post_receive(ib_res->qp, ib_handle->mr);
        post_send(ib_handle->mr, ib_res->qp, data_size);
        poll_completion(ib_handle, 2);
    }

    destroy_ib_resource(ib_res);
}