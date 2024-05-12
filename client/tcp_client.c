#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <time.h>
#include "tcp_client.h"

void tcp_client(void) {
    socket_t sock = connect_tcp_to_server(SERVER_IP, TCP_SERVER_PORT);
    void *buffer = malloc(MESSAGE_SIZE);

    while(1) {
        char option = get_option();
        if (option == QUIT) {
            break;
        }
        struct packet_s *request_packet = create_request_packet(option);
        send_packet(sock, request_packet, buffer);
        
        struct packet_s *response_packet = recv_packet(sock, buffer);
        printf("[받은 데이터]:\n%s\n", response_packet->body.data);
        
        free_packet(request_packet);
        free_packet(response_packet);
    }
    close_socket(sock);
}