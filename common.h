#define DEFAULT_PORT (50000) 
#define DEFAULT_HOST "localhost"

#define CONNECTION_BACKLOG (10)
#define TRANSMIT_BUFFER_SIZE (1000)
#define RECEIVE_BUFFER_SIZE (1000)

#define DEFAULT_DIR "."
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)


#define DEBUG 1
#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#define FATAL_ERROR(x) do { perror(__FUNCTION__); fprintf(stderr, ...); exit(EXIT_FAILURE); } while (0);

int transmit_data(int sock, char* buffer, size_t size) {
	size_t bytes_sent = write(sock, buffer, size);
    debug_print("sent %lu bytes\n", bytes_sent);
    if (bytes_sent != size) {
        perror(__FUNCTION__); 
        exit(EXIT_FAILURE);
    }
    return bytes_sent;
}

int receive_data(int sock, char* buffer, size_t size) {
	size_t bytes_received = read(sock, buffer, size);
    debug_print("received %lu bytes\n", bytes_received);
    return bytes_received;
}
