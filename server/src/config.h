#ifndef CONFIG_H
#define CONFIG_H

#include "../../libs/src/settings.h"

/* server settings */
typedef enum {
    OT_DAEMON,
    OT_ECHO,
    OT_MAX_FDS,
    OT_SERVER_LOG_LEVEL,
    OT_SERVER_NAME,
    OT_PORT,
    OT_THREADS,
    OT_WAIT_TIME,
    SERVER_OT_COUNT
} ServerOptionType;

/* run server as daemon process */
void daemonize(void);

/* parse command line arguments */
void get_command_line_args(int argc, char **argv);

/* initialize default settings */
void initialize_server_settings(void);

#endif