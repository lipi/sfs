
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include "common.h"

ssize_t transmit_data(int sock, char* buffer, ssize_t size) {
    ssize_t bytes_sent = write(sock, buffer, size);
    debug_print("sent %ld bytes\n", bytes_sent);
    if (bytes_sent != size) {
        perror(__FUNCTION__); 
        exit(EXIT_FAILURE);
    }
    return bytes_sent;
}

ssize_t receive_data(int sock, char* buffer, ssize_t size) {
    ssize_t bytes_received = read(sock, buffer, size);
    debug_print("received %ld bytes\n", bytes_received);
    if ( bytes_received < 0 ) {
        perror(__FUNCTION__); 
    }
    return bytes_received;
}

double seconds_since_epoch(void) {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec + spec.tv_nsec / 1.0e9;
}

void setup_signal_handler(void (*handler)(int)) {
    // handle ctrl+c
    struct sigaction sa;

    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        exit_on_error("can't change sigaction for SIGINT\n");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        exit_on_error("can't change sigaction for SIGINT\n");
    }
}

void signal_handler(int sig __attribute__((unused))) {
    printf("\nExiting...\n");
    exit(EXIT_SUCCESS);
}

