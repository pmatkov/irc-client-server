#include "../src/error_control.h"

#include <check.h>
#include <stdlib.h>

START_TEST(test_get_error_code_string) {

    ck_assert_str_eq(get_error_code_string(NO_ERRCODE), "No error");
}
END_TEST

START_TEST(test_failed1) {

    failed("Error test [1]", NO_ERRCODE, __func__, __FILE__, __LINE__);

}
END_TEST

START_TEST(test_failed2) {

    failed("Error test [%d]", NO_ERRCODE, __func__, __FILE__, __LINE__, 2);
    failed(NULL, ARG_ERROR, __func__, __FILE__, __LINE__);
}
END_TEST

START_TEST(test_failed3) {

    failed(NULL, ARG_ERROR, __func__, __FILE__, __LINE__);
}
END_TEST

Suite* error_control_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Error control");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_get_error_code_string);
    tcase_add_exit_test(tc_core, test_failed1, 1);
    tcase_add_exit_test(tc_core, test_failed2, 1);
    tcase_add_exit_test(tc_core, test_failed3, 1);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = error_control_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
