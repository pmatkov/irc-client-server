#include "../src/string_utils.h"
#include "../src/common.h"
#include "../src/error_control.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define DEF_CAPACITY 10

typedef struct {
    char **strings;
    int count;
    int capacity;
} StringList;

static StringList * create_string_list(int capacity) {

    StringList *stringList = (StringList*) malloc(sizeof(StringList));
    if (stringList == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    if (capacity > DEF_CAPACITY) {
        capacity = DEF_CAPACITY;
    }

    stringList->strings = (char**) malloc(capacity * sizeof(char*));

    if (stringList->strings == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    for (int i = 0; i < capacity; i++) {

        stringList->strings[i] = (char*) malloc((MAX_CHARS + 1) * sizeof(char));
        if (stringList->strings[i] == NULL) {
            FAILED(ALLOC_ERROR, NULL);
        }
    }

    stringList->capacity = capacity;
    stringList->count = 0;

    return stringList;
}

static void delete_string_list(StringList *stringList) {

    if (stringList != NULL) {
           
        for (int i = 0; i < stringList->capacity; i++) {

            free(stringList->strings[i]);
        }
        free(stringList->strings);
    }

    free(stringList);
}

static void upper(const char *string, void *arg) {

    StringList *stringList = arg;

    if (string != NULL && stringList != NULL && stringList->count < stringList->capacity) {

        char buffer[MAX_CHARS + 1] = {'\0'};
        str_to_upper(buffer, ARRAY_SIZE(buffer), string);

        safe_copy(stringList->strings[stringList->count], MAX_CHARS + 1, buffer);
        stringList->count++;
    }
}

static void print_upper(const char *string, void *arg) {

    if (string != NULL) {

        while (*string) {
            putchar(toupper(*string++));
        }
        printf("\n");
    }
}

START_TEST(test_tokenize_string) {

    const char *tokens[5] = {NULL};  
    char msg1[] = "/msg john what is going on";

    int tkCount = tokenize_string(msg1, tokens, 3, " ");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 3);
    ck_assert_str_eq(tokens[0], "/msg");
    ck_assert_str_eq(tokens[1], "john");
    ck_assert_str_eq(tokens[2], "what is going on");

    char msg2[] = "/msg john";

    tkCount = tokenize_string(msg2, tokens, 2, " ");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 2);
    ck_assert_str_eq(tokens[0], "/msg");
    ck_assert_str_eq(tokens[1], "john");

    char msg3[] = "/msg john";

    tkCount = tokenize_string(msg3, tokens, 2, "\n");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 1);
    ck_assert_str_eq(tokens[0], "/msg john");

    char msg4[] = "/msg john ";

    tkCount = tokenize_string(msg4, tokens, 1, " ");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 1);
    ck_assert_str_eq(tokens[0], "/msg john");

    char msg5[] = "/msg john ";

    tkCount = tokenize_string(msg5, tokens, 0, " ");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 0);

    char msg6[] = "/msg john ";

    tkCount = tokenize_string(msg6, tokens, 2, " ");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 2);
    ck_assert_str_eq(tokens[0], "/msg");
    ck_assert_str_eq(tokens[1], "john");

    char msg7[] = "PRIVMSG #cmsc23300 :Hello everybody";

    tkCount = tokenize_string(msg7, tokens, 3, ":");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 2);
    ck_assert_str_eq(tokens[0], "PRIVMSG #cmsc23300 ");
    ck_assert_str_eq(tokens[1], "Hello everybody");

    char msg8[] = "";
    tkCount = tokenize_string(msg8, tokens, 5, " ");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 0);

}
END_TEST

START_TEST(test_concat_tokens) {

    const char *tokens[2] = {"/help", "connect"};  

    char buffer[MAX_CHARS + 1] = {'\0'};

    int tkCount = concat_tokens(buffer, MAX_CHARS, tokens, 2, " ");

    ck_assert_int_eq(tkCount, 2);
    ck_assert_str_eq(buffer, "/help connect");

    memset(buffer, '\0', ARRAY_SIZE(buffer));

    tkCount = concat_tokens(buffer, 10, tokens, 2, " ");

    ck_assert_str_eq(buffer, "/help con");

    memset(buffer, '\0', ARRAY_SIZE(buffer));

    const char *msgTokens[] = {"PRIVMSG", "#general", NULL};

    concat_tokens(buffer, MAX_CHARS, msgTokens, 3, " ");

    ck_assert_str_eq(buffer, "PRIVMSG #general");

}
END_TEST

