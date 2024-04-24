#include <stdlib.h>
#include "ib.h"

void poll_completion(struct ib_handle_s *ib_handle);

int main()
{
    struct ib_handle_s ib_handle;
    create_ib_handle(&ib_handle);
    struct ib_resources_s *ib_res = connect_ib_server(&ib_handle);

    while(1) {
        char option = get_option();
        if (option == QUIT) {
            break;
        } 

        struct packet_s *request_packet = create_request_packet(option);
        
        post_send(ib_res, request_packet);
        poll_completion(&ib_handle);
        
        post_receive(ib_res);
        poll_completion(&ib_handle); 
        struct packet_s *response_packet = create_response_packet(ib_res->mr_addr);
        
        printf("[받은 데이터]:\n%s\n", response_packet->body.data); 
    }
    destroy_ib_resource(ib_res);
    destroy_ib_handle(&ib_handle);


    return 0;
} 

void poll_completion(struct ib_handle_s *ib_handle) {
    struct ibv_cq *event_cq;
    void *event_cq_ctx;
    struct ibv_wc wc;
    
    notify_cq(ib_handle->cq);
    int rc = ibv_get_cq_event(ib_handle->cq_channel, &event_cq, &event_cq_ctx);
    if (rc) {
        perror("ibv_get_cq_event");
        exit(EXIT_FAILURE);
    }

    do {
        rc = ibv_poll_cq(event_cq, 1, &wc);
        if (rc < 0) {
            perror("ibv_poll_cq");
            exit(EXIT_FAILURE);
        }
    } while (rc == 0);
    

    ibv_ack_cq_events(event_cq, 1);
    notify_cq(ib_handle->cq);
}