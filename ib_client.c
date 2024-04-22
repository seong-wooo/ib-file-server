#include "ib.h"
#include "message.h"

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

        post_send(ib_res);
        poll_completion_for_client(&ib_handle);
        
        create_request(option, ib_res->mr_addr);
        
        post_receive(ib_res);
        poll_completion_for_client(&ib_handle); 
        
        printf("[받은 데이터]:\n%s\n", ib_res->mr_addr); 
    }
    destroy_ib_resource(ib_res);
    destroy_ib_handle(&ib_handle);


    return 0;
} 