START_TEST(test_count_tokens) {

    char msg1[] = "PRIVMSG #cmsc23300 :Hello everybody";  

    int count = count_tokens(msg1, ":");

    ck_assert_int_eq(count, 3);

    count = count_tokens(msg1, "\0");

    ck_assert_int_eq(count, 4);

    char msg2[] = "PRIVMSG #cmsc23300";  

    count = count_tokens(msg2, ":");

    ck_assert_int_eq(count, 2);

}
END_TEST

START_TEST(test_prepend_char) {

    char buffer[MAX_CHARS + 1] = "test";  

    prepend_char(buffer, MAX_CHARS, buffer, ':');

    ck_assert_str_eq(buffer, ":test");

}
END_TEST

START_TEST(test_delimit_messages) {

    char string1[] = "message1\r\nmessage2\r\n";
    const char *tokens[2] = {NULL};  

    int count = delimit_messages(string1, tokens, 2, CRLF);

    ck_assert_int_eq(count, 2);
    ck_assert_str_eq(tokens[0], "message1");
    ck_assert_str_eq(tokens[1], "message2");

    char string2[] = "message1\r\nmessage2";

    count = delimit_messages(string2, tokens, 1, CRLF);

    ck_assert_int_eq(count, 1);
    ck_assert_str_eq(tokens[0], "message1");

}
END_TEST

START_TEST(test_extract_message) {

    char buffer[MAX_CHARS + 1] = {'\0'};
    char string[] = "message1\r\nmessage2\r\nmes";
    char *messages[] = {"message1", "message2", "mes"};

    int i = 0;
    while (extract_message(buffer, ARRAY_SIZE(buffer), string, CRLF)) {
        ck_assert_str_eq(buffer, messages[i]);
        i++;
    }
    ck_assert_int_eq(i, 2);
    ck_assert_str_eq(string, messages[i]);
}
END_TEST

START_TEST(test_process_messages) {

    char string[] = "message1\r\nmessage2\r\n";

    process_messages(string, CRLF, print_upper, NULL);

}
END_TEST

START_TEST(test_terminate_string) {

    char buffer[MAX_CHARS + 1] = {'\0'};

    terminate_string(buffer, MAX_CHARS + 1, "message", CRLF);
    ck_assert_str_eq(buffer, "message\r\n");
    
    memset(buffer, '\0', ARRAY_SIZE(buffer));
    terminate_string(buffer, MAX_CHARS + 1, ":irc.server.com 001 tom :Welcome to the IRC Network", CRLF);
    ck_assert_str_eq(buffer, ":irc.server.com 001 tom :Welcome to the IRC Network\r\n");

}
END_TEST

START_TEST(test_clear_terminator) {

    char string[] = "message\r\n";

    clear_terminator(string, CRLF);

    ck_assert_str_eq(string, "message");

}
END_TEST

START_TEST(test_is_terminated) {

    ck_assert_int_eq(is_terminated("message\r\n", CRLF), 1);
    ck_assert_int_eq(is_terminated("message\n", CRLF), 0);
    ck_assert_int_eq(is_terminated("message ", " "), 1);

    char *string = "message one ";
    ck_assert_int_eq(is_terminated(&string[strlen("message") + 1], " "), 1);

}
END_TEST

START_TEST(test_find_delimiter) {

    char *string = "message1\r\nmessage2\r\n";
    ck_assert_ptr_eq(find_delimiter(string, CRLF), &string[strlen("message1")]);

}
END_TEST

START_TEST(test_count_delimiters) {

    char *string = "message1\r\nmessage2\r\n";
    ck_assert_int_eq(count_delimiters(string, CRLF), 2);

}
END_TEST

START_TEST(test_escape_crlf_sequence) {

    char string[] = "message1\r\nmessage2\r\n";
    char escapedMsg[MAX_CHARS + 2 * sizeof(CRLF) + 1] = {'\0'};

    escape_crlf_sequence(escapedMsg, sizeof(escapedMsg), string);

    ck_assert_str_eq(escapedMsg, "message1\\r\\nmessage2\\r\\n");

}
END_TEST

START_TEST(test_count_format_specifiers) {

    int fsCount = count_format_specifiers("This is a %s long %s");

    ck_assert_int_eq(fsCount, 2);

}
END_TEST

START_TEST(test_is_valid_name) {

    const char *validChars = "$#\\[]{}";

    ck_assert_int_eq(is_valid_name("john", validChars), 1);
    ck_assert_int_eq(is_valid_name("john$", validChars), 1);
    ck_assert_int_eq(is_valid_name("john doe", validChars), 0);
    ck_assert_int_eq(is_valid_name("(john)", validChars), 0);

}
END_TEST

