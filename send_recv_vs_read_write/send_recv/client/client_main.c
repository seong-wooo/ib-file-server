#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ib_client.h"

int main(int argc, char const *argv[]) {   
    
    if (argc < 2) {    
        printf("실행 시 주고 받을 데이터 크기를 입력해주세요. (바이트 단위)\n");
        return 1;
    }
    else if (argc < 3) {
        printf("실행 시 데이터를 주고 받을 횟수를 입력해주세요.\n");
        return 1;
    }

    int data_size = atoi(argv[1]);
    int transmission_count = atoi(argv[2]);
    send_receive_client(data_size, transmission_count);

    return 0;
}