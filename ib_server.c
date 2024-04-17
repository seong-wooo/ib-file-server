#include "ib.h"

int main(int argc, char const *argv[])
{
    SOCKET server_sock = create_server_socket();
    struct resources res;

    while (1)
    {
        accept_ib_client(server_sock, &res);
        post_receive(&res);
        poll_completion(&res);
    }
    destroy_resources(&res);
    printf("[TCP 서버] 서버 종료\n");

    return 0;
}