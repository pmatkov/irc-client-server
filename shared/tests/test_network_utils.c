#include "../src/network_utils.h"

#include <check.h>

#define MAX_CHARS 512

START_TEST(test_convert_hostname_to_ip) {

    char ipv4Address[INET_ADDRSTRLEN] = {'\0'};
    convert_hostname_to_ip("localhost", ipv4Address, sizeof(ipv4Address));

    ck_assert_str_eq(ipv4Address, "127.0.0.1");

}
END_TEST

START_TEST(test_convert_ip_to_hostname) {

    char hostname[MAX_CHARS + 1] = {'\0'};

    convert_ip_to_hostname("127.0.0.1", hostname, sizeof(hostname));

    ck_assert_str_eq(hostname, "localhost");

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

Suite* network_utils_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Network utils");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_convert_hostname_to_ip);
    tcase_add_test(tc_core, test_convert_ip_to_hostname);
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

    s = network_utils_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
