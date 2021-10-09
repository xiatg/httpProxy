/*
 * @Date: 2021-10-09 08:57:47
 * @LastEditTime: 2021-10-09 11:40:24
 * @Description: Proxy
 * @FilePath: /CS3103Proxy/proxy.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h> // Internet Protocol Family

int main(int argc, char *argv[]){
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <port> <flag_telemetry> <filename of blacklist>\n", argv[0]); // stdio.h
        exit(EXIT_FAILURE); // stdlib.h
    }

    int port = atoi(argv[1]);
    int flag_telemetry = atoi(argv[2]);
    char *b_list_path = argv[3];

    // Debug input
    printf("%d\n%d\n%s\n", port, flag_telemetry, b_list_path);

    struct sockaddr_in address; // netinet/in.h

    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Socket Creation sys/socket.h

    if (sockfd < 0) {
        perror("Socket creation failed.");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);


}