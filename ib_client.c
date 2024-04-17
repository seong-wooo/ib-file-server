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
        post_send(&res, IBV_WR_SEND);

        poll_completion(&res);
    }
    destroy_resources(&res);


    return 0;
} 