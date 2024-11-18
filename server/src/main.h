#ifndef MAIN_SERVER_H
#define MAIN_SERVER_H

#include "../../libs/src/threads.h"

/* server's settings options */
typedef enum {
    OT_DAEMON,
    OT_ECHO,
    OT_MAX_FDS,
    OT_SERVER_LOG_LEVEL,
    OT_PORT,
    OT_SERVER_NAME,
    OT_THREADS,
    OT_WAIT_TIME,
    SERVER_OT_COUNT
} ServerOptionType;

void initialize_server_settings(void);

#endif