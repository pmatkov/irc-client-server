#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#define TO_STRING(string) #string
#define APPEND_PREFIX(prefix, string) prefix TO_STRING(string)

#define ARR_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define MAX_CHARS 512
#define MAX_DIGITS 10
#define CRLF "\r\n"

typedef void (*StringListFunc)(const char *string, void *arg);

/* tokenize string using delim. tkCount represents the 
    desired number of tokens. if tkCount is less than 
    the maximum possible number of tokens, the last 
    token will contain all the remaining tokens. this 
    function modifies the original string. 

    -example-
        string: "This is a long string", tkCount = 3, delim = ' '
        result: "This", "is", "a long string" */
int tokenize_string(char *string, const char **tokens, int tkCount, const char *delim);

/* concatenate tokens into a string using delim 
    
    -example-
    tokens = {"This", "is", "a" "long", "string"}, tkCount = 5, delim = ' '
    result: "This is a long string" */
int concat_tokens(char *buffer, int size, const char **tokens, int tkCount, const char *delim);

/* count tokens before the delim. all tokens after 
    delimiter are counted as a single token. if 
    multiple delimiters are present, counting stops 
    at the first occurence of the delimiter

    -example-
    string = "This is a *long string*", delim = '*'
    result: 4 */
int count_tokens(const char *string, const char *delim);

/* prepend a char to the string 

  -example-
    string = "This is a long string", ch = ':'
    result: ":This is a long string" */
void prepend_char(char *buffer, int size, const char *string, char ch);

/* delimit messages in a string using a delim. tkCount
    represents a desired number of messages. the remaining
    chars, if present, are ignored. returns the number 
    of delimited messages. this function modifies the 
    original string. 
    
    -example-
    string: "message1\r\nmessage2\r\n", tkCount = 2, delim = '\r\n'
    result: "message1", "message2" */
int delimit_messages(char *string, const char **tokens, int tkCount, const char *delim);


/* extract a single message from string using delimiter 
    and store it in buffer */
int extract_message(char *buffer, int size, char *string, const char *delim); 

/* process messages in a string in the buffer using
    iterator. any remaining string (non-delimited) 
    is left in the buffer */
void process_messages(char *string, const char* delim, StringListFunc iteratorFunc, void *arg);

/* terminate the string */
void terminate_string(char *buffer, int size, const char *string, const char *term);

/* clear the terminator */
void clear_terminator(char *string, const char *term);

/* check if the string is terminated */
int is_terminated(const char *string, const char *term);

/* find the first occurence of a delimiter in a string */
char * find_delimiter(const char *string, const char *delim);

/* count delimiters in a string */
int count_delimiters(const char *string, const char *delim);

/* escape CRLF sequence with '\' chars */
void escape_crlf_sequence(char *buffer, int size, const char *string);

/* count format specifiers in the format string

    -example-
    string = "This is %s long %s"
    result: 2 */
int count_format_specifiers(const char *string);

/* check whether the user's name or channel's name
    contain allowed chars based on the IRC standard */
int is_valid_name(const char *name, int isChannel);

/* iterate over the list of strings and process each
    string with a callback */
void iterate_string_list(const char **stringList, int size, StringListFunc iteratorFunc, void *arg);

/* string callback which saves string length */
void add_string_length(const char *string, void *arg);

/* copy a string from the source to the destination
    with a maximum size */
int safe_copy(char *buffer, int size, const char *string);

/* convert a number from a string representation to 
    an unsigned int */
int str_to_uint(const char *string);

/* convert a number from an unsigned int to a string
    representation */
int uint_to_str(char *buffer, int size, unsigned number);

#endif