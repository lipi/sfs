
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include "common.h"

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

double seconds_since_epoch(void) {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec + spec.tv_nsec / 1.0e9;
}

void setup_sigint_handler(void (*handler)(int)) {
    // handle ctrl+c
    struct sigaction sa;

    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        exit_on_error("error: can't change sigaction for SIGINT\n");
    }
}

void sigint_handler(int sig) {
    printf("\nExiting...\n");
    exit(EXIT_SUCCESS);
}

