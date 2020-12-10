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
#include <stdbool.h>

#include "common.h"

void print_help(char* progname) {
    printf("Usage: %s [-h <hostname>] [-p <port>] -f <filename>\n", progname);
    printf("       -h <host> - hostname to connect (default: %s)\n", DEFAULT_HOST);
    printf("       -p <port> - TCP port to connect (default: %d)\n", DEFAULT_PORT);
    printf("       -f <filename> - file to download\n");
    printf("       -c - continue file download\n");
}

int main(int argc, char ** argv)
{
    int opt;
    char* hostname = DEFAULT_HOST;
    char* filename = NULL;
    int port = DEFAULT_PORT;
    char receive_buffer[RECEIVE_BUFFER_SIZE];
    bool cont = false;

    errno = 0;
    setup_signal_handler(signal_handler);
    
    // process commandline parameters
    
    while ((opt = getopt(argc, argv, "h:p:f:c")) != -1) {
        switch (opt) {
        case 'h':
            hostname = optarg;
            break;
        case 'p':
            port = strtoul(optarg, NULL, 10);
            if (0 == port || errno) {
                fprintf(stderr, "error: invalid port number: %s\n", optarg);
                print_help(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;
        case 'f':
            filename = optarg;
            break;
        case 'c':
            cont = true;
            break;
        default: /* '?' */
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // create local file

    if (NULL == filename) {
        fprintf(stderr, "error: missing filename\n");
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int fd = -1;
    off_t file_size = 0;
    if (cont) {
        fd = open(filename, O_WRONLY, FILE_MODE);
        if (errno) {
            perror(NULL);
            exit_on_error("can't open file %s\n", filename);
        }
        
        struct stat st;
        fstat(fd, &st);
        file_size = st.st_size;
        
        debug_print("seeking to end: %ld\n", file_size);
        if ( 0 > lseek(fd, file_size, SEEK_END)) {
            perror(NULL);
            exit_on_error("can't seek in file %s\n", filename);
        }
    } else {
        fd = creat(filename, FILE_MODE); // we may end up with zero length file
        if (errno) {
            exit_on_error("can't create file %s\n", filename);
        }
    }

    // create socket 

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock <= 0) {
        exit_on_error("can't create socket\n");
    }

    int tcp_rcv_buf_size = TCP_RCV_BUFFER_SIZE;
    if (0 != setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)& tcp_rcv_buf_size, sizeof(tcp_rcv_buf_size))) {
        exit_on_error("can't set socket option\n");
    }
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    struct hostent * host = gethostbyname(hostname);
    if (!host){
        exit_on_error("unknown host %s\n", hostname);
    }
    memcpy(&address.sin_addr, host->h_addr_list[0], host->h_length);
    if (connect(sock, (struct sockaddr *)&address, sizeof(address))) {
        exit_on_error("can't connect to host %s\n", hostname);
    }

    // request file from server
    
    size_t filename_len = strlen(filename); // excluding \0
    request_t* request;
    size_t request_size = sizeof(header_t) + filename_len + 1; // including \0
    request = calloc(request_size, 1);
    if (NULL == request) {
        exit_on_error("can't allocate %lu bytes\n", filename_len);
    }
    request->header.offset = file_size;
    request->header.filename_len = filename_len;
    memcpy(request->filename, filename, filename_len);
    transmit_data(sock, (char*)request, request_size);
    free(request);
   
    while ( 1 ) {
        ssize_t chunk_size;
        chunk_size = receive_data(sock, receive_buffer, sizeof(receive_buffer));
        if (0 >= chunk_size) {
            // retry?
            break;
        }
        if (chunk_size != write(fd, receive_buffer, chunk_size)) {
            exit_on_error("can't  write to file %s\n", filename);
        }
    }
    
    // clean up
    close(fd);
    close(sock);

    return 0;
}
