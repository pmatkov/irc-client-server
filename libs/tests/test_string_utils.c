#include "../src/string_utils.h"

#include <check.h>

#define LIST_CAPACITY 10
#define STRING_LENGTH 20

START_TEST(test_create_string_list) {

    StringList *stringList = create_string_list(LIST_CAPACITY, STRING_LENGTH);

    ck_assert_ptr_ne(stringList, NULL);
    ck_assert_int_eq(stringList->stringLength, STRING_LENGTH);
    ck_assert_int_eq(stringList->capacity, LIST_CAPACITY);
    ck_assert_int_eq(stringList->count, 0);

    delete_string_list(stringList);
}
END_TEST

START_TEST(test_is_string_list_empty) {

    StringList *stringList = create_string_list(LIST_CAPACITY, STRING_LENGTH);

    ck_assert_int_eq(is_string_list_empty(stringList), 1);

    delete_string_list(stringList);
}
END_TEST

START_TEST(test_is_string_list_full) {

    StringList *stringList = create_string_list(LIST_CAPACITY, STRING_LENGTH);

    ck_assert_int_eq(is_string_list_full(stringList), 0);

    delete_string_list(stringList);
}
END_TEST

START_TEST(test_add_remove_string_from_string_list) {

    StringList *stringList = create_string_list(LIST_CAPACITY, STRING_LENGTH);

    add_string_to_string_list(stringList, "john");

    ck_assert_int_eq(stringList->count, 1);
    ck_assert_str_eq(stringList->strings[0], "john");

    remove_string_from_string_list(stringList);
    ck_assert_int_eq(stringList->count, 0);
    ck_assert_str_eq(stringList->strings[0], "");

    delete_string_list(stringList);
}
END_TEST

START_TEST(test_split_input_string) {

    const char *tokens[3] = {NULL};  
    char msg1[] = "/msg john what is going on";

    int tkCount = split_input_string(msg1, tokens, 3, " ");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 3);
    ck_assert_str_eq(tokens[0], "/msg");
    ck_assert_str_eq(tokens[1], "john");
    ck_assert_str_eq(tokens[2], "what is going on");

    char msg2[] = "/msg john";

    tkCount = split_input_string(msg2, tokens, 2, " ");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 2);
    ck_assert_str_eq(tokens[0], "/msg");
    ck_assert_str_eq(tokens[1], "john");

    char msg3[] = "/msg john";

    tkCount = split_input_string(msg3, tokens, 2, "\n");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 1);
    ck_assert_str_eq(tokens[0], "/msg john");

    char msg4[] = "/msg john ";

    tkCount = split_input_string(msg4, tokens, 1, " ");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 1);
    ck_assert_str_eq(tokens[0], "/msg john");

    char msg5[] = "/msg john ";

    tkCount = split_input_string(msg5, tokens, 0, " ");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 0);

    char msg6[] = "/msg john ";

    tkCount = split_input_string(msg6, tokens, 2, " ");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 2);
    ck_assert_str_eq(tokens[0], "/msg");
    ck_assert_str_eq(tokens[1], "john");

    char msg7[] = "PRIVMSG #cmsc23300 :Hello everybody";

    tkCount = split_input_string(msg7, tokens, 3, ":");

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 2);
    ck_assert_str_eq(tokens[0], "PRIVMSG #cmsc23300 ");
    ck_assert_str_eq(tokens[1], "Hello everybody");

    char msg8[] = "";
    tkCount = split_input_string(msg8, tokens, 5, " ");

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

    memset(buffer, '\0', sizeof(buffer));

    tkCount = concat_tokens(buffer, 10, tokens, 2, " ");

    ck_assert_str_eq(buffer, "/help con");

    memset(buffer, '\0', sizeof(buffer));

    const char *msgTokens[] = {"PRIVMSG", "#general", NULL};

    concat_tokens(buffer, MAX_CHARS + 1, msgTokens, 3, " ");

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

START_TEST(test_crlf_terminate) {

    char buffer[MAX_CHARS + 1] = {'\0'};

    crlf_terminate(buffer, MAX_CHARS + 1, "message");

    ck_assert_str_eq(buffer, "message\r\n");

}
END_TEST

START_TEST(test_is_crlf_terminated) {

    ck_assert_int_eq(is_crlf_terminated("message\r\n"), 1);
    ck_assert_int_eq(is_crlf_terminated("message\n"), 0);
    ck_assert_int_eq(is_crlf_terminated("message"), 0);

}
END_TEST

START_TEST(test_is_valid_name) {

    ck_assert_int_eq(is_valid_name("john", 0), 1);
    ck_assert_int_eq(is_valid_name("john doe", 0), 0);
    ck_assert_int_eq(is_valid_name("(john)", 0), 0);
    ck_assert_int_eq(is_valid_name("#general", 1), 1);
    ck_assert_int_eq(is_valid_name("general", 1), 0);

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
    
    status = uint_to_str(buffer, 2, 10);

    ck_assert_int_eq(status, 0);
    ck_assert_str_eq(buffer, "100");

    status = uint_to_str(buffer, 0, 100);
    ck_assert_int_eq(status, 0);

    char *nullPtr = NULL;

    status = uint_to_str(nullPtr, 0, 100);
    ck_assert_int_eq(status, 0);

}
END_TEST

Suite* string_utils_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("String utils");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_string_list);
    tcase_add_test(tc_core, test_is_string_list_empty);
    tcase_add_test(tc_core, test_is_string_list_full);
    tcase_add_test(tc_core, test_add_remove_string_from_string_list);
    tcase_add_test(tc_core, test_split_input_string);
    tcase_add_test(tc_core, test_concat_tokens);
    tcase_add_test(tc_core, test_count_tokens);
    tcase_add_test(tc_core, test_prepend_char);
    tcase_add_test(tc_core, test_crlf_terminate);
    tcase_add_test(tc_core, test_is_crlf_terminated);
    tcase_add_test(tc_core, test_is_valid_name);
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
