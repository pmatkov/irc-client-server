#include "../src/command_processor.h"
#include "../src/display.h"

#include <check.h>
#include <stdlib.h>

START_TEST(test_has_command_prefix) {

    ck_assert_int_eq(has_command_prefix("/help"), 1);
    ck_assert_int_eq(has_command_prefix("help connect"), 0);

}
END_TEST

START_TEST(test_string_to_command_type) {

    ck_assert_int_eq(string_to_command_type("/help"), HELP);
    ck_assert_int_eq(string_to_command_type("connect"), CONNECT);
}
END_TEST


Suite* command_processor_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Command processor");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_has_command_prefix);
    tcase_add_test(tc_core, test_string_to_command_type);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = command_processor_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
