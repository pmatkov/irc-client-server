#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include "threads.h"

#include <signal.h>

/* set signal handler */
void set_sigaction(void (*handler)(int), int sig, int *blockedSig);

/* signal handlers */
void handle_server_sigint(int sig);
void handle_client_sigint(int sig);
void handle_sigalrm(int sig);
void handle_sigwinch(int sig);

void set_thread_pool_callback(NotifyThreadFunc func);
void set_thread_callback(NotifyThreadFunc func);
void set_thread_pool(ThreadPool *pool);
void set_thread(Thread *th);

void set_client_pipe(int pipeFd);
void set_server_pipe(int pipeFd);

#endif