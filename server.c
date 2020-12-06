/*
 * Simple File Server https://github.com/lipi/fts
 * 
 * - serves files for reading (req#1)
 * - can handle multiple concurrent clients (req#2)
 * - no authentication or encryption (req#3-1)
 * - serves files from a single directory (no recursion) (req#3-2)
 * - optimized for narrow bandwidth, high latency (req#4)
 * - no dependencies (req#5)
 * - works on Linux
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

#include "common.h"

void print_help(char* progname) {
    printf("Usage: %s [-p <port>] [-d <directory>]\n", progname);
    printf("       -p <port> - TCP port to listen on (default: %d)\n", DEFAULT_PORT);
    printf("       -d <directory> - directory to serve files from (default: current)\n");
}

typedef struct {
	int sock;
	struct sockaddr address;
	int addr_len;
} connection_t;

void * connection_handler(void * ptr) {
	size_t len;
	connection_t * conn;

	if (NULL == ptr) {
        pthread_exit(0);
    }
    
	conn = (connection_t *)ptr;

	receive_data(conn->sock, (char*)&len, sizeof(len));
	if (len > 0)
	{
		long addr = (long)((struct sockaddr_in *)&conn->address)->sin_addr.s_addr;
		char* filename = (char *)malloc((len+1) * sizeof(char));
        if (NULL == filename) {
            perror(__FUNCTION__);
            exit(EXIT_FAILURE);
        }
		filename[len] = 0;
		receive_data(conn->sock, filename, len);

        // TODO: mutex to avoid garbled lines
		printf("%d.%d.%d.%d: %s - download started\n",
               (int)((addr      ) & 0xff),
               (int)((addr >>  8) & 0xff),
               (int)((addr >> 16) & 0xff),
               (int)((addr >> 24) & 0xff),
               filename);

        int fd = open(filename, O_RDONLY);
        if ( fd < 0 ) {
            perror(__FUNCTION__);
            fprintf(stderr, "error: can't open file: %s\n", filename);
            exit(EXIT_FAILURE);
        }
        size_t chunk_size;
        size_t total_size = 0;
        char buffer[TRANSMIT_BUFFER_SIZE];
        do {
            chunk_size = read(fd, buffer, sizeof(buffer));
            if (chunk_size < 0 ) {
                perror(__FUNCTION__);
                exit(EXIT_FAILURE);
            }
            if (0 == chunk_size) {
                break;
            }
            if (chunk_size != transmit_data(conn->sock, buffer, chunk_size)) {
                perror(__FUNCTION__);
                exit(EXIT_FAILURE);
            }
            total_size += chunk_size;
        } while (chunk_size > 0);
        
        // TODO: mutex to avoid garbled lines
		printf("%d.%d.%d.%d: %s - download finished (%lu bytes)\n",
               (int)((addr      ) & 0xff),
               (int)((addr >>  8) & 0xff),
               (int)((addr >> 16) & 0xff),
               (int)((addr >> 24) & 0xff),
               filename, total_size);

        close(fd);
		free(filename);
	}

    // clean up
    
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

    // process commandline parameters
    
    while ((opt = getopt(argc, argv, "p:d:")) != -1) {
        switch (opt) {
        case 'p':
            port = strtoul(optarg, NULL, 10);
            if (0 == port || errno) {
                fprintf(stderr, "%s: error: invalid port number: %s\n", argv[0], optarg);
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
        fprintf(stderr, "%s: error: can't change to directory: %s\n", argv[0], dirname);
        exit(EXIT_FAILURE);
    }

    // create socket
    
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock <= 0) {
		fprintf(stderr, "%s: error: cannot create socket\n", argv[0]);
        exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "%s: error: cannot bind socket to port %d\n", argv[0], port);
        exit(EXIT_FAILURE);
	}

	if (listen(sock, CONNECTION_BACKLOG) < 0) {
		fprintf(stderr, "%s: error: cannot listen on port\n", argv[0]);
        exit(EXIT_FAILURE);
	}

	printf("%s: listening on port %u and serving files from %s\n", argv[0], port, dirname);

    // start serving requests
    
	while (1) {
		connection = (connection_t *)malloc(sizeof(connection_t)); // freed in the handler
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
