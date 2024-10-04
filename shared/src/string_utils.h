#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#define TO_STRING(string) #string
#define APPEND_PREFIX(prefix, string) prefix TO_STRING(string)

#define ARR_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define MAX_CHARS 512

typedef struct {
    char **strings;
    int stringLength;
    int capacity;
    int count;
} StringList;

StringList * create_string_list(int capacity, int stringLength);
void delete_string_list(StringList *stringList);

int is_string_list_empty(StringList *stringList);
int is_string_list_full(StringList *stringList);

void add_string_to_string_list(StringList *stringList, const char* string);
void remove_string_from_string_list(StringList *stringList);

int split_input_string( char *input, const char **tokens, int tkCount, int sep);
int concat_tokens(char *buffer, int buffSize, const char **tokens, int tkCount, const char *sep);
int count_tokens(const char *input, char delimiter);

void prepend_char(char *buffer, int size, const char *string, char ch);
void crlf_terminate(char *buffer, int size, const char *string);
int is_crlf_terminated(const char *string);

int is_valid_name(const char *name, int isChannel);

int safe_copy(char *buffer, int size, const char *string);

int str_to_uint(const char *string);
int uint_to_str(char *buffer, int size, unsigned number);

#endif