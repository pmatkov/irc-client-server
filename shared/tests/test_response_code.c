#include "../src/response_code.h"

#include <check.h>

START_TEST(test_get_response_type) {

    ResponseType responseType = get_response_type(403);

    ck_assert_int_eq(responseType, ERR_NOSUCHCHANNEL);
}
END_TEST

START_TEST(test_get_response_message) {

    const char *message = get_response_message(403);

    ck_assert_ptr_ne(message, NULL);
    ck_assert_str_eq(message, "No such channel");
}
END_TEST

START_TEST(test_get_response_code) {

    int code = get_response_code(ERR_NOSUCHCHANNEL);

    ck_assert_int_eq(code, 403);
}
END_TEST

Suite* response_code_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Response code");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_get_response_type);
    tcase_add_test(tc_core, test_get_response_message);
    tcase_add_test(tc_core, test_get_response_code);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = response_code_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
