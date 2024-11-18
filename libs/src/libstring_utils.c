#include "string_utils.h"
#include "error_control.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

int tokenize_string(char *string, const char **tokens, int tkCount, const char *delim) {
     
    if (string == NULL || tokens == NULL || delim == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int count = 0;

    if (tkCount) {

        char *strPtr = string, *tokenPtr = string;

        while (count < tkCount - 1 && (strPtr = find_delimiter(strPtr, delim)) != NULL) {

            memset(strPtr, '\0', strlen(delim));
            strPtr += strlen(delim);

            tokens[count] = tokenPtr;
            tokenPtr = strPtr;
            count++;
        }

        if (strPtr != NULL && is_terminated(strPtr, delim)) {
            strPtr += strlen(strPtr) - strlen(delim);
            memset(strPtr, '\0', strlen(delim));
        }

        tokens[count] = tokenPtr;

        if (strlen(tokenPtr)) {
            count += 1;
        }
    }
    return count;

}

int concat_tokens(char *buffer, int size, const char **tokens, int tkCount, const char *delim) {

    if (buffer == NULL || tokens == NULL || delim == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int i = 0;

    while (i < tkCount && tokens[i] != NULL) {

        if (strlen(tokens[i])) {

            strncat(buffer, tokens[i], size - strlen(buffer));
            strncat(buffer, delim, size - strlen(buffer));
        }
        i++;
    }

    if (strlen(buffer) && buffer[strlen(buffer) - 1] == delim[0]) {
        buffer[strlen(buffer) - 1] = '\0'; 
    }

    return i;
}

int count_tokens(const char *string, const char *delim) {

    if (string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int count = strlen(string) ? 1 : 0;

    while (*string && (delim == NULL || *string != delim[0])) {
        if (*string == ' ' && *(string + 1) != '\0') {
            count++;
        }
        string++;
    }

    return count;
}

void prepend_char(char *buffer, int size, const char *string, char ch) {

    if (buffer == NULL || string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int len = strlen(string);

    if (len + 1 <= size) {

        for (int i = len; i >= 0; i--) {
            buffer[i + 1] = string[i];
        }
        buffer[0] = ch;
    }
}

int delimit_messages(char *string, const char **tokens, int tkCount, const char *delim) {

    if (string == NULL || tokens == NULL || delim == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int count = 0;

    if (tkCount) {

        char *strPtr = string, *tokenPtr = string;

        while ((strPtr = find_delimiter(strPtr, delim)) != NULL && count < tkCount) {

            memset(strPtr, '\0', strlen(delim));
            strPtr += strlen(delim);

            tokens[count] = tokenPtr;
            tokenPtr = strPtr;
            count++;
        }
    }
    return count;
}

int extract_message(char *buffer, int size, char *string, const char *delim) {

    if (buffer == NULL || string == NULL || delim == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    char *strPtr = string;
    int extracted = 0;
    
    if ((strPtr = find_delimiter(strPtr, delim)) != NULL) {

        memset(strPtr, '\0', strlen(delim));
        safe_copy(buffer, size, string);

        const char *remainingString = strPtr + strlen(delim);

        if (strlen(remainingString)) {
            memmove(string, remainingString, strlen(remainingString) + 1);
        }
        extracted = 1;
    }

    return extracted;
}
void process_messages(char *string, const char *delim, StringListFunc iteratorFunc, void *arg) {

    if (string == NULL || delim == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int len = strlen(string);
    int tkCount = count_delimiters(string, delim);

    if (tkCount) {

        const char **tokens = (const char **) malloc(tkCount * sizeof(const char*));
        if (tokens == NULL) {
            FAILED(NO_ERRCODE, "Error allocating memory");
        }

        delimit_messages(string, tokens, tkCount, delim);
        iterate_string_list(tokens, tkCount, iteratorFunc, arg);

        const char *partialMessage = tokens[tkCount - 1] + strlen(tokens[tkCount - 1]) + strlen(delim);

        if (strlen(partialMessage)) {
            memmove(string, partialMessage, strlen(partialMessage) + 1);
        }
        else {
            memset(string, '\0', len);
        }

        free(tokens);
    }
}

void terminate_string(char *buffer, int size, const char *string, const char *term) {

    if (buffer == NULL || string == NULL || term == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (strlen(string) + strlen(term) < size) {

        strcat(buffer, string);
        strcat(buffer, term);
    }
}

void clear_terminator(char *string, const char *term) {

    if (string == NULL || term == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int terminated = 1;

    int i = strlen(string) - 1;
    int j = strlen(term) - 1;

    while (i >= 0 && j >= 0 && terminated) {
        if (string[i] == term[j]) {
            string[i] = '\0';
            i--;
            j--;
        }
        else {
            terminated = 0;
        }
    }
}

int is_terminated(const char *string, const char *term) {

    if (string == NULL || term == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int terminated = 1;
    int i = strlen(string) - 1;
    int j = strlen(term) - 1;

    while (i >= 0 && j >= 0 && terminated) {
        if (string[i] == term[j]) {
            i--;
            j--;
        }
        else {
            terminated = 0;
        }
    }

    return terminated;
}

char * find_delimiter(const char *string, const char *delim) {

    if (string == NULL || delim == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return strstr(string, delim);
}

int count_delimiters(const char *string, const char *delim) {

    if (string == NULL || delim == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int count = 0;
    const char *strPtr = string;

    while ((strPtr = strstr(strPtr, delim)) != NULL) {
        strPtr += strlen(delim);
        count++;
    }

    return count;
}


void escape_crlf_sequence(char *buffer, int size, const char *string) {

    if (buffer == NULL || string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int count = count_delimiters(string, CRLF);

    if ((strlen(string) + strlen(CRLF) * count) < size) {
 
        for (int i = strlen(string); i >= 0; i--) {

            if ((string[i] == '\r' || string[i] == '\n')) {

                buffer[i] = string[i] == '\r' ? 'r' : 'n';
                memmove(buffer + i + 1, buffer + i, strlen(buffer + i));
                buffer[i] = '\\';
            }
            else {
                buffer[i] = string[i];
            }
        }
    }
}

int count_format_specifiers(const char *string) {

    if (string == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int fsCount = 0;

    while (*string) {
        if (*string == '%') {
            fsCount++;
        }
        string++;
    }

    return fsCount;
}

int is_valid_name(const char *name, int isChannel) {

    if (name == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    /* a list of allowed special chars */
    const char specialChars[] = {'#', '-', '_', '\\', '[', ']', '{', '}', '|', '^', '~'};

    int validName = 1;
    const char *ch = name;

    if (isChannel && *ch != '#') {
        validName = 0;
    }

    while (*ch && validName) {

        if (!isalnum(*ch)) {

            int validChar = 0;

            for (int i = 0; i < sizeof(specialChars)/ sizeof(specialChars[0] && !validChar); i++) {

                if (*ch == specialChars[i]) {
                    validChar = 1;
                }
            }
            validName = validChar; 
        }
        ch++;
    }

    return validName;
}

void iterate_string_list(const char **stringList, int size, StringListFunc iteratorFunc, void *arg) {

    if (stringList == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    for (int i = 0; i < size && stringList[i] != NULL; i++) {
        iteratorFunc(stringList[i], arg);
    }
}

void add_string_length(const char *string, void *arg) {

    if (string != NULL && arg != NULL) {
        *((int*)arg) += strlen(string);
    }
}


int safe_copy(char *buffer, int size, const char *string) {

    if (buffer == NULL) {
        FAILED(ARG_ERROR, NULL);
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
    // else if (size) {
    //     buffer[0] = '\0';
    // }

    return copied;
}

int str_to_uint(const char *string) {

    char *end = NULL;
    errno = 0;

    long number = strtol(string, &end, 0);

    if (string == end || errno == ERANGE || *end != '\0' || number < 0) {
        number = -1;
    }

    return number;

}

int uint_to_str(char *buffer, int size, unsigned number) {

    if (buffer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int comp = 10, digits = 1, converted = 0;

    while (number >= comp) {
        comp *= 10;
        digits++;
    }

    if (digits <= size) {

        for (int i = 0; i < digits; i++) {
            buffer[digits-i-1] = number % 10 + '0';
            number /= 10;
        }

        converted = 1;
    }

    return converted;
}
