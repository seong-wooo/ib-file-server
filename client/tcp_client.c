#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <time.h>
#include "tcp_client.h"

void tcp_client(void) {
    socket_t sock = connect_tcp_to_server(SERVER_IP, TCP_SERVER_PORT);
    struct packet_s *packet = create_packet();

    while(1) {
        char option = get_option();
        if (option == QUIT) {
            break;
        }
        create_request_packet(option, packet);
        send_packet(sock, packet);
        
        recv_packet(sock, packet);
        printf("[받은 데이터]:\n%s\n", packet->body.data);
        
    }
    free_packet(packet);
    close_socket(sock);
}