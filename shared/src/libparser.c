#include "parser.h"

#include "error_control.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/* splits string to tokens. If tkCount is less
 than word count, the last token will contain
  all the reamining words. */
int split_input_string(char *input, char **tokens, int tkCount, int sep) {
     
    if (input == NULL || tokens == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char *tokenPtr = input, *sepPtr = NULL;

    int i = 0;
    int len = strlen(input);

    while (i < (tkCount - 1) && (tokenPtr - input) < len && (sepPtr = strchr(tokenPtr, sep)) != NULL) {

        if (tokenPtr != sepPtr) {
            *sepPtr = '\0';
            tokens[i++] = tokenPtr;
            tokenPtr = sepPtr + 1;
        }
    }

    if (tkCount && (tokenPtr - input) < len) {
        tokens[i] = tokenPtr;
        i += 1;
    }

    return i;

}

// concatenates tokens into string
int concat_tokens(char *buffer, int buffSize, char **tokens, int tkCount, const char *sep) {

    if (buffer == NULL || tokens == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int i = 0;

    while (i < tkCount) {

        int len = strlen(buffer);

        strncat(buffer, tokens[i], buffSize - len - 1);

        if (i < tkCount - 1) {
            strncat(buffer, sep, buffSize - len - 1);
        }
        i++;
    }

    return i;
}

// converts number from string representation to unsigned int
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

int uint_to_str(char *string, int size, unsigned number) {

    if (string == NULL || !size) {
        return 0;
    }

    unsigned numberCp = number;
    int digits = 0;

    do {
        numberCp /= 10;
        digits++;
    } while (numberCp);

    if (size < digits + 1) {
        return 0;
    }

    for (int i = 0; i < digits; i++) {
        string[digits-i-1] = number % 10 + '0';
        number /= 10;
    }

    string[digits] = '\0';

    return 1;
}