START_TEST(test_str_to_upper_lower) {

    char buffer[MAX_CHARS + 1] = {'\0'};

    str_to_upper(buffer, ARRAY_SIZE(buffer), "mark");
    ck_assert_str_eq(buffer, "MARK");
    str_to_lower(buffer, ARRAY_SIZE(buffer), "JOHN");
    ck_assert_str_eq(buffer, "john");

}
END_TEST

START_TEST(test_strn_to_upper_lower) {

    char buffer[MAX_CHARS + 1] = {'\0'};

    strn_to_upper(buffer, ARRAY_SIZE(buffer), "markus", 4);
    ck_assert_str_eq(buffer, "MARK");
    strn_to_lower(buffer, ARRAY_SIZE(buffer), "JOHNATHAN", 4);
    ck_assert_str_eq(buffer, "john");

}
END_TEST

START_TEST(test_shift_chars) {

    char buffer1[MAX_CHARS + 1] = "message";

    shift_chars(buffer1, ARRAY_SIZE(buffer1), 2, 5);
    ck_assert_str_eq(buffer1, "mege");

    char buffer2[MAX_CHARS + 1] = "message";

    shift_chars(buffer2, ARRAY_SIZE(buffer2), 5, 6);
    ck_assert_str_eq(buffer2, "messae");

}
END_TEST

START_TEST(test_iterate_string_list) {

    const char *strings[] = {"apple", "orange", "banana"};

    StringList *stringList = create_string_list(ARRAY_SIZE(strings));

    iterate_string_list(strings, ARRAY_SIZE(strings), upper, stringList);

    ck_assert_str_eq(stringList->strings[0], "APPLE");
    ck_assert_str_eq(stringList->strings[1], "ORANGE");

    delete_string_list(stringList);

}
END_TEST

START_TEST(test_safe_copy) {

    char buffer[10 + 1] = {'\0'}; 

    int copied = safe_copy(buffer, 10 + 1, "/help");

    ck_assert_int_eq(copied, 1);
    ck_assert_str_eq(buffer, "/help");

    copied = safe_copy(buffer, 10 + 1, "john");

    ck_assert_int_eq(copied, 1);
    ck_assert_str_eq(buffer, "john");

    copied = safe_copy(buffer, 10 + 1, "/help connect");

    ck_assert_int_eq(copied, 0);
    ck_assert_str_ne(buffer, "/help connect");

}
END_TEST

START_TEST(test_str_to_uint) {

    ck_assert_int_eq(str_to_uint("45"), 45);
    ck_assert_int_eq(str_to_uint("-45"), -1);
    ck_assert_int_eq(str_to_uint("abc"), -1);
    ck_assert_int_eq(str_to_uint("45abc"), -1);
    ck_assert_int_eq(str_to_uint("001"), 1);

}
END_TEST

START_TEST(test_uint_to_str) {

    char buffer[5] = {'\0'};

    int status = uint_to_str(buffer, 3, 10);

    ck_assert_int_eq(status, 1);
    ck_assert_str_eq(buffer, "10");

    status = uint_to_str(buffer, 5, 100);
    ck_assert_int_eq(status, 1);
    ck_assert_str_eq(buffer, "100");
    
    status = uint_to_str(buffer, 1, 10);

    ck_assert_int_eq(status, 0);
    ck_assert_str_eq(buffer, "100");

    status = uint_to_str(buffer, 0, 100);
    ck_assert_int_eq(status, 0);

}
END_TEST

Suite* string_utils_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("String utils");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_tokenize_string);
    tcase_add_test(tc_core, test_concat_tokens);
    tcase_add_test(tc_core, test_count_tokens);
    tcase_add_test(tc_core, test_prepend_char);
    tcase_add_test(tc_core, test_delimit_messages);
    tcase_add_test(tc_core, test_extract_message);
    tcase_add_test(tc_core, test_process_messages);
    tcase_add_test(tc_core, test_terminate_string);
    tcase_add_test(tc_core, test_clear_terminator);
    tcase_add_test(tc_core, test_is_terminated);
    tcase_add_test(tc_core, test_find_delimiter);
    tcase_add_test(tc_core, test_count_delimiters);
    tcase_add_test(tc_core, test_escape_crlf_sequence);
    tcase_add_test(tc_core, test_count_format_specifiers);
    tcase_add_test(tc_core, test_is_valid_name);
    tcase_add_test(tc_core, test_str_to_upper_lower);
    tcase_add_test(tc_core, test_strn_to_upper_lower);
    tcase_add_test(tc_core, test_shift_chars);
    tcase_add_test(tc_core, test_iterate_string_list);
    tcase_add_test(tc_core, test_safe_copy);
    tcase_add_test(tc_core, test_str_to_uint);
    tcase_add_test(tc_core, test_uint_to_str);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = string_utils_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
