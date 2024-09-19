#define _XOPEN_SOURCE 700

#include "signal_handler.h"
#include "error_control.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void set_sigaction(void (*handler)(int), int sig) {

    struct sigaction sa;

    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(sig, &sa, NULL) == -1) {
        FAILED("Failed to set signal handler", NO_ERRCODE);
    }
}

void handle_sigint(int sig) {

    exit(EXIT_SUCCESS);
}