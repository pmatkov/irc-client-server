#include "../src/command_parser.h"
#include "../src/input_handler.h"
#include "../src/display.h"

#include <check.h>
#include <stdlib.h>

START_TEST(test_create_ib) {

    LineEditor *lnEditor = create_line_editor(NULL);

    ck_assert_ptr_ne(lnEditor, NULL);
    ck_assert_int_eq(lnEditor->cursor, PROMPT_SIZE);

    ck_assert_int_eq(lnEditor->charCount, 0);

    delete_line_editor(lnEditor);
}
END_TEST

START_TEST(test_split_input_string) {

    char *cmdTokens[3] = {NULL};  
    char input[] = "/help connect";

    split_input_string(cmdTokens, input);

    ck_assert_ptr_ne(cmdTokens, NULL);
    ck_assert_str_eq(cmdTokens[0], "/help");
    ck_assert_str_eq(cmdTokens[1], "connect");

}
END_TEST

START_TEST(test_concat_tokens) {

    char *cmdTokens[3] = {NULL};  
    char input[] = "/help connect";

    int tokenCount = split_input_string(cmdTokens, input);

    char buffer[255] = {'\0'};

    concat_tokens(buffer, 255, cmdTokens, tokenCount);

    ck_assert_str_eq(buffer, "/help connect");

}
END_TEST

START_TEST(test_has_command_prefix) {

    ck_assert_int_eq(has_command_prefix("/help"), 1);
    ck_assert_int_eq(has_command_prefix("help connect"), 0);

}
END_TEST

START_TEST(test_convert_token_to_cmd) {

    ck_assert_int_eq(convert_token_to_cmd("/help", 1), HELP);
    ck_assert_int_eq(convert_token_to_cmd("connect", 0), CONNECT);
}
END_TEST

Suite* parser_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Command parser");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_ib);
    tcase_add_test(tc_core, test_split_input_string);
    tcase_add_test(tc_core, test_concat_tokens);
    tcase_add_test(tc_core, test_has_command_prefix);
    tcase_add_test(tc_core, test_convert_token_to_cmd);

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
