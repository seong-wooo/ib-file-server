#include <stdlib.h>
#include "ib.h"

int main() {
    struct ib_handle_s *ib_handle = create_ib_handle();
    struct ib_resources_s *ib_res = connect_ib_server(ib_handle);

    while(1) {
        char option = get_option();
        if (option == QUIT) {
            break;
        }
        struct packet_s *request_packet = create_response_packet(option);
        request_packet->header.qp_num = ib_res->remote_props->qp_num;
        
        post_receive(ib_res->qp, ib_handle->mr);
        post_send(ib_handle->mr, ib_res->qp, request_packet);
        poll_completion(ib_handle); 
        poll_completion(ib_handle);

        struct packet_s *response_packet = create_request_packet(ib_handle->mr->addr);
        
        printf("[받은 데이터]:\n%s\n", response_packet->body.data); 
    }
    destroy_ib_resource(ib_res);
    destroy_ib_handle(ib_handle);


    return 0;
}