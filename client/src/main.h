#ifndef MAIN_CLIENT_H
#define MAIN_CLIENT_H

#include "../../libs/src/settings.h"

/* client's settings options */
typedef enum {
    OT_NICKNAME,
    OT_USERNAME,
    OT_REALNAME,
    OT_SERVER_ADDRESS,
    OT_SERVER_PORT,
    OT_COLOR,
    OT_CLIENT_LOG_LEVEL,
    OT_MULTIPLIER,
    CLIENT_OT_COUNT
} ClientOptionType;

/* initialize default settings and load
    settings from file */
void initialize_client_settings(void);

// int get_client_pipe_fd(void);

#endif