#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#define TO_STRING(string) #string
#define APPEND_PREFIX(prefix, string) prefix TO_STRING(string)

int split_input_string( char *input, const char **tokens, int tkCount, int sep);
int concat_tokens(char *buffer, int buffSize, const char **tokens, int tkCount, const char *sep);
void prepend_char(char *buffer, int size, const char *string, char ch);
int count_tokens(const char *input, char delimiter);

int is_valid_name(const char *name, int isChannel);

int safe_copy(char *buffer, int size, const char *string);

int str_to_uint(const char *string);
int uint_to_str(char *string, int size, unsigned number);

#endif