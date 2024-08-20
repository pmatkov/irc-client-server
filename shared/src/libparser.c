#include "parser.h"
#include "../../server/src/main.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define MAX_ARGS 3

/* splits input string to tokens, if tkCount is less than maximum possible tokens, 
the last token will be a group. For example: "this is a sequence of tokens", tkCount = 3,
result "this", "is", "a sequence of tokens" */
int split_input_string(char *input, char **tokens, int tkCount) {
    if (input == NULL || tokens == NULL || tkCount <= 0) {
        return 0;
    }

    char *token = NULL, *savePtr = NULL;
    int i;

    for (i = 0; i < tkCount - 1; i++) {

        token = strtok_r(savePtr ? NULL : input, " ", &savePtr);

        if (token != NULL) {
        
            tokens[i] = token;
        }
        else {
            break;
        }
    }
    tokens[i] = savePtr;

    if (tokens[i][0] != '\0') {
        i += 1;
    }

    return i;
}

// concatenate tokens
int concat_tokens(char *buffer, int buffSize, char **tokens, int tkCount) {

    if (buffer == NULL || tokens == NULL) {
        return 0;
    }

    int i = 0;
    while (i < tkCount) {
        strncat(buffer, tokens[i], buffSize - strlen(buffer) - 1);

        if (i < tkCount - 1) {
            strncat(buffer, " ", buffSize - strlen(buffer) - 1);
        }
        i++;
    }

    return i;
}

// converts string representation of a number to unsigned int
int str_to_uint(const char *string) {

    char *end = NULL;
    errno = 0;

    long n = strtol(string, &end, 0);

    if (string == end || errno == ERANGE || *end != '\0' || n < 0) {
        return -1;
    }
    else {
        return n;
    }

}


