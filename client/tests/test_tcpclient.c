#include "../src/tcpclient.h"

#include <check.h>

START_TEST(test_create_client) {

    TCPClient *tcpClient = create_client();

    ck_assert_ptr_ne(tcpClient, NULL);
    ck_assert_ptr_ne(client_get_queue(tcpClient), NULL);
    ck_assert_ptr_ne(client_get_buffer(tcpClient), NULL);
    ck_assert_int_eq(client_get_fd(tcpClient), -1);
    ck_assert_int_eq(client_is_connected(tcpClient), 0);

    delete_client(tcpClient);
}
END_TEST

START_TEST(test_get_set_client_values) {

    TCPClient *tcpClient = create_client();

    client_set_fd(tcpClient, 2);
    client_set_connected(tcpClient, 1);
    client_set_inchannel(tcpClient, 1);

    client_set_servername(tcpClient, "server");
    client_set_channelname(tcpClient, "#networking");

    ck_assert_int_eq(client_get_fd(tcpClient), 2);
    ck_assert_int_eq(client_is_connected(tcpClient), 1);
    ck_assert_int_eq(client_is_inchannel(tcpClient), 1);
    ck_assert_str_eq(client_get_servername(tcpClient), "server");
    ck_assert_str_eq(client_get_channelname(tcpClient), "#networking");

    delete_client(tcpClient);
}
END_TEST


Suite* client_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("TCP client");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_client);
    tcase_add_test(tc_core, test_get_set_client_values);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = client_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
