#include "../src/parser.h"

#include <check.h>
#include <stdlib.h>


START_TEST(test_split_input_string) {

    char *cmdTokens[3] = {NULL};  
    char input1[] = "/msg john what is going on";

    int tkCount = split_input_string(input1, cmdTokens, 3);

    ck_assert_ptr_ne(cmdTokens, NULL);
    ck_assert_int_eq(tkCount, 3);
    ck_assert_str_eq(cmdTokens[0], "/msg");
    ck_assert_str_eq(cmdTokens[1], "john");
    ck_assert_str_eq(cmdTokens[2], "what is going on");

    char input2[] = "/msg john";

    tkCount = split_input_string(input2, cmdTokens, 2);

    ck_assert_ptr_ne(cmdTokens, NULL);
    ck_assert_int_eq(tkCount, 2);
    ck_assert_str_eq(cmdTokens[0], "/msg");
    ck_assert_str_eq(cmdTokens[1], "john");

}
END_TEST


START_TEST(test_concat_tokens) {

    char *cmdTokens[3] = {NULL};  
    char input[] = "/help connect";

    int tkCount = split_input_string(input, cmdTokens, 3);
    ck_assert_int_eq(tkCount, 2);

    char output[64] = {'\0'};

    tkCount = concat_tokens(output, 64, cmdTokens, tkCount);

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

Suite* parser_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Parser");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_split_input_string);
    tcase_add_test(tc_core, test_concat_tokens);
    tcase_add_test(tc_core, test_str_to_uint);

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
