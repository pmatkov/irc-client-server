#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

void set_sigaction(void (*handler)(int), int sig);
void handle_sigint(int sig);
void handle_sigwinch(int sig);

int get_resized(void);
void set_resized(int resized);

#endif