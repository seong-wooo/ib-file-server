#include <stdio.h>
#include "client.h"
#include "ib_client.h"
#include "tcp_client.h"

int main(int argc, char const *argv[])
{   
    if (argc < 2) {
        printf("실행 시 인자를 입력해주세요. IB: %c, TCP: %c\n", IB, TCP);
        return 1;
    }
    switch (*argv[1]) {
        case IB:
            ib_client();
            break;
        
        case TCP:
            tcp_client();
            break;

        default:
            printf("인자가 올바르지 않습니다.\n");
            break;
        }
    return 0;
}