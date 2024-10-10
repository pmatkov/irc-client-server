#include "../src/priv_tcpserver.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/priv_message.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/mock.h"

#include <check.h>
#include <unistd.h>

#define INT_DIGITS 10

START_TEST(test_create_server) {

    set_default_settings();
    read_settings(NULL, SERVER_PROPERTY);

    TCPServer *server = create_server(0);

    ck_assert_ptr_ne(server, NULL);
    ck_assert_ptr_ne(server->pfds, NULL);
    ck_assert_ptr_ne(server->clients, NULL);
    ck_assert_ptr_ne(server->session, NULL);
    ck_assert_ptr_ne(server->msgQueue, NULL);
    ck_assert_str_eq(server->serverName, "irc.example.com");
    ck_assert_int_eq(server->capacity, MAX_FDS);
    ck_assert_int_eq(server->count, 0);

    delete_server(server);
}
END_TEST

START_TEST(test_create_pfds) {

    struct pollfd *pfds = create_pfds(0);

    ck_assert_ptr_ne(pfds, NULL);

    delete_pfds(pfds);
}
END_TEST

START_TEST(test_are_pfds_empty) {

    TCPServer *server = create_server(0);

    ck_assert_int_eq(are_pfds_empty(server), 1);

    delete_server(server);
}
END_TEST

START_TEST(test_are_pfds_full) {

    TCPServer *server = create_server(0);

    ck_assert_int_eq(are_pfds_full(server), 0);

    delete_server(server);
}
END_TEST

START_TEST(test_set_fd) {

    TCPServer *server = create_server(0);
    
    set_fd(server, 0, 1);

    ck_assert_int_eq(server->pfds[0].fd, 1);
    ck_assert_int_eq(server->pfds[0].events, POLLIN);
    ck_assert_int_eq(server->count, 1);
    ck_assert_int_eq(*server->clients[0].fd, 1);

    delete_server(server);
}
END_TEST

START_TEST(test_unset_fd) {

    TCPServer *server = create_server(0);
    
    set_fd(server, 0, 1);
    ck_assert_int_eq(server->count, 1);
    ck_assert_int_eq(server->pfds[0].fd, 1);
    ck_assert_int_eq(*server->clients[0].fd, 1);

    unset_fd(server, 0);
    ck_assert_int_eq(server->count, 0);
    ck_assert_int_eq(server->pfds[0].fd, -1);
    ck_assert_ptr_eq(server->clients[0].fd, NULL);

    delete_server(server);
}
END_TEST

START_TEST(test_is_fd_ready) {

    TCPServer *server = create_server(0);
   
    int fdIndex = 0;

    set_fd(server, fdIndex, 1);
    server->pfds[fdIndex].revents = POLLIN;

    int fdReady = is_fd_ready(server, fdIndex);

    ck_assert_int_eq(fdReady, 1);

    delete_server(server);
}
END_TEST

START_TEST(test_create_clients) {

    Client *clients = create_clients(10);

    ck_assert_ptr_ne(clients, NULL);
    ck_assert_str_eq(clients[0].nickname, "");
    ck_assert_str_eq(clients[0].inBuffer, "");
    ck_assert_int_eq(clients[0].registered, 0);
    ck_assert_ptr_eq(clients[0].fd, NULL);
    ck_assert_ptr_ne(clients[0].timer, NULL);

    delete_clients(clients, 0);
}
END_TEST

START_TEST(test_add_client) {

    set_mock_fd(3);
    set_mock_port(50200);
    set_initial_fd_count(get_mock_fd());

    const int fdIndex = 0;

    TCPServer *server = create_server(0);

    set_fd(server, fdIndex, get_mock_fd());

    add_client(server, fdIndex);
    
    ck_assert_int_eq(server->count, 2);
    ck_assert_int_eq(server->pfds[fdIndex + 1].fd, get_mock_fd() + 1);
    ck_assert_int_eq(server->pfds[fdIndex + 1].events, POLLIN);

    char port[INT_DIGITS] = {'\0'};

    uint_to_str(port, sizeof(port)/sizeof(port[0]), get_mock_port());

    ck_assert_int_eq(*server->clients[fdIndex + 1].fd, get_mock_fd() + 1);
    ck_assert_str_eq(server->clients[fdIndex + 1].ipv4Address, "127.0.0.1");
    ck_assert_int_eq(server->clients[fdIndex + 1].port, get_mock_port());
    ck_assert_int_eq(server->clients[fdIndex + 1].registered, 0);

    delete_server(server);
}
END_TEST

START_TEST(test_remove_client) {

    set_mock_fd(3);
    set_mock_port(50200);
    set_initial_fd_count(get_mock_fd());

    const int listenFdIndex = 0;

    TCPServer *server = create_server(0);

    set_fd(server, listenFdIndex, get_mock_fd());

    add_client(server, listenFdIndex);
    
    ck_assert_int_eq(server->count, 2);
    ck_assert_int_eq(server->pfds[listenFdIndex + 1].fd, get_mock_fd() + 1);
    ck_assert_int_eq(*server->clients[listenFdIndex + 1].fd, get_mock_fd() + 1);
    ck_assert_int_eq(server->pfds[listenFdIndex + 1].events, POLLIN);

    remove_client(server, listenFdIndex + 1);

    ck_assert_int_eq(server->count, 1);
    ck_assert_int_eq(server->pfds[listenFdIndex + 1].fd, -1);
    ck_assert_ptr_eq(server->clients[listenFdIndex + 1].fd, NULL);
    ck_assert_str_eq(server->clients[listenFdIndex + 1].nickname, "");

    delete_server(server);
}
END_TEST

START_TEST(test_find_client) {

    TCPServer *server = create_server(0);

    int index1 = 0;
    int index2 = 1;

    set_fd(server, index1, 4);
    set_client_nickname(&server->clients[index1], "john");

    set_fd(server, index2, 5);
    set_client_nickname(&server->clients[index2], "mark");

    Client *client = find_client(server, "mark");

    ck_assert_int_eq(get_client_fd(client), 5);

    delete_server(server);
}
END_TEST

START_TEST(test_remove_inactive_clients) {

    set_mock_fd(3);
    set_mock_port(50200);
    set_initial_fd_count(get_mock_fd());

    const int listenFdIndex = 0;
    const int waitingTime = 2;

    TCPServer *server = create_server(0);

    set_fd(server, listenFdIndex, get_mock_fd());

    add_client(server, listenFdIndex);
    ck_assert_int_eq(server->count, 2);

    sleep(2);

    remove_inactive_clients(server, waitingTime);

    ck_assert_int_eq(server->count, 1);

    delete_server(server);
}
END_TEST


START_TEST(test_add_message_to_queue) {

    set_mock_fd(4);

    TCPServer *server = create_server(0);

    set_fd(server, 0, get_mock_fd());

    char fdBuffer[INT_DIGITS + 1] = {'\0'};
    uint_to_str(fdBuffer, INT_DIGITS + 1, *server->clients[0].fd);
 
    set_client_nickname(&server->clients[0], "john");
    set_client_registered(&server->clients[0], 0);

    add_message_to_queue(server, &server->clients[0], "message");
    ck_assert_int_eq(server->msgQueue->count, 1);

    ExtMessage *message = remove_message_from_server_queue(server);

    ck_assert_str_eq(get_ext_message_recipient(message), fdBuffer);
    ck_assert_str_eq(get_ext_message_content(message), "message");

    delete_server(server);
}
END_TEST

START_TEST(test_add_remove_message_from_server_queue) {

    TCPServer *server = create_server(0);

    add_message_to_server_queue(server, &(RegMessage){"message"});

    ck_assert_int_eq(server->msgQueue->count, 1);

    RegMessage *message = remove_message_from_server_queue(server);

    ck_assert_int_eq(server->msgQueue->count, 0);
    ck_assert_str_eq(get_reg_message_content(message), "message");

    delete_server(server);
}
END_TEST

START_TEST(test_find_fd_index) {

    TCPServer *server = create_server(0);
    
    set_fd(server, 0, 1);

    int fdIndex = find_fd_index(server, 1);
    ck_assert_int_eq(fdIndex, 0);

    fdIndex = find_fd_index(server, -1);
    ck_assert_int_eq(fdIndex, 1);

    fdIndex = find_fd_index(server, -2);
    ck_assert_int_eq(fdIndex, -1);

    delete_server(server);
}
END_TEST

START_TEST(test_server_read) {

    char input1[MAX_CHARS + 1] = "This is a";

    set_mock_fd(5);
    set_mock_len(strlen(input1));
    set_mock_buffer(input1);

    const int fdIndex = 1;

    TCPServer *server = create_server(0);
    
    set_fd(server, fdIndex, get_mock_fd());

    int fullRead = server_read(server, fdIndex);

    ck_assert_int_eq(fullRead, 0);
    ck_assert_str_eq(input1, server->clients[fdIndex].inBuffer);

    char input2[] = " full sentence\r\n";

    set_mock_len(strlen(input2));
    set_mock_buffer(input2);

    fullRead = server_read(server, fdIndex);

    ck_assert_int_eq(fullRead, 1);
    ck_assert_str_eq(strcat(input1, input2), server->clients[fdIndex].inBuffer);

    delete_server(server);
}
END_TEST


START_TEST(test_server_write) {

    char input[MAX_CHARS + 1] = {'\0'};
    char output[MAX_CHARS + 1] = {'\0'};

    set_mock_fd(5);
    set_mock_len(5);
    set_mock_buffer(output);

    const int fdIndex = 1;

    crlf_terminate(input, MAX_CHARS + 1, "This is a full sentence");

    TCPServer *server = create_server(0);
    
    set_fd(server, fdIndex, get_mock_fd());

    server_write(input, get_mock_fd());

    ck_assert_str_eq(input, output);
    ck_assert_int_eq(server->clients[fdIndex].inBuffer[0], '\0');

    delete_server(server);
}
END_TEST


Suite* tcpserver_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("TCPServer");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_server);
    tcase_add_test(tc_core, test_create_pfds);
    tcase_add_test(tc_core, test_are_pfds_empty);
    tcase_add_test(tc_core, test_are_pfds_full);
    tcase_add_test(tc_core, test_set_fd);
    tcase_add_test(tc_core, test_unset_fd);
    tcase_add_test(tc_core, test_is_fd_ready);
    tcase_add_test(tc_core, test_create_clients);
    tcase_add_test(tc_core, test_add_client);
    tcase_add_test(tc_core, test_remove_client);
    tcase_add_test(tc_core, test_find_client);
    tcase_add_test(tc_core, test_remove_inactive_clients);
    tcase_add_test(tc_core, test_add_message_to_queue);
    tcase_add_test(tc_core, test_add_remove_message_from_server_queue);
    tcase_add_test(tc_core, test_find_fd_index);
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