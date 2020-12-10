/*
 * Simple File Server - https://github.com/lipi/sfs
 * 
 * File transfer client, see server.c for server code.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"

void print_help(char* progname) {
    printf("Usage: %s [-h <hostname>] [-p <port>] -f <filename>\n", progname);
    printf("       -h <host> - hostname to connect (default: %s)\n", DEFAULT_HOST);
    printf("       -p <port> - TCP port to connect (default: %d)\n", DEFAULT_PORT);
    printf("       -f <filename> - file to download\n");
}

int main(int argc, char ** argv)
{
    int opt;
    char* hostname = DEFAULT_HOST;
    char* filename = NULL;
    int port = DEFAULT_PORT;
    size_t len;
    char receive_buffer[RECEIVE_BUFFER_SIZE];

    setup_sigint_handler(sigint_handler);
    
    // process commandline parameters
    
    while ((opt = getopt(argc, argv, "h:p:f:")) != -1) {
        switch (opt) {
        case 'h':
            hostname = optarg;
            break;
        case 'p':
            port = strtoul(optarg, NULL, 10);
            if (0 == port || errno) {
                fprintf(stderr, "%s: error: invalid port number: %s\n", argv[0], optarg);
                print_help(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;
        case 'f':
            filename = optarg;
            break;
        default: /* '?' */
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // create local file
    
    if (NULL == filename) {
        fprintf(stderr, "%s: error: missing filename\n", argv[0]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = creat(filename, FILE_MODE);
    if (errno) {
        exit_on_error("%s: error: can't create file %s\n", argv[0], filename);
    }
    
    // create socket 

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock <= 0) {
        exit_on_error("%s: error: can't create socket\n", argv[0]);
    }

    int tcp_rcv_buf_size = 2 << 20;
    if (0 != setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)& tcp_rcv_buf_size, sizeof(tcp_rcv_buf_size))) {
        exit_on_error("%s: error: can't set socket option\n", argv[0]);
    }
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    struct hostent * host = gethostbyname(hostname);
    if (!host){
        exit_on_error("%s: error: unknown host %s\n", argv[0], hostname);
    }
    memcpy(&address.sin_addr, host->h_addr_list[0], host->h_length);
    if (connect(sock, (struct sockaddr *)&address, sizeof(address))) {
        exit_on_error("%s: error: can't connect to host %s\n", argv[0], hostname);
    }

    // request file from server
    
    len = strlen(filename);
    transmit_data(sock, (char*)&len, sizeof(len));
    transmit_data(sock, filename, len);

    size_t remaining_size = len;
    while (remaining_size > 0 ) {
        size_t chunk_size;
        chunk_size = receive_data(sock, receive_buffer, sizeof(receive_buffer));
        if (0 == chunk_size) {
            // retry?
            break;
        }
        if (chunk_size != write(fd, receive_buffer, chunk_size)) {
            exit_on_error("%s: error: can't  write to file %s\n", argv[0], filename);
        }
        remaining_size -= chunk_size;
    }
    
    // clean up
    close(fd);
    close(sock);

    return 0;
}
