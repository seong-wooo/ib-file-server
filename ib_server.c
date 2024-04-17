#include "ib.h"

int main(int argc, char const *argv[])
{
    SOCKET server_sock = create_server_socket();
    struct resources res;

    accept_ib_client(server_sock, &res);
    while (1)
    {
        post_receive(&res);
        poll_completion(&res);
        printf("받은 데이터 : %s\n", res.buffer);
    }
    destroy_resources(&res);
    printf("[TCP 서버] 서버 종료\n");

    return 0;
}