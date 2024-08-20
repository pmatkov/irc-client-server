#include "../src/tcpserver.h"

#include <check.h>

#define MAX_FDS 100

START_TEST(test_create_pfds) {

    PollFds *pfds = create_pfds(MAX_FDS);

    ck_assert_ptr_ne(pfds, NULL);
    ck_assert_ptr_ne(pfds->clients, NULL);
    ck_assert_int_eq(pfds->allocatedPfds, MAX_FDS);
    ck_assert_int_eq(pfds->usedPfds, 0);
    ck_assert_int_eq(pfds->pfds[0].fd, -1);

    delete_pfds(pfds);
}
END_TEST


START_TEST(test_create_clients) {

    Client *clients = create_clients(MAX_FDS);

    ck_assert_ptr_ne(clients, NULL);
    ck_assert_int_eq(clients[0].fd, -1);
    ck_assert_str_eq(clients[0].nickname, "");
    ck_assert_str_eq(clients[0].msgBuffer, "");

    delete_clients(clients);
}
END_TEST

START_TEST(test_set_pfd) {

    PollFds *pfds = create_pfds(MAX_FDS);

    ck_assert_ptr_ne(pfds, NULL);
    
    set_pfd(pfds, 0, 5, POLLIN);

    ck_assert_int_eq(pfds->pfds[0].fd, 5);
    ck_assert_int_eq(pfds->pfds[0].events, POLLIN);
    ck_assert_int_eq(pfds->clients[0].fd, 5);
    ck_assert_int_eq(pfds->usedPfds, 1);

    delete_pfds(pfds);
}
END_TEST

START_TEST(test_unset_pfd) {

    PollFds *pfds = create_pfds(MAX_FDS);

    ck_assert_ptr_ne(pfds, NULL);
    
    set_pfd(pfds, 0, 5, POLLIN);

    ck_assert_int_eq(pfds->usedPfds, 1);

    unset_pfd(pfds, 0);
    ck_assert_int_eq(pfds->usedPfds, 0);

    delete_pfds(pfds);
}
END_TEST

START_TEST(test_find_pfd_index) {

    PollFds *pfds = create_pfds(MAX_FDS);
    
    ck_assert_ptr_ne(pfds, NULL);

    int found = find_pfd_index(pfds, 1);
    ck_assert_int_eq(found, -1);

    set_pfd(pfds, 0, 5, POLLIN);

    found = find_pfd_index(pfds, 5);
    ck_assert_int_eq(found, 0);

    delete_pfds(pfds);
}
END_TEST


Suite* tcpserver_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("TCPServer");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_pfds);
    tcase_add_test(tc_core, test_create_clients);
    tcase_add_test(tc_core, test_set_pfd);
    tcase_add_test(tc_core, test_unset_pfd);
    tcase_add_test(tc_core, test_find_pfd_index);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = tcpserver_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif