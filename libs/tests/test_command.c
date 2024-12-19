#include "../src/priv_command.h"
#include "../../libs/src/response_code.h"

#include <check.h>

START_TEST(test_create_command_tokens) {

    CommandTokens *cmdTokens = create_command_tokens(1);

    ck_assert_ptr_ne(cmdTokens, NULL);

    delete_command_tokens(cmdTokens);
}
END_TEST


START_TEST(test_string_to_command_type) {

    ck_assert_int_eq(string_to_command_type("/help"), HELP);
    ck_assert_int_eq(string_to_command_type("connect"), CONNECT);
}
END_TEST

START_TEST(test_has_command_prefix) {

    ck_assert_int_eq(has_command_prefix("/help"), 1);
    ck_assert_int_eq(has_command_prefix("help connect"), 0);

}
END_TEST

START_TEST(test_get_command_data) {

    const CommandInfo **commandInfos = get_cmd_infos();
    const CommandInfo *command = get_cmd_info(HELP);

    ck_assert_ptr_ne(commandInfos, NULL);
    ck_assert_ptr_ne(command, NULL);
    ck_assert_str_eq(command->label, "help");

}
END_TEST

Suite* command_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Command");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_command_tokens);
    tcase_add_test(tc_core, test_string_to_command_type);
    tcase_add_test(tc_core, test_has_command_prefix);
    tcase_add_test(tc_core, test_get_command_data);

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
