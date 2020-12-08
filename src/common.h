#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <time.h>

#define DEFAULT_PORT (50000) 
#define DEFAULT_HOST "localhost"

#define CONNECTION_BACKLOG (5)
#define TRANSMIT_BUFFER_SIZE (100 * 1000)
#define RECEIVE_BUFFER_SIZE (100 * 1000)

#define DEFAULT_DIR "."
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define DEBUG 1
#define debug_print(fmt, ...) \
    do { if (DEBUG) { fprintf(stdout,"%.3f ", seconds_since_epoch()); fprintf(stdout, fmt, __VA_ARGS__); }} while (0);

#define exit_on_error(fmt, ...) \
    do { perror(__func__); fprintf(stderr, "error: " #fmt "\n", ## __VA_ARGS__); exit(EXIT_FAILURE); } while (0);

int transmit_data(int sock, char* buffer, size_t size);
int receive_data(int sock, char* buffer, size_t size);
double seconds_since_epoch(void);
void setup_sigint_handler(void (*handler)(int));
void sigint_handler(int sig);

#endif // _COMMON_H_
