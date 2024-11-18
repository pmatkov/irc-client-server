#include "../src/priv_client.h"
#include "../../libs/src/string_utils.h"

#include <check.h>

START_TEST(test_create_client) {

    Client *client = create_client();

    ck_assert_ptr_ne(client, NULL);
    ck_assert_ptr_eq(client->fd, NULL);
    ck_assert_str_eq(client->nickname, "");
    ck_assert_str_eq(client->inBuffer, "");
    ck_assert_str_eq(client->ipv4Address, "");
    ck_assert_int_eq(client->port, 0);
    ck_assert_int_eq(client->registered, 0);
    ck_assert_ptr_ne(client->timer, NULL);

    delete_client(client);
}
END_TEST

START_TEST(test_get_set_client_data) {

    Client *client = create_client();

    set_client_fd(client, &(int){3});
    ck_assert_int_eq(*get_client_fd(client), 3);
    set_client_nickname(client, "john");
    ck_assert_str_eq(get_client_nickname(client), "john");

    delete_client(client);
}
END_TEST

Suite* client_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Client");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_client);
    tcase_add_test(tc_core, test_get_set_client_data);
    
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