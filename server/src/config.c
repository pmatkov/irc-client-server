#include "config.h"

#include "../../libs/src/data_type.h"
#include "../../libs/src/enum_utils.h"
#include "../../libs/src/network_utils.h"
#include "../../libs/src/signal_handler.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct {
    ServerOptionType optionType;
    DataItem dataItem;
    DataType dataType;
} ServerOptions;

static const ServerOptions serverOptions[] = {
    {OT_DAEMON, {.itemInt = 0}, INT_TYPE},
    {OT_ECHO, {.itemInt = 0}, INT_TYPE},
    {OT_MAX_FDS, {.itemInt = 1024}, CHAR_TYPE},
    {OT_SERVER_LOG_LEVEL, {.itemInt = DEBUG}, INT_TYPE},
    {OT_SERVER_NAME, {.itemChar = "irc.server.com"}, CHAR_TYPE},
    {OT_PORT, {.itemInt = 50100}, INT_TYPE},
    {OT_THREADS, {.itemInt = 0}, INT_TYPE},
    {OT_WAIT_TIME, {.itemInt = 60}, INT_TYPE}
};

/* create a deamon process that will run in the background
    detached from the terminal */
void daemonize(void) {

    pid_t pid = fork();

    if (pid < 0) {
        FAILED(NO_ERRCODE, "Error creating daemon process");
    }
    else if (pid) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        FAILED(NO_ERRCODE, "Error creating daemon process");
    }

    set_sigaction(SIG_IGN, SIGHUP, NULL);

    pid = fork();

    if (pid < 0) {
        FAILED(NO_ERRCODE, "Error creating daemon process");
    }
    else if (pid) {
        exit(EXIT_SUCCESS);
    }

    enable_stderr_logging(0);
    enable_stdout_logging(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);

}

/* parse command line arguments */
void get_command_line_args(int argc, char **argv) {

    int opt;

    while ((opt = getopt(argc, argv, "deflnptw")) != -1) {

        switch (opt) {
            case 'd': {
                set_option_value(OT_DAEMON, &(int){1});
                break;
            }
            case 'e': {
                set_option_value(OT_ECHO, &(int){1});
                break;
            }
            case 'f': {
                int maxFds = str_to_uint(argv[3]);
                if (maxFds != -1) {
                    set_option_value(OT_MAX_FDS, &maxFds);
                }
                break;
            }
            case 'l': {

                LogLevel logLevel = string_to_enum_type(get_log_level_strings(), LOGLEVEL_COUNT, argv[4]);
                
                if (is_valid_enum_type(logLevel, LOGLEVEL_COUNT)) {
                    set_option_value(OT_SERVER_LOG_LEVEL, &logLevel);
                }
                break;
            }            
            case 'n': {
                set_option_value(OT_SERVER_NAME, argv[6]);
                break;
            }
            case 'p': {
                int port = str_to_uint(argv[5]);
                if (is_valid_port(port)) {
                    set_option_value(OT_PORT, &port);
                }
                break;
            }
            case 't': {
                set_option_value(OT_THREADS, &(int){str_to_uint(argv[7])});
                break;
            }
            case 'w': {
                int waitTime = str_to_uint(argv[8]);
                if (waitTime != -1) {
                    set_option_value(OT_WAIT_TIME, &waitTime);
                }
                break;
            }
            default:
                printf("Usage: %s [-d <daemon>] [-e <echo>] [-f <max fds>] [-l <loglevel>]  [-n <servername>] [-p <port>][-t <threads>] [-w <waittime>]\n", argv[0]);
                printf("\tOptions:\n");
                printf("\t  -d : Run as a daemon\n");
                printf("\t  -e : Enable echo mode\n");
                printf("\t  -f : Set max file descriptors\n");
                printf("\t  -l : Set the logging level\n");
                printf("\t  -n : Specify the server name\n");
                printf("\t  -p : Specify the port number\n");
                printf("\t  -t : Use multithreading\n");
                printf("\t  -w : Set wait time\n");
                exit(EXIT_FAILURE);
        }
    }
}

void initialize_server_settings(void) {

    register_option(INT_TYPE, OT_DAEMON, "daemon", &(int){serverOptions[OT_DAEMON].dataItem.itemInt});
    register_option(INT_TYPE, OT_ECHO, "echo", &(int){serverOptions[OT_ECHO].dataItem.itemInt});
    register_option(INT_TYPE, OT_MAX_FDS, "maxfds", &(int){serverOptions[OT_MAX_FDS].dataItem.itemInt});
    register_option(INT_TYPE, OT_SERVER_LOG_LEVEL, "loglevel", &(int){serverOptions[OT_SERVER_LOG_LEVEL].dataItem.itemInt});
    register_option(CHAR_TYPE, OT_SERVER_NAME, "servername", (char*)serverOptions[OT_SERVER_NAME].dataItem.itemChar);
    register_option(INT_TYPE, OT_PORT, "port",  &(int){serverOptions[OT_PORT].dataItem.itemInt});
    register_option(INT_TYPE, OT_THREADS, "threads", &(int){serverOptions[OT_THREADS].dataItem.itemInt});
    register_option(INT_TYPE, OT_WAIT_TIME, "waittime", &(int){serverOptions[OT_WAIT_TIME].dataItem.itemInt});
}
