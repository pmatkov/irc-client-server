#include "../src/tcpclient.h"

#include <check.h>

START_TEST(test_str_to_int) {

    ck_assert_int_eq(str_to_int("45"), 45);
    ck_assert_int_eq(str_to_int("-45"), -1);
    ck_assert_int_eq(str_to_int("abc"), -1);
    ck_assert_int_eq(str_to_int("45abc"), -1);

}
END_TEST

START_TEST(test_is_port) {

    ck_assert_int_eq(is_port("50100"), 1);
    ck_assert_int_eq(is_port("65536"), 0);
    ck_assert_int_eq(is_port("123"), 0);
    ck_assert_int_eq(is_port("abc"), 0);
}
END_TEST

START_TEST(test_is_ipv4address) {

    ck_assert_int_eq(is_ipv4address("127.0.0.1"), 1);
    ck_assert_int_eq(is_ipv4address("127"), 0);
    ck_assert_int_eq(is_ipv4address("127.0.0.1."), 0);
    ck_assert_int_eq(is_ipv4address("256.0.0.1"), 0);
}
END_TEST

Suite* tcpclient_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("TCP client");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_str_to_int);
    tcase_add_test(tc_core, test_is_port);
    tcase_add_test(tc_core, test_is_ipv4address);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = tcpclient_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
