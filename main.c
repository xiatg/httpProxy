#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h> // Internet Protocol Family
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <netdb.h>
#include <sys/time.h>
#include <pthread.h>
#include <ctype.h>

#define HTTP10 "HTTP/1.0"
#define RFC1123FMT "%a, %d, %b, %Y, %H:%M:%S GMT"
#define TIMEOUT 5 // in seconds
#define THREADS_NUM 8

// #define DEBUG

int flag_telemetry = 0;

int client_sock_stack[10000];
int client_sock_top = -1;

char *blacklist[100];
int num_black_list = 0;

pthread_t threadpool[THREADS_NUM];
pthread_mutex_t client_sock_stack_lock;
pthread_cond_t client_sock_stack_empty;

void send_headers(int client, int status, char *title, char *extra_header, char *content_type, int length, char *protocol) {
    char buffer[10000];

    // (HTTP/1.x) (200) (OK)
    snprintf(buffer, 10000, "%s %d %s\r\n", protocol, status, title);
    send(client, buffer, strlen(buffer), 0);

#ifdef DEBUG
    printf("[Send Header] HERE! 1\n");
#endif

    // Date
    time_t now = time((time_t*) 0);
    char time_buffer[100];
    strftime(time_buffer, 100, RFC1123FMT, gmtime(&now));
    snprintf(buffer, 10000, "Date: %s\r\n", time_buffer);
    send(client, buffer, strlen(buffer), 0);

#ifdef DEBUG
    printf("[Send Header] HERE! 2\n");
#endif

    // Context-Type
    if (content_type != (char *) 0) {
        snprintf(buffer, 10000, "Content-Type: %s\r\n", content_type);
        send(client, buffer, strlen(buffer), 0);
    }

#ifdef DEBUG
    printf("[Send Header] HERE! 3\n");
#endif

    // Content-Length
    if (length >= 0) {
        snprintf(buffer, 10000, "Content-Length: %d\r\n", length);
        send(client, buffer, strlen(buffer), 0);
    }

#ifdef DEBUG
    printf("[Send Header] HERE! 4\n");
#endif

    // \r\n END
    snprintf(buffer, 10000, "\r\n");

#ifdef DEBUG
    printf("[Send Header] HERE! 5\n");
#endif

    send(client, buffer, 2, 0);

#ifdef DEBUG
    printf("[Send Header] HERE! 6\n");
#endif
}

void raise_error(int client, int status, char *title, char *protocol){
    send_headers(client, status, title, (char *) 0, (char *) 0, -1, protocol);
}

int init_server_socket(int port) {  // TODO: Only IPv4 at present
    struct sockaddr_in Saddress; // IPv4 Address netinet/in.h

    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Socket Creation sys/socket.h; AF_INET: IPv4

    if (sockfd < 0) {
        perror("Server Socket creation failed.");
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

int init_client_socket(int client_sock, char* host, int port, char *protocol) {
    
    struct sockaddr_in TAddress; // Target IPv4 address

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        // Target Socket Creation Failed
#ifdef DEBUG
        perror("Target Socket creation failed.");
#endif

        raise_error(client_sock, 500, "Internal Error", protocol);
        return -1;
    }

    struct hostent *he; // netdb.h

    if ((he = gethostbyname(host)) == NULL) {
        // Unknown host
#ifdef DEBUG
        perror("Unknown target host.");
#endif
        raise_error(client_sock, 404, "Not Found", protocol);
        return -1;
    }

    memset(&TAddress, '\0', sizeof(TAddress));
    TAddress.sin_family = AF_INET;
    memcpy((char *) &TAddress.sin_addr.s_addr, he->h_addr_list[0], he->h_length);
    TAddress.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*) &TAddress, sizeof(TAddress)) < 0) {
        //Target connection failed
#ifdef DEBUG
        perror("Target connection failed.");
#endif
        raise_error(client_sock, 503, "Service Unavailable", protocol);
        return -1;
    }

    return sockfd;
}

/*it will discard the "\\r\\n" at the end of each line*/
int read_request_line(int sock, char *buffer, int buf_size) {
    int i = 0;
    char c = '\0';
    int n;

    while ((i < buf_size - 1) && (c != '\n')) {
        n = recv(sock, &c, 1, 0); // receive one char from the socket
        buffer[i] = c;
        i++;

        //Debug: print received msg
        // printf("%c", c);
    }

    while (buffer[i-1] == '\n' || buffer[i-1] == '\r') {
        buffer[i-1] = '\0';
        i--;
    }

    return i;
}

long proxy_https(int client, char* method, char* host, char* protocol, char* headers, FILE* sockrfp, FILE* sockwfp) {
    //HTTP/1.x 200 Connection established\r\n\r\n
    char connection_established[100];

    snprintf(connection_established, 100, "%s 200 Connection established\r\n\r\n", protocol);
    
    send(client, connection_established, strlen(connection_established), 0);

    int client_read_fd, server_read_fd, client_write_fd, server_write_fd;
    client_read_fd = client;
    server_read_fd = fileno(sockrfp);
    client_write_fd = client;
    server_write_fd = fileno(sockwfp);

    struct timeval timeout; // sys/time.h
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    int maxfd; //maximum file descriptor + 1
    if (client_read_fd >= server_read_fd) {
        maxfd = client_read_fd + 1;
    } else {
        maxfd = server_read_fd + 1;
    }

    fd_set fdset;
    int r;
    char buffer[1024];

#ifdef DEBUG
    printf("(Tunnel) Tunneling for client sock %d & target sock %d\n", client, server_read_fd);
#endif

    long byte_count = 0;

    while (1){
        FD_ZERO(&fdset);
        FD_SET(client_read_fd, &fdset);
        FD_SET(server_read_fd, &fdset);
        
        r = select(maxfd, &fdset, (fd_set*) 0, (fd_set*) 0, &timeout);

        if (r == 0) {
            //Time out

#ifdef DEBUG
            printf("(Tunnel) Timeout for client sock %d & target sock %d\n", client, server_read_fd);
#endif

            return byte_count;
        } else if (FD_ISSET(client_read_fd, &fdset)) {
            r = read(client_read_fd, buffer, sizeof(buffer));
            if (r <= 0) {
                break;
            }
            r = write(server_write_fd, buffer, r);
            if (r <= 0) {
                break;
            }
        } else if (FD_ISSET(server_write_fd, &fdset)) {
            r = read(server_read_fd, buffer, sizeof(buffer));
            if (r <= 0) {
                break;
            }
            r = write(client_write_fd, buffer, r);
            if (r <= 0) {
                break;
            }
        }

        byte_count += r;

        //Debug
        // printf("(Tunnel) client %d <--> target %d: %d bytes\n", client, server_read_fd, r);
    }

#ifdef DEBUG
    printf("(Tunnel) Normal End for client sock %d & target sock %d\n", client, server_read_fd);
#endif

    return byte_count;
}

void *process_request(void * _client_sock) {
    int client_sock = (int) (long) _client_sock;

    // Parse the request
    char line[10000]; // TODO: Length?

    char method[10000];
    char url[10000];
    char protocol[10000];

    //Parse the first line
    int line_len = read_request_line(client_sock, line, sizeof(line));

    // Wrong format
    if (sscanf(line, "%[^ ] %[^ ] %[^ ]", method, url, protocol) != 3) {

#ifdef DEBUG
        perror("Unknown HTTP Request First Line Format:");
        printf("%s\n", line);
#endif

        //TODO
        if (strlen(line) == 0) {
            /*
            TODO:
            a very weired bug: getting an empty http request from the 
            browser and calling send() back to the browser will crash
            the program.
            */
            return NULL;
        }

        raise_error(client_sock, 400, "Bad Request", HTTP10);
        return NULL;
    }

    // Null URL
    if (url[0] == '\0') {

#ifdef DEBUG
        perror("NULL URL");
#endif
        raise_error(client_sock, 400, "Bad Request", protocol);
        return NULL;
    }

    char host[10000];
    int port = 443;

    if (strcmp(method, "CONNECT") == 0) {
        // URL without :port
        if (sscanf(url, "%[^:]:%d", host, &port) != 2) {
            
#ifdef DEBUG
            perror("URL without :port");
#endif

            raise_error(client_sock, 400, "Bad Request", protocol);
            return NULL;
        } 
    } else {

#ifdef DEBUG
        perror("method != CONNECT");
#endif

        raise_error(client_sock, 400, "Bad Request", protocol);
        return NULL;
    }

    //Blacklisting
    int i = 0;
    for (i = 0; i < num_black_list; i++) {
        if (strstr(host, blacklist[i]) != NULL) {
#ifdef DEBUG
            perror("blacklist triggered!");
            printf("Blocked host: %s\n", host);
#endif
            raise_error(client_sock, 403, "Blacklisted URL", protocol);
            return NULL;
        }
    }

    //Parse the headers
    char headers[20000];
    int headers_len = 0;
    while (read_request_line(client_sock, line, sizeof(line)) > 0) {
        int line_len = strlen(line);

        //Debug
        // printf("HEADER LENGTH LOL = %d\n", headers_len);

        memcpy(&headers[headers_len], line, line_len);
        headers_len += line_len;
    }
    headers[headers_len] = '\0';

    int target_sock = init_client_socket(client_sock, host, port, protocol);

#ifdef DEBUG
    printf("(HTTPS Proxy) initialized client sock %d to target sock %d\n", client_sock, target_sock);
#endif

    if (target_sock >= 0) {
        FILE* sockrfp;
        FILE* sockwfp;
        sockrfp = fdopen(target_sock, "r");
        sockwfp = fdopen(target_sock, "w");
        
        clock_t start, end; 
        start = clock();
        long byte_count = proxy_https(client_sock, method, host, protocol, headers, sockrfp, sockwfp);

        if (flag_telemetry == 1 && byte_count >= 0) {
            end = clock();
            printf("Hostname: %s, Size: %ld bytes, Time: %.3f sec\n", host, byte_count, (float) (end-start) / 1000.0);
        }

        close(target_sock);
    }
    return NULL;
}

void *thread_control(void *arg) {
    int index = (int) (long) arg;
    int client_sock;

    while (1) {
        pthread_mutex_lock(&client_sock_stack_lock);
        while (client_sock_top == -1) {
            pthread_cond_wait(&client_sock_stack_empty, &client_sock_stack_lock);
        }
        client_sock = client_sock_stack[client_sock_top];
        client_sock_top -= 1;
        pthread_mutex_unlock(&client_sock_stack_lock);

#ifdef DEBUG
        printf("(Thread %d) Processing! client sock = %d, Stack_top = %d\n", index, client_sock, client_sock_top);
#endif

        process_request((void *) (long) client_sock); // Void Pointer <== Long (same-size) <== Int

        close(client_sock);
    
#ifdef DEBUG
        printf("(Thread %d) Completed! client sock = %d, Stack_top = %d\n", index, client_sock, client_sock_top);
#endif
    }
}

int is_number(char *s) {
    int i = 0;
    while(s[i] != '\0') {
        if (isdigit(s[i]) == 0) {
            return 0;
        }
        i++;
    }
    return 1;
}

int read_black_list(char *file_path) {
    int num_black = 0;

    FILE * fp;
    char *line = NULL;
    size_t line_len = 0;
    ssize_t nread;
    fp = fopen(file_path, "r");
    if (fp == NULL) {
        printf("failed to load blacklist.txt\n");
        exit(EXIT_FAILURE);
    }

    while ((nread = getline(&line, &line_len, fp)) != -1) {

        while (line[nread - 1] == '\n'){
            line[nread - 1] = '\0';
            nread--;
        }

        //Debug
        // printf("%s\n", line);
        // printf("%p\n", blacklist[num_black]);
        blacklist[num_black] = malloc(nread);
        strcpy(blacklist[num_black], line);
        num_black++;
    }
    
    fclose(fp);
    return num_black;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <port> <flag_telemetry> <filename of blacklist>\n", argv[0]); // stdio.h
        exit(EXIT_FAILURE); // stdlib.h
    }

    int port = atoi(argv[1]);

    //Checking the correctness of flag_telemetry
    if (strlen(argv[2]) > 1) {
        printf("Wrong flag for telemetry, should be 0 or 1.\n");
        exit(EXIT_FAILURE);
    }
    if (is_number(argv[2]) == 0) {
        printf("Wrong flag for telemetry, should be 0 or 1.\n");
        exit(EXIT_FAILURE);
    }
    flag_telemetry = atoi(argv[2]);
    if (flag_telemetry != 0 && flag_telemetry != 1) {
        printf("Wrong flag for telemetry, should be 0 or 1.\n");
        exit(EXIT_FAILURE);
    }

    char *b_list_path = argv[3];

    num_black_list = read_black_list(b_list_path);

    //Debug
    // for (int i = 0; i < num_black_list; i++) {
    //     printf("%s\n", blacklist[i]);
    // }

    int server_sock = init_server_socket(port);
    printf(";) HTTPS Proxy running on port %d!\n", port);

    struct sockaddr_in Caddress;
    socklen_t clen = sizeof(Caddress);
    int client_sock;

    pthread_mutex_init(&client_sock_stack_lock, NULL);
    pthread_cond_init(&client_sock_stack_empty, NULL);

    int i = 0;

    for (i = 0; i < THREADS_NUM; i++) {
        pthread_create(&threadpool[i], NULL, &thread_control,(void *) (long) i);
    }

    while (1) {
        if ((client_sock = accept(server_sock, (struct sockaddr_in *) &Caddress, &clen)) < 0) {
            perror("Accept failed.\n");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&client_sock_stack_lock);
        client_sock_top += 1;
        client_sock_stack[client_sock_top] = client_sock;
        if (client_sock_top == 0) {
            pthread_cond_signal(&client_sock_stack_empty);
        }
        pthread_mutex_unlock(&client_sock_stack_lock);
    }

    for (i = 0; i < THREADS_NUM; i++) {
        pthread_join(threadpool[i], NULL);
    }

    pthread_mutex_destroy(&client_sock_stack_lock);
    pthread_cond_destroy(&client_sock_stack_empty);

    close(server_sock);
    return 0;
}