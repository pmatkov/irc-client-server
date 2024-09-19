#include "../src/priv_tcpserver.h"
#include "../../shared/src/priv_message.h"
#include "../../shared/src/mock.h"

#include <check.h>

#define MAX_FDS 1024

START_TEST(test_create_pollfd_set) {

    PollFdSet *pollFdSet = create_pollfd_set();

    ck_assert_ptr_ne(pollFdSet, NULL);
    ck_assert_int_eq(pollFdSet->capacity, MAX_FDS);
    ck_assert_int_eq(pollFdSet->count, 0);

    delete_pollfd_set(pollFdSet);
}
END_TEST

START_TEST(test_is_pfd_set_empty) {

    PollFdSet *pollFdSet = create_pollfd_set();

    ck_assert_int_eq(is_pfd_set_empty(pollFdSet), 1);

    delete_pollfd_set(pollFdSet);
}
END_TEST

START_TEST(test_is_pfd_set_full) {

    PollFdSet *pollFdSet = create_pollfd_set();

    ck_assert_int_eq(is_pfd_set_full(pollFdSet), 0);

    delete_pollfd_set(pollFdSet);
}
END_TEST

START_TEST(test_create_server) {

    TCPServer *server = create_server("irc.example.com");

    ck_assert_ptr_ne(server, NULL);
    ck_assert_ptr_ne(server->clients, NULL);
    ck_assert_ptr_ne(server->session, NULL);
    ck_assert_ptr_ne(server->msgQueue, NULL);
    ck_assert_str_eq(server->serverName, "irc.example.com");

    delete_server(server);
}
END_TEST

START_TEST(test_set_pfd) {

    PollFdSet *pollFdSet = create_pollfd_set();
    TCPServer *server = create_server("irc.example.com");
    
    set_pfd(pollFdSet, server, 0, 1);

    ck_assert_int_eq(pollFdSet->count, 1);
    ck_assert_int_eq(pollFdSet->pfds[0].fd, 1);
    ck_assert_int_eq(pollFdSet->pfds[0].events, POLLIN);
    ck_assert_int_eq(server->clients[0].fd, 1);

    delete_pollfd_set(pollFdSet);
    delete_server(server);
}
END_TEST

START_TEST(test_unset_pfd) {

    PollFdSet *pollFdSet = create_pollfd_set();
    TCPServer *server = create_server("irc.example.com");
    
    set_pfd(pollFdSet, server, 0, 1);
    ck_assert_int_eq(pollFdSet->count, 1);
    ck_assert_int_eq(pollFdSet->pfds[0].fd, 1);

    unset_pfd(pollFdSet, server, 0);
    ck_assert_int_eq(pollFdSet->count, 0);
    ck_assert_int_eq(pollFdSet->pfds[0].fd, -1);

    delete_pollfd_set(pollFdSet);
    delete_server(server);
}
END_TEST

START_TEST(test_add_message_to_server_queue) {

    TCPServer *server = create_server("irc.example.com");

    add_message_to_server_queue(server, &(RegMessage){"message"});
    ck_assert_int_eq(server->msgQueue->count, 1);

    delete_server(server);
}
END_TEST

START_TEST(test_create_clients) {

    Client *clients = create_clients(MAX_FDS);

    ck_assert_ptr_ne(clients, NULL);
    ck_assert_int_eq(clients[0].registered, 0);
    ck_assert_int_eq(clients[0].fd, -1);

    delete_clients(clients);
}
END_TEST

START_TEST(test_find_pfd_index) {

    PollFdSet *pollFdSet = create_pollfd_set();
    TCPServer *server = create_server("irc.example.com");
    
    set_pfd(pollFdSet, server, 0, 1);

    int index = find_pfd_index(pollFdSet, 1);
    ck_assert_int_eq(index, 0);

    index = find_pfd_index(pollFdSet, -1);
    ck_assert_int_eq(index, 1);

    index = find_pfd_index(pollFdSet, -2);
    ck_assert_int_eq(index, -1);

    delete_pollfd_set(pollFdSet);
    delete_server(server);
}
END_TEST

START_TEST(test_server_read) {

    char input1[30] = "This is a";
    mockBuffP = input1;
    mockLen = strlen(input1);
    mockFd = 5; 

    int i = 1;

    PollFdSet *pollFdSet = create_pollfd_set();
    TCPServer *server = create_server("irc.example.com");
    
    set_pfd(pollFdSet, server, i, mockFd);

    int fullRead = server_read(pollFdSet, server, i);

    ck_assert_int_eq(fullRead, 0);
    ck_assert_str_eq(input1, server->clients[i].inBuffer);

    char input2[] = " full sentence\n";
    mockBuffP = input2;
    mockLen = strlen(input2);

    fullRead = server_read(pollFdSet, server, i);

    ck_assert_int_eq(fullRead, 1);
    ck_assert_str_eq(strcat(input1, input2), server->clients[i].inBuffer);

    delete_pollfd_set(pollFdSet);
    delete_server(server);
}
END_TEST


START_TEST(test_server_write) {

    char input[] = "This is a full sentence\n";
    char output[30] = {'\0'};
    mockBuffP = output;
    mockLen = 5;
    mockFd = 5; 

    int i = 1;

    PollFdSet *pollFdSet = create_pollfd_set();
    TCPServer *server = create_server("irc.example.com");
    
    set_pfd(pollFdSet, server, i, mockFd);

    strcpy(server->clients[i].inBuffer, input);

    server_write(server, i);

    ck_assert_str_eq(input, output);
    ck_assert_int_eq(server->clients[i].inBuffer[0], '\0');

    delete_pollfd_set(pollFdSet);
    delete_server(server);
}
END_TEST


Suite* tcpserver_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("TCPServer");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_pollfd_set);
    tcase_add_test(tc_core, test_is_pfd_set_empty);
    tcase_add_test(tc_core, test_is_pfd_set_full);
    tcase_add_test(tc_core, test_create_server);
    tcase_add_test(tc_core, test_set_pfd);
    tcase_add_test(tc_core, test_unset_pfd);
    tcase_add_test(tc_core, test_add_message_to_server_queue);
    tcase_add_test(tc_core, test_create_clients);
    tcase_add_test(tc_core, test_find_pfd_index);
    tcase_add_test(tc_core, test_server_read);
    tcase_add_test(tc_core, test_server_write);

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