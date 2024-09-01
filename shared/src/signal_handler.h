#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

void set_sigaction(void (*handler)(int), int sig);
void handle_sigint(int sig);

#endif