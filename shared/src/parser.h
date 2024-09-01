#ifndef PARSER_H
#define PARSER_H

#define ARR_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


int split_input_string( char *input, char **tokens, int tkCount, int sep);
int concat_tokens(char *buffer, int buffSize, char **tokens, int tkCount, const char *sep);

int str_to_uint(const char *string);
int uint_to_str(char *string, int size, unsigned number);

#endif