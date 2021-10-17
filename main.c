/*
 * @Date: 2021-10-09 08:57:47
 * @LastEditTime: 2021-10-12 11:10:44
 * @Description: Proxy
 * @FilePath: /CS3103Proxy/proxy.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h> // Internet Protocol Family
#include <unistd.h>
#include <string.h>

int init_server_socket(int port) {
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

    return sockfd;
}

// int init_client_socket() {

// }

int read_request_line(int sock, char *buffer, int buf_size) {
    int i = 0;
    char c = '\0';
    int n;

    while ((i < buf_size - 1) && (c != '\n')) {
        n = recv(sock, &c, 1, 0); // receive one char from the socket
        buffer[i] = c;
        i++;

        //Debug: print received msg
        printf("%c", c);
    }

    // while (buffer[i-1] == '\n' || )

    return i;
}

void *process_request(void * _client_sock) {
    int client_sock = (int) (long) _client_sock;

    // Parse the request
    char line[10000]; // TODO: Length?
    char method[10000];
    char url[10000];
    char protocol[10000];
    char host[10000];
    char path[10000];
    char headers[20000];
    int line_len = read_request_line(client_sock, line, sizeof(line));

    if ((sscanf(line, "%[^ ] %[^ ] %[^ ]", method, url, protocol)))


    // while (read_request_line(client_sock, line, sizeof(line)) > 0) {

    // }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <port> <flag_telemetry> <filename of blacklist>\n", argv[0]); // stdio.h
        exit(EXIT_FAILURE); // stdlib.h
    }

    int port = atoi(argv[1]);
    int flag_telemetry = atoi(argv[2]);
    char *b_list_path = argv[3];

    // Debug input
    // printf("%d\n%d\n%s\n", port, flag_telemetry, b_list_path);

    int server_sock = init_server_socket(port);
    printf(";) HTTPS Proxy running on port %d!\n", port);

    struct sockaddr_in Caddress;
    socklen_t clen = sizeof(Caddress);
    int client_sock;

    while (1) {
        if ((client_sock = accept(server_sock, (struct sockaddr_in *) &Caddress, &clen)) < 0) {
            perror("Accept failed.");
            exit(EXIT_FAILURE);
        }

        int valread;
        int buffer_size = 1024; // Buffer size;
        char buffer[buffer_size];
        
        // Debug Listen Value
        // valread = read(client_sock, buffer, sizeof(buffer)); // unistd.h
        // printf("%s\n", buffer);

        process_request((void *) (long) client_sock); // Void Pointer <== Long (same-size) <== Int
    }

    close(server_sock);
    
    return 0;
}