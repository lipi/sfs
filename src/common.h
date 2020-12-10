#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define DEFAULT_PORT (50000) 
#define DEFAULT_HOST "localhost"

#define CONNECTION_BACKLOG (5)
#define TRANSMIT_BUFFER_SIZE (100 * 1000)
#define RECEIVE_BUFFER_SIZE (100 * 1000)

#define TCP_RCV_BUFFER_SIZE (2 << 20)
#define TCP_SND_BUFFER_SIZE (2 << 20)

#define DEFAULT_DIR "."
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define DEBUG 1
#define debug_print(fmt, ...) \
    do { if (DEBUG) { fprintf(stdout,"%.3f ", seconds_since_epoch()); fprintf(stdout, fmt, __VA_ARGS__); }} while (0);

#define exit_on_error(fmt, ...) \
    do { perror(__func__); fprintf(stderr, "error: " #fmt "\n", ## __VA_ARGS__); exit(EXIT_FAILURE); } while (0);

typedef struct __attribute__((packed, aligned(4))) {
    uint64_t offset;
    uint32_t filename_len;
} header_t;

typedef struct __attribute__((packed, aligned(4))) {
    header_t header;
    char filename[1]; // first character of filename
} request_t;

ssize_t transmit_data(int sock, char* buffer, size_t size);
ssize_t receive_data(int sock, char* buffer, size_t size);
double seconds_since_epoch(void);
void setup_signal_handler(void (*handler)(int));
void signal_handler(int sig);

#endif // _COMMON_H_
