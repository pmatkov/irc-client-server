#include "string_utils.h"
#include "error_control.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

StringList * create_string_list(int capacity, int stringLength) {
    
    StringList *stringList = (StringList *) malloc(sizeof(StringList));
    if (stringList == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    stringList->strings = (char **) malloc(capacity * sizeof(char *));
    if (stringList->strings  == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    for (int i = 0; i < capacity; i++) {

        stringList->strings[i] = (char*) malloc((stringLength + 1) * sizeof(char));
        if (stringList->strings[i] == NULL) {
            FAILED("Error allocating memory", NO_ERRCODE);
        }
    }

    stringList->stringLength = stringLength;
    stringList->capacity = capacity;
    stringList->count = 0;

    return stringList; 
}

void delete_string_list(StringList *stringList) {

    if (stringList != NULL) {

        for (int i = 0; i < stringList->capacity; i++) {
            free(stringList->strings[i]);
        }
        free(stringList->strings);
    }

    free(stringList);   
}

int is_string_list_empty(StringList *stringList) {

    if (stringList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return stringList->count == 0;
}

int is_string_list_full(StringList *stringList) {

    if (stringList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return stringList->count == stringList->capacity;
}

void add_string_to_string_list(StringList *stringList, const char* string) {

    if (stringList == NULL || string == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (!is_string_list_full(stringList)) {

        safe_copy(stringList->strings[stringList->count], stringList->stringLength + 1, string);

        stringList->count++;
    }
}

void remove_string_from_string_list(StringList *stringList) {

    if (stringList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (!is_string_list_full(stringList)) {

        memset(stringList->strings[stringList->count - 1], '\0', stringList->stringLength + 1);
        stringList->count--;
    }
}

/* splits string to tokens. If tkCount is less than word
 count, the last token will contain all the reamining words. */
int split_input_string(char *input, const char **tokens, int tkCount, int sep) {
     
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

    if (tkCount && tokenPtr[strlen(tokenPtr)-1] == sep) {
       tokenPtr[strlen(tokenPtr)-1] = '\0'; 
    }

    return i;

}

// concatenates tokens into a string
int concat_tokens(char *buffer, int size, const char **tokens, int tkCount, const char *sep) {

    if (buffer == NULL || tokens == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int i = 0;

    while (i < tkCount && tokens[i] != NULL) {

        if (strlen(tokens[i])) {

            strncat(buffer, tokens[i], size - strlen(buffer) - 1);
            strncat(buffer, sep, size - strlen(buffer) - 1);
        }
        i++;
    }

    if (strlen(buffer) && buffer[strlen(buffer) - 1] == sep[0]) {
        buffer[strlen(buffer) - 1] = '\0'; 
    }

    return i;
}

// counts tokens before delimiter
int count_tokens(const char *input, char delimiter) {

    if (input == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int count = 0;

    if (strlen(input)) {

        const char *inputPtr = input;
        count = 1;

        while (*inputPtr && *inputPtr != delimiter) {
            if (*inputPtr == ' ' && inputPtr - input != strlen(input)-1) {
                count++;
            }
            inputPtr++;
        }
    }

    return count;
}

// prepends char to string
void prepend_char(char *buffer, int size, const char *string, char ch) {

    if (buffer == NULL || string == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int len = strlen(string);

    if (size >= len + 1) {

        for (int i = len; i >= 0; i--) {
            buffer[i + 1] = string[i];
        }

        buffer[0] = ch;
    }
}

void crlf_terminate(char *buffer, int size, const char *string) {

    if (buffer == NULL || string == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (strlen(string) + strlen("\r\n") < size) {

        strcat(buffer, string);
        strcat(buffer, "\r\n");
    }
}

int is_crlf_terminated(const char *string) {

    if (string == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int terminated = 0;
    int len = strlen(string);

    if (len > strlen("\r\n") && string[len-2] == '\r' && string[len-1] == '\n') {
        terminated = 1;
    }

    return terminated;
}

/* checks if nickname or channel name 
contains valid chars according to IRC RFC*/
int is_valid_name(const char *name, int isChannel) {

    if (name == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    const char specialChars[] = {'#', '-', '_', '\\', '[', ']', '{', '}', '|', '^', '~'};

    int validName = 1;
    const char *ch = name;

    if (isChannel && *ch != '#') {
        validName = 0;
    }

    while (*ch && validName) {

        if (!isalnum(*ch)) {

            int validCh = 0;

            for (int i = 0; i < sizeof(specialChars)/ sizeof(specialChars[0] && !validCh); i++) {

                if (*ch == specialChars[i]) {
                    validCh = 1;
                }
            }
            validName = validCh; 
        }
        ch++;
    }

    return validName;
}

// copies string up to size chars from source to destination
int safe_copy(char *buffer, int size, const char *string) {

    if (buffer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (string == NULL) {
        string = "";
    }

    int copied = 0;
    int len = strnlen(string, size);
    
    if (len < size) {

        strcpy(buffer, string);

        copied = 1;
    }
    else if (size) {
        buffer[0] = '\0';
    }

    return copied;
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

// converts number from unsigned int to string representation 
int uint_to_str(char *buffer, int size, unsigned number) {

    if (buffer == NULL || !size) {
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
        buffer[digits-i-1] = number % 10 + '0';
        number /= 10;
    }

    buffer[digits] = '\0';

    return 1;
}
