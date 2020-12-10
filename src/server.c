/*
 * Simple File Server - https://github.com/lipi/sfs
 * 
 * - serves files for reading (req#1)
 * - can handle multiple concurrent clients (req#2)
 * - no authentication or encryption (req#3-1)
 * - serves files from a single directory (no recursion) (req#3-2)
 * - optimized for narrow bandwidth, high latency (req#4)
 * - no dependencies (req#5)
 * - works on Linux
 * 
 * See client.c for client code.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "common.h"

typedef struct {
    int sock;
    struct sockaddr address;
    unsigned int addr_len;
} connection_t;

void print_help(char* progname) {
    printf("Usage: %s [-p <port>] [-d <directory>]\n", progname);
    printf("       -p <port> - TCP port to listen on (default: %d)\n", DEFAULT_PORT);
    printf("       -d <directory> - directory to serve files from (default: current)\n");
}

void * connection_handler(void * ptr) {
    int fd = -1;
    char* filename = NULL;
    char ipv4_address[16]; // i.e. "123.123.123.123\0"
                               
    if (NULL == ptr) {
        pthread_exit(0);
    }
    
    connection_t * conn = (connection_t *)ptr;
    long addr = (long)((struct sockaddr_in *)&conn->address)->sin_addr.s_addr;
    snprintf(ipv4_address, sizeof(ipv4_address), "%d.%d.%d.%d",
             (int)((addr      ) & 0xff),
             (int)((addr >>  8) & 0xff),
             (int)((addr >> 16) & 0xff),
             (int)((addr >> 24) & 0xff));

    header_t header;
    if (0 > receive_data(conn->sock, (char*)&header, sizeof(header)) ) {
        perror(NULL);
        fprintf(stderr, "can't receive from client %s", ipv4_address);
        goto disconnect;
    }
    
    if (header.filename_len > 0)
    {
        size_t num_bytes = (header.filename_len + 1) * sizeof(char);
        filename = (char *)calloc(num_bytes, 1);
        if (NULL == filename) {
            perror(NULL);
            fprintf(stderr, "can't allocate %lu bytes", num_bytes);
            goto disconnect;
        }
        
        if ( 0 > receive_data(conn->sock, filename, num_bytes) ) {
            perror(NULL);
            fprintf(stderr, "can't receive from client %s", ipv4_address);
            goto disconnect;
        }

        // TODO: mutex to avoid garbled lines
        printf("%s: %s - download started\n", ipv4_address, filename);

        fd = open(filename, O_RDONLY);
        if ( fd < 0 ) {
            perror(NULL);
            fprintf(stderr, "can't open file: %s\n", filename);
            goto disconnect;
        }

        // TODO: offset
        
        size_t chunk_size;
        size_t total_size = 0;
        char buffer[TRANSMIT_BUFFER_SIZE];
        do {
            chunk_size = read(fd, buffer, sizeof(buffer));
            if (chunk_size < 0 ) {
                perror(NULL);
                fprintf(stderr, "can't read from file: %s\n", filename);
                goto close_file;
                            
            }
            if (0 == chunk_size) {
                break;
            }
            if (chunk_size != transmit_data(conn->sock, buffer, chunk_size)) {
                fprintf(stderr, "can't send data to client\n");
                goto close_file;
            }
            total_size += chunk_size;
        } while (chunk_size > 0);
        
        // TODO: mutex to avoid garbled lines
        printf("%s: %s - all data sent (%lu bytes)\n", ipv4_address, filename, total_size);
        // 'sent', i.e. not guaranteed to be received
    }

  close_file:
    close(fd);
    free(filename);

  disconnect:   
    close(conn->sock);
    free(conn);
    pthread_exit(0);
}

int main(int argc, char* argv[])
{
    int opt;
    int port = DEFAULT_PORT;
    char* dirname = DEFAULT_DIR;
    int sock = -1;
    struct sockaddr_in address;
    connection_t * connection;
    pthread_t thread;

    errno = 0;
    setup_signal_handler(signal_handler);

    // process commandline parameters
    
    while ((opt = getopt(argc, argv, "p:d:")) != -1) {
        switch (opt) {
        case 'p':
            port = strtoul(optarg, NULL, 10);
            if (0 == port || errno) {
                fprintf(stderr, "error: invalid port number: %s\n", optarg);
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            dirname = optarg;
            break;
        default: /* '?' */
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (0 != chdir(dirname)) {
        exit_on_error("error: can't change to directory: %s\n", dirname);
    }

    // create socket
    
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock <= 0) {
         exit_on_error("error: cannot create socket\n");
        exit(EXIT_FAILURE);
    }

    int tcp_snd_buf_size = TCP_SND_BUFFER_SIZE; // due to high latency, see req#4
    if (0 != setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)& tcp_snd_buf_size, sizeof(tcp_snd_buf_size))) {
        perror(__func__);
        fprintf(stderr, "error: can't set socket option\n");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0) {
        fprintf(stderr, "error: cannot bind socket to port %d\n", port);
        exit(EXIT_FAILURE);
    }

    if (listen(sock, CONNECTION_BACKLOG) < 0) {
        fprintf(stderr, "error: cannot listen on port\n");
        exit(EXIT_FAILURE);
    }

    printf("listening on port %u and serving files from %s\n", port, dirname);

    // start serving requests concurrently (see req#2)
    
    while (1) {
        connection = (connection_t *)calloc(1, sizeof(connection_t)); // freed in the handler
        connection->sock = accept(sock, &connection->address, &connection->addr_len);
        if (connection->sock <= 0) {
            free(connection);
        }
        else {
            pthread_create(&thread, 0, connection_handler, (void *)connection);
            pthread_detach(thread);
        }
    }
    
    return 0;
}
