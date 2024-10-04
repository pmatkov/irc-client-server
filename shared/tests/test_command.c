#include "../src/priv_command.h"
#include "../../shared/src/string_utils.h"
#include "../../shared/src/response_code.h"

#include <check.h>

START_TEST(test_create_message) {

    char message[MAX_CHARS + 1] = {'\0'};

    const char *code = get_response_code(ERR_NICKNAMEINUSE);
    create_message(message, MAX_CHARS, &(MessageTokens){{"irc.example.com"}, {code, "john"}, {get_response_message(code)}, 1});

    ck_assert_str_eq(message, ":irc.example.com 433 john :Nickname is already in use");

}
END_TEST

START_TEST(test_command_type_to_string) {

    ck_assert_str_eq(command_type_to_string(HELP), "help");

}
END_TEST

START_TEST(test_string_to_command_type) {

    ck_assert_int_eq(string_to_command_type("/help"), HELP);
    ck_assert_int_eq(string_to_command_type("connect"), CONNECT);
}
END_TEST

START_TEST(test_is_valid_command) {

    ck_assert_int_eq(is_valid_command(HELP), 1);
    ck_assert_int_eq(is_valid_command(10), 0);

}
END_TEST

START_TEST(test_has_command_prefix) {

    ck_assert_int_eq(has_command_prefix("/help"), 1);
    ck_assert_int_eq(has_command_prefix("help connect"), 0);

}
END_TEST

Suite* command_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Command");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_message);
    tcase_add_test(tc_core, test_command_type_to_string);
    tcase_add_test(tc_core, test_string_to_command_type);
    tcase_add_test(tc_core, test_is_valid_command);
    tcase_add_test(tc_core, test_has_command_prefix);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = command_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
