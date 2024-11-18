#define _XOPEN_SOURCE 700

#include "signal_handler.h"
#include "../../server/src/main.h"
#include "../../client/src/main.h"
#include "logger.h"
#include "error_control.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define UNASSIGNED_FD -1

static NotifyThreadFunc notifyPoolFunc = NULL;
static NotifyThreadFunc notifyThreadFunc = NULL;
static ThreadPool *threadPool = NULL;
static Thread *thread = NULL;

static int clientPipeFd = UNASSIGNED_FD;
static int serverPipeFd = UNASSIGNED_FD;

void set_sigaction(void (*handler)(int), int sig, int *blockedSig) {

    struct sigaction sa;

    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    /* block specified signals when the signal handler is
        executing */
    while (blockedSig != NULL && *blockedSig) {
        sigaddset(&sa.sa_mask, *blockedSig);
        blockedSig++;
    }

    if (sigaction(sig, &sa, NULL) == -1) {
        FAILED(NO_ERRCODE, "Failed to set signal handler");
    }
}

/* signal handlers below utilize the self-pipe trick
    for signal handling with poll(). it works like this.
    first, a pipe is created and its file decriptor is 
    added to the set of fd's monitored by poll(). when a
    signal arrrives, poll() is interrrupted and control is 
    transfered to the signal handler. in the handler, a 
    message indicating the type of the signal is written to 
    the pipe. poll() is then restarted and an input event is
    generated for the pipe. finally, the message is read
    from the pipe and the signal is handled */
void handle_server_sigint(int sig) {
                 
    int errnosv = errno;
    const char *message = "sigint\r\n";

    if (thread != NULL && notifyThreadFunc != NULL) {

        notifyThreadFunc(thread, message);
    }
    if (threadPool != NULL && notifyPoolFunc != NULL) {

        notifyPoolFunc(threadPool, message);
    }
    if (serverPipeFd != UNASSIGNED_FD) {
        /* send message to the main thread to initiate cleanup */
        write(serverPipeFd, message, strlen(message));
    }

    errno = errnosv;
    // exit(EXIT_SUCCESS);
}

void handle_client_sigint(int sig) {

    int errnosv = errno;
    const char *message = "sigint\r\n";

    if (clientPipeFd != UNASSIGNED_FD) {
        write(clientPipeFd, message, strlen(message));
    }
    
    errno = errnosv;
    // exit(EXIT_SUCCESS);
}

/* SIGALRM is generated on expired interval timer */
void handle_sigalrm(int sig) {

    int errnosv = errno;
    const char *message = "sigalrm\r\n";

    if (clientPipeFd != UNASSIGNED_FD) {
        write(clientPipeFd, message, strlen(message));
    }
    errno = errnosv;
    // windowResized = 1;
}

/* SIGWINCH is generated on window resize */
void handle_sigwinch(int sig) {

    int errnosv = errno;
    const char *message = "sigwinch\r\n";

    if (clientPipeFd != UNASSIGNED_FD) {
        write(clientPipeFd, message, strlen(message));
    }
    errno = errnosv;
    // windowResized = 1;
}

void set_thread_pool_callback(NotifyThreadFunc func) {

    notifyPoolFunc = func;
}

void set_thread_callback(NotifyThreadFunc func) {

    notifyThreadFunc = func;
}

void set_thread_pool(ThreadPool *pool) {

    threadPool = pool;
}

void set_thread(Thread *th) {

    thread = th;
}

void set_client_pipe(int pipeFd) {

    clientPipeFd = pipeFd;
}

void set_server_pipe(int pipeFd) {

    serverPipeFd = pipeFd;
}