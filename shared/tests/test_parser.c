 #include "../src/parser.h"

#include <check.h>
#include <stdlib.h>

START_TEST(test_split_input_string) {

    char *tokens[3] = {NULL};  
    char msg1[] = "/msg john what is going on";

    int tkCount = split_input_string(msg1, tokens, 3, ' ');

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 3);
    ck_assert_str_eq(tokens[0], "/msg");
    ck_assert_str_eq(tokens[1], "john");
    ck_assert_str_eq(tokens[2], "what is going on");

    char msg2[] = "/msg john";

    tkCount = split_input_string(msg2, tokens, 2, '\n');

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 1);
    ck_assert_str_eq(tokens[0], "/msg john");

    char msg3[] = "/msg john ";

    tkCount = split_input_string(msg3, tokens, 1, ' ');

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 1);
    ck_assert_str_eq(tokens[0], "/msg john ");

    char msg4[] = "/msg john ";

    tkCount = split_input_string(msg4, tokens, 0, ' ');

    ck_assert_ptr_ne(tokens, NULL);
    ck_assert_int_eq(tkCount, 0);

}
END_TEST

START_TEST(test_concat_tokens) {

    char *tokens[3] = {NULL};  
    char msg[] = "/help connect";

    int tkCount = split_input_string(msg, tokens, 3, ' ');
    ck_assert_int_eq(tkCount, 2);

    char output[64] = {'\0'};

    tkCount = concat_tokens(output, 64, tokens, tkCount, " ");

    ck_assert_int_eq(tkCount, 2);
    ck_assert_str_eq(output, "/help connect");

}
END_TEST

START_TEST(test_str_to_uint) {

    ck_assert_int_eq(str_to_uint("45"), 45);
    ck_assert_int_eq(str_to_uint("-45"), -1);
    ck_assert_int_eq(str_to_uint("abc"), -1);
    ck_assert_int_eq(str_to_uint("45abc"), -1);

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

Suite* parser_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Parser");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_split_input_string);
    tcase_add_test(tc_core, test_concat_tokens);
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

    s = parser_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
