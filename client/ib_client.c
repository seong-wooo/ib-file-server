#include "ib_client.h"

void ib_client(void) {
    socket_t sock = connect_tcp_to_server(LOCAL_IP, DB_PORT);
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