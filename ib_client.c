#include "ib.h"
#include "packet.h"

int main()
{
    struct resources res;
    connect_ib_server(&res);

    while(1) {
        char option = get_option();
        if (option == QUIT) {
            break;
        }
        create_request(option, res.buffer);
        post_send(&res);
        poll_completion(&res);

        post_receive(&res);
        poll_completion(&res);
        printf("[받은 데이터]:\n%s\n", res.buffer); 
    }
    destroy_resources(&res);


    return 0;
} 