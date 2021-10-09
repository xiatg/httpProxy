/*
 * @Date: 2021-10-09 08:57:47
 * @LastEditTime: 2021-10-09 14:23:50
 * @Description: Proxy
 * @FilePath: /CS3103Proxy/proxy.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h> // Internet Protocol Family
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]){
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <port> <flag_telemetry> <filename of blacklist>\n", argv[0]); // stdio.h
        exit(EXIT_FAILURE); // stdlib.h
    }

    int port = atoi(argv[1]);
    int flag_telemetry = atoi(argv[2]);
    char *b_list_path = argv[3];

    // Debug input
    // printf("%d\n%d\n%s\n", port, flag_telemetry, b_list_path);

    struct sockaddr_in Saddress; // IPv4 Address netinet/in.h

    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Socket Creation sys/socket.h

    if (sockfd < 0) {
        perror("Socket creation failed.");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) { // DIFF
        perror("Setsockopt failed.");
        exit(EXIT_FAILURE);
    }

    Saddress.sin_family = AF_INET;
    Saddress.sin_addr.s_addr = htonl(INADDR_ANY);
    Saddress.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &Saddress, sizeof(Saddress)) < 0) { // Cast to a general descriptor (struct sockaddr *)
        perror("Bind failed.");
        exit(EXIT_FAILURE);
    }

    int backlog = 3; // Number of pending connections to queue;

    if (listen(sockfd, backlog) < 0) {
        perror("Listen failed.");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in Caddress;
    int new_socket;

    socklen_t clen = sizeof(Caddress);
    if ((new_socket = accept(sockfd, (struct sockaddr_in *) &Caddress, &clen)) < 0) {
        perror("Accept failed.");
        exit(EXIT_FAILURE);
    }

    int valread;
    int buffer_size = 1024; // Buffer size;
    char buffer[buffer_size];
    
    valread = read(new_socket, buffer, sizeof(buffer)); // unistd.h
    
    // Debug Listen Value
    printf("%s\n", buffer);
    
    // Debug Send Back
    char *hello = "Hello World from server";
    send(new_socket, hello, strlen(hello), 0); // string.h
    printf("Hello sent!\n");
    
    return 0;
}