#include "../src/network_utils.h"
#include "../src/string_utils.h"

#include <check.h>

START_TEST(test_convert_hostname_to_ip_address) {

    char ipv4Address[INET_ADDRSTRLEN] = {'\0'};

    int status = convert_hostname_to_ip_address(ipv4Address, sizeof(ipv4Address), "localhost");
    ck_assert_int_eq(status, 1);
    ck_assert_str_eq(ipv4Address, "127.0.0.1");

    status = convert_hostname_to_ip_address(ipv4Address, sizeof(ipv4Address), "127.0.0.1");
    ck_assert_int_eq(status, 1);
    
    status = convert_hostname_to_ip_address(ipv4Address, sizeof(ipv4Address), "local");
    ck_assert_int_eq(status, 0);

}
END_TEST

START_TEST(test_convert_ip_to_hostname) {

    char hostname[MAX_CHARS + 1] = {'\0'};

    int status = convert_ip_to_hostname(hostname, sizeof(hostname), "127.0.0.1");
    ck_assert_int_eq(status, 1);
    ck_assert_str_eq(hostname, "localhost");

    memset(hostname, '\0', MAX_CHARS + 1);

    status = convert_ip_to_hostname(hostname, sizeof(hostname), "127");
    ck_assert_int_eq(status, 0);
    ck_assert_str_eq(hostname, "");

    status = convert_ip_to_hostname(hostname, sizeof(hostname), "localhost");
    ck_assert_int_eq(status, 0);
    ck_assert_str_eq(hostname, "");

}
END_TEST


START_TEST(test_get_local_ip_address) {

    char ipv4Address[INET_ADDRSTRLEN] = {'\0'};

    int fd = socket (AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sa;
    int len = sizeof (sa);

    memset (&sa, 0, len);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(50100); 

    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

    bind(fd, (struct sockaddr *) &sa, sizeof(sa));

    get_local_ip_address(ipv4Address, INET_ADDRSTRLEN, fd);
    ck_assert_str_eq(ipv4Address, "127.0.0.1");

}
END_TEST

START_TEST(test_is_valid_port) {

    ck_assert_int_eq(is_valid_port("50100"), 1);
    ck_assert_int_eq(is_valid_port("65536"), 0);
    ck_assert_int_eq(is_valid_port("123"), 0);
    ck_assert_int_eq(is_valid_port("abc"), 0);
}
END_TEST

START_TEST(test_is_valid_ip_address) {

    ck_assert_int_eq(is_valid_ip_address("127.0.0.1"), 1);
    ck_assert_int_eq(is_valid_ip_address("127"), 0);
    ck_assert_int_eq(is_valid_ip_address("127.0.0.1."), 0);
    ck_assert_int_eq(is_valid_ip_address("256.0.0.1"), 0);
}
END_TEST

Suite* network_utils_suite(void) {
    Suite *s;
    TCase *tc_core, *tc_convert;

    s = suite_create("Network utils");
    tc_core = tcase_create("Core");
    tc_convert = tcase_create("Convert");

    // Add the test case to the test suite
    tcase_add_test(tc_convert, test_convert_hostname_to_ip_address);
    tcase_add_test(tc_convert, test_convert_ip_to_hostname);
    tcase_add_test(tc_convert, test_get_local_ip_address);
    tcase_add_test(tc_core, test_is_valid_port);
    tcase_add_test(tc_core, test_is_valid_ip_address);

    suite_add_tcase(s, tc_convert);
    suite_add_tcase(s, tc_core);

    tcase_set_timeout(tc_convert, 15);

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
