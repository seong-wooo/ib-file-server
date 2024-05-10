#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <infiniband/verbs.h>
#include <stdlib.h>
#include <time.h>
#include "ib_client.h"

void ib_client(void) {
    struct ib_handle_s *ib_handle = create_ib_handle();
    struct ib_resources_s *ib_res = connect_ib_server(ib_handle);

    while(1) {
        char option = get_option();
        if (option == QUIT) {
            break;
        }
        struct packet_s *request_packet = create_request_packet(option);
        post_receive(ib_res->qp, ib_handle->mr);
        post_send(ib_handle->mr, ib_res->qp, request_packet);
        free_packet(request_packet);

        poll_completion(ib_handle); 
        poll_completion(ib_handle);

        struct packet_s *packet = deserialize_packet(ib_handle->mr->addr);
        printf("[받은 데이터]:\n%s\n", packet->body.data);
        free_packet(packet);

    }
    destroy_ib_resource(ib_res);
    destroy_ib_handle(ib_handle);
}