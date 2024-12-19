#ifndef CONFIG_H
#define CONFIG_H

#include "../../libs/src/settings.h"

/* client options */
typedef enum {
    OT_NICKNAME,
    OT_USERNAME,
    OT_REALNAME,
    OT_SERVER_ADDRESS,
    OT_SERVER_PORT,
    OT_COLOR,
    OT_HISTORY,
    OT_CLIENT_LOG_LEVEL,
    OT_SCROLLBACK,
    CLIENT_OT_COUNT
} ClientOptionType;

/* parse command line arguments */
void get_command_line_args(int argc, char **argv);

/* initialize default settings and/ or load
    from file */
void initialize_client_settings(void);

#endif