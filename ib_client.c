#include "ib.h"

int main()
{
    struct resources res;
    connect_ib_server(&res);

    strcpy(res.buffer, MSG);
    post_send(&res, IBV_WR_SEND);

    poll_completion(&res);
    destroy_resources(&res);

    return 0;
}   