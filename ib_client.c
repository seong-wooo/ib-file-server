#include "ib.h"

char *SERVERIP = (char *) "192.1.1.207";

void connect_to_server(SOCKET sock, char* ip, int port);
void get_list(char* buf);
void get_read(char* buf);
void get_write(char* buf);
void get_delete(char* buf);
int get_option(void);
int get_offset(void);
int get_length(void);
char* get_data(void);
char* get_filename(void);

int main()
{
    struct resources res;
    create_resources(&res);
    connect_to_server(res.sock, SERVERIP, SERVERPORT);
    send_qp_sync_data(res.sock, &res);
    recv_qp_sync_data(res.sock, &res);
    
    modify_qp_to_init(&res);
    modify_qp_to_rtr(&res);
    modify_qp_to_rts(&res);


    strcpy(res.buffer, MSG);
    post_send(&res, IBV_WR_SEND);
    poll_completion(&res);
    destroy_resources(&res);
    
    return 0;
}   

void connect_to_server(SOCKET sock, char* ip, int port) {
    int retval;
    struct sockaddr_in serveraddr;

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(port);
    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");
}

int get_option(void) {
    char option;
    do {
        fseek(stdin, 0, SEEK_END);
        printf("[파일 리스트 조회: %c / 파일 읽기 : %c / 파일 쓰기 : %c / 파일 삭제 : %c / 종료 : %c]: ", LIST, READ, WRITE, DELETE, QUIT);
        option = getchar();
        getchar();
    } while (option != LIST && option != READ && option != WRITE && option != DELETE && option != QUIT);

    return option;
}

void get_list(char* buf) {
    sprintf(buf, "%s%s%c", OPTION, TOKEN_PARSER, LIST);
}

void get_read(char* buf) {
    char* filename;
    int offset;
    int length;

    filename = get_filename();
    offset = get_offset();
    length = get_length();
    sprintf(buf, "%s%s%c%s%s%s%s%s%s%s%d%s%s%s%d", OPTION, TOKEN_PARSER, READ, LINE_PARSER, FILENAME, TOKEN_PARSER, filename, LINE_PARSER, OFFSET, TOKEN_PARSER, offset, LINE_PARSER, LENGTH, TOKEN_PARSER, length);
    free(filename);
}

void get_write(char* buf) {
    char* filename;
    int offset;
    int length;
    char* data;

    filename = get_filename();
    offset = get_offset();
    data = get_data();
    sprintf(buf, "%s%s%c%s%s%s%s%s%s%s%d%s%s%s%s", OPTION, TOKEN_PARSER, WRITE, LINE_PARSER, FILENAME, TOKEN_PARSER, filename, LINE_PARSER, OFFSET, TOKEN_PARSER, offset, LINE_PARSER, DATA, TOKEN_PARSER, data);
    
    free(filename);
    free(data);
}

void get_delete(char* buf) {
    char* filename;

    filename = get_filename();
    sprintf(buf, "%s%s%c%s%s%s%s", OPTION, TOKEN_PARSER, DELETE, LINE_PARSER, FILENAME, TOKEN_PARSER, filename);
    free(filename);
}

int get_offset(void) {
    int offset;
    printf("[오프셋]: ");
    scanf("%d", &offset);
    getchar();
    
    return offset;
}

int get_length(void) {
    int length;
    printf("[읽을 길이]: ");
    scanf("%d", &length);
    getchar();
    
    return length;
}

char* get_data(void) {
    char temp[1000];
    char* data;

    printf("[쓸 내용]: ");
    fgets(temp, sizeof(temp), stdin);
    temp[strlen(temp) - 1] = '\0';
    data = (char *)malloc(strlen(temp) + 1);
    strcpy(data, temp);
    
    return data;
}

char* get_filename(void) {
    char temp[1000];
    char* filename;

    printf("[파일명]: ");
    fgets(temp, sizeof(temp), stdin);
    temp[strlen(temp) - 1] = '\0';
    filename = (char *)malloc(strlen(temp) + 1);
    strcpy(filename, temp);
    
    return filename;
}