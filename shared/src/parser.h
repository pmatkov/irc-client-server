#ifndef PARSER_H
#define PARSER_H

int split_input_string( char *input, char **tokens, int tkCount);
int concat_tokens(char *buffer, int buffSize, char **tokens, int tkCount);
int str_to_uint(const char *string);

#endif