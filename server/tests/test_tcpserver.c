#include "../src/test_tcpserver.h"
#include "../../shared/src/mock.h"

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

START_TEST(test_get_pfds) {

    PollFds *pollFds = create_pfds(MAX_FDS);

    struct pollfd *pfds = get_pfds(pollFds);

    ck_assert_ptr_ne(pfds, NULL);

    delete_pfds(pollFds);
}
END_TEST

START_TEST(test_get_allocated_pfds) {

    PollFds *pfds = create_pfds(MAX_FDS);

    int allocated = get_allocated_pfds(pfds);

    ck_assert_int_eq(allocated, MAX_FDS);

    delete_pfds(pfds);
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

START_TEST(test_create_clients) {

    Client *clients = create_clients(MAX_FDS);

    ck_assert_ptr_ne(clients, NULL);
    ck_assert_int_eq(clients[0].fd, -1);
    ck_assert_str_eq(clients[0].nickname, "");
    ck_assert_str_eq(clients[0].msgBuffer, "");

    delete_clients(clients);
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

START_TEST(test_read_data) {

    char input1[30] = "This is a";
    mockBuffP = input1;
    mockLen = strlen(input1);
    mockFd = 5; 

    int i = 1;

    PollFds *pfds = create_pfds(MAX_FDS);
    
    ck_assert_ptr_ne(pfds, NULL);
    
    set_pfd(pfds, i, mockFd, POLLIN);

    int fullRead = read_data(pfds, i);

    ck_assert_int_eq(fullRead, 0);
    ck_assert_str_eq(input1, pfds->clients[i].msgBuffer);

    char input2[] = " full sentence\n";
    mockBuffP = input2;
    mockLen = strlen(input2);

    fullRead = read_data(pfds, i);

    ck_assert_int_eq(fullRead, 1);
    ck_assert_str_eq(strcat(input1, input2), pfds->clients[i].msgBuffer);

    delete_pfds(pfds);
}
END_TEST


START_TEST(test_write_data) {

    char input[] = "This is a full sentence\n";
    char output[30] = {'\0'};
    mockBuffP = output;
    mockLen = 5;
    mockFd = 5; 

    int i = 1;

    PollFds *pfds = create_pfds(MAX_FDS);
    
    ck_assert_ptr_ne(pfds, NULL);
    
    set_pfd(pfds, i, mockFd, POLLIN);
    strcpy(pfds->clients[i].msgBuffer, input);

    write_data(pfds, i);

    ck_assert_str_eq(input, output);
    ck_assert_int_eq(pfds->clients[i].msgBuffer[0], '\0');

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
    tcase_add_test(tc_core, test_get_pfds);
    tcase_add_test(tc_core, test_get_allocated_pfds);
    tcase_add_test(tc_core, test_set_pfd);
    tcase_add_test(tc_core, test_unset_pfd);
    tcase_add_test(tc_core, test_create_clients);
    tcase_add_test(tc_core, test_find_pfd_index);
    tcase_add_test(tc_core, test_read_data);
    tcase_add_test(tc_core, test_write_data);

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