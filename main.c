/*
 * @Date: 2021-10-09 08:57:47
 * @LastEditTime: 2021-10-09 10:43:09
 * @Description: Proxy
 * @FilePath: /Proxy/proxy.c
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <port> <flag_telemetry> <filename of blacklist>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    
    
}