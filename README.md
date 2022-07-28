# CS3103-AS1

This repository was part of the assignment 1 in CS3103 (Autumn 2021) in NUS.

## Program Design
### proxy implementation
To implement the https proxy, we first set up a sever socket using `init_server_socket()`. Upon successful initialization, we can accept requests sending from the browser. For each incoming request, we use `process_request()` to parse the request and set up the corresponding socket to the target host using `init_client_socket()`. Upon successful creation, the https tunnel will be handled by `proxy_https()`.

### multi-threading
To process multiple requests simultaneously, we use `pthread.h` and create 8 threads during the initialization of the proxy. For incoming requests, the corresponding file descriptor will be pushed to the stack `client_sock_stack`. The 8 threads would compete to take one request from the top of the stack and process it. The synchronization is solved with the pthread mutex `client_sock_stack_lock`, pthread condition `client_sock_stack_empty` and an index `client_sock_top` that stores the index of the top of the stack.

### https tunneling
To handle the https tunneling, multiple file descriptors need to be handled at the same time. We use `select()` to handle the incoming information from both sides and use `read()` and `write()` to exchange the information. The tunnel will be shutdown in two conditions: one of the incoming information is empty or `select()` triggers timeout.

### telemetry
During the tunneling, both `read()` and `write()` would record amount of exchanged information in bytes. We just need to sum them up to get the total amount of exchanged information. To record the elapsed time for each established tunnel, we use `clock()` from `time.h`.

### blacklisting
The blacklist file is only read by the proxy during the initialization of the proxy, and the entries will be stored in blacklist. To decide whether a request targeting certain url should be blocked, we use `strstr()` to check the url against all the entries to see if the url contains any of the entries. Upon valid detection, error 403 Blacklisted URL will be raised to the browser.