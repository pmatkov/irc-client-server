#ifndef MAIN_H
#define MAIN_H

#include "../../libs/src/settings.h"

/* clientProperties contains client's
    configurable settings */

typedef enum {
    CP_NICKNAME,
    CP_USERNAME,
    CP_REALNAME,
    CP_ADDRESS,
    CP_PORT
} ClientProperties;


/* set default settings and read settings
    from the file, if available */
void initialize_settings(Settings *settings, LookupTable *lookupTable);

#endif