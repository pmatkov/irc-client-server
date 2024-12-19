#include "config.h"

#include "../../libs/src/session_state.h"
#include "../../libs/src/enum_utils.h"
#include "../../libs/src/logger.h"
#include "../../libs/src/error_control.h"

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <pwd.h>
#include <sys/types.h>
#include <assert.h>

typedef struct {
    ClientOptionType optionType;
    DataItem dataItem;
    DataType dataType;
} ClientOptions;

static const ClientOptions clientOptions[] = {
    {OT_NICKNAME, {.itemChar = "anonymous"}, CHAR_TYPE},
    {OT_USERNAME, {.itemChar = "anonymous"}, CHAR_TYPE},
    {OT_REALNAME, {.itemChar = "anonymous"}, CHAR_TYPE},
    {OT_SERVER_ADDRESS, {.itemChar = "localhost"}, CHAR_TYPE},
    {OT_SERVER_PORT, {.itemInt = 50100}, INT_TYPE},
    {OT_COLOR, {.itemInt = 1}, INT_TYPE},
    {OT_HISTORY, {.itemInt = 10}, INT_TYPE},
    {OT_CLIENT_LOG_LEVEL, {.itemInt = (int){DEBUG}}, INT_TYPE},
    {OT_SCROLLBACK, {.itemInt = 5}, INT_TYPE},
};

void get_command_line_args(int argc, char **argv) {

    int opt;

    struct option longOptions[] = {
        {"coloroff", no_argument, NULL, 'c'},
        {"history", no_argument, NULL, 'h'},
        {"loglevel", required_argument, NULL, 'l'},
        {"scrollback", required_argument, NULL, 's'},
        {0, 0, NULL, 0}
    };

    while ((opt = getopt_long(argc, argv, "", longOptions, NULL)) != -1) {
        
        switch (opt) {    

            case 'c':
                if (optarg) {
                    int color = str_to_uint(optarg);
                    if (color == 0 || color == 1) {
                        set_option_value(OT_COLOR, &color);
                    }
                }
                break;
            case 'h':
                if (optarg) {
                    int history = str_to_uint(optarg);
                    if (history >= 1 && history <= clientOptions[OT_HISTORY].dataItem.itemInt) {
                        set_option_value(OT_HISTORY, &history);
                    }
                }
                break;
            case 'l':
                if (optarg) {
                    LogLevel logLevel = string_to_enum_type(get_log_level_strings(), LOGLEVEL_COUNT, optarg);
                    if (is_valid_enum_type(logLevel, LOGLEVEL_COUNT)) {
                        set_option_value(OT_CLIENT_LOG_LEVEL, &logLevel);
                    }
                }
                break;

            case 's':
                if (optarg) {
                    int multiplier = str_to_uint(optarg);
                    if (multiplier >= 1 && multiplier <= clientOptions[OT_SCROLLBACK].dataItem.itemInt) {
                        set_option_value(OT_SCROLLBACK, &multiplier);
                    }
                }
                break;
            default:
                printf("Usage: %s [--coloroff] [--history] [--loglevel <debug | info | warning | error>] [--scrollback <1-5>]\n", argv[0]);
                printf("\tOptions:\n");
                printf("\t  --coloroff      : Disable colors for the user interface\n");
                printf("\t  --history       : Set max command history (1-20)\n");
                printf("\t  --loglevel      : Set the logging level\n");
                printf("\t  --scrollback:   : Set the scrollback buffer multiplier (1-5)\n");
                exit(EXIT_FAILURE);
        }
    }
}

void initialize_client_settings(void) {

    /* get username of the current user */
    struct passwd *userRecord = getpwuid(getuid());
    char *name = userRecord != NULL ? userRecord->pw_name : "";

    register_option(CHAR_TYPE, OT_NICKNAME, "nickname", name);
    register_option(CHAR_TYPE, OT_USERNAME, "username", name);
    register_option(CHAR_TYPE, OT_REALNAME, "realname", (char*) clientOptions[OT_REALNAME].dataItem.itemChar);
    register_option(CHAR_TYPE, OT_SERVER_ADDRESS, "address", (char*) clientOptions[OT_SERVER_ADDRESS].dataItem.itemChar);
    register_option(INT_TYPE, OT_SERVER_PORT, "port", &(int){clientOptions[OT_SERVER_PORT].dataItem.itemInt});
    register_option(INT_TYPE, OT_COLOR, "color", &(int){clientOptions[OT_COLOR].dataItem.itemInt});
    register_option(INT_TYPE, OT_HISTORY, "history", &(int){clientOptions[OT_HISTORY].dataItem.itemInt});
    register_option(INT_TYPE, OT_CLIENT_LOG_LEVEL, "loglevel", &(int){clientOptions[OT_CLIENT_LOG_LEVEL].dataItem.itemInt});
    register_option(INT_TYPE, OT_SCROLLBACK, "multiplier", &(int){clientOptions[OT_SCROLLBACK].dataItem.itemInt});

    read_settings(NULL);
}

