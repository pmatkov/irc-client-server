#include "../src/priv_tcp_server.h"
#include "../src/priv_client.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/priv_message.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/mock.h"

#include <check.h>
#include <unistd.h>

#define SERVER_NAME "irc.server.com"

START_TEST(test_create_server) {

    TCPServer *server = create_server(0, SERVER_NAME);

    ck_assert_ptr_ne(server, NULL);
    ck_assert_ptr_ne(server->pfds, NULL);
    ck_assert_ptr_ne(server->clients, NULL);
    ck_assert_ptr_ne(server->session, NULL);
    ck_assert_ptr_ne(server->outQueue, NULL);
    ck_assert_str_eq(server->servername, SERVER_NAME);
    ck_assert_int_eq(server->capacity, MAX_FDS);
    ck_assert_int_eq(server->pfdsCount, 0);
    ck_assert_int_eq(server->clientsCount, 0);

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

    TCPServer *server = create_server(0, SERVER_NAME);

    ck_assert_int_eq(are_pfds_empty(server), 1);

    delete_server(server);
}
END_TEST

START_TEST(test_are_pfds_full) {

    TCPServer *server = create_server(0, SERVER_NAME);

    ck_assert_int_eq(are_pfds_full(server), 0);

    delete_server(server);
}
END_TEST

START_TEST(test_assign_fd) {

    TCPServer *server = create_server(0, SERVER_NAME);
    
    assign_fd(server, 0, 1);
    assign_client_fd(server, 0, 1);

    ck_assert_int_eq(server->pfds[0].fd, 1);
    ck_assert_int_eq(server->pfds[0].events, POLLIN);
    ck_assert_int_eq(*server->clients[0]->fd, 1);
    ck_assert_int_eq(server->pfdsCount, 1);
    ck_assert_int_eq(server->clientsCount, 1);

    delete_server(server);
}
END_TEST

START_TEST(test_unassign_fd) {

    TCPServer *server = create_server(0, SERVER_NAME);
    
    assign_fd(server, 0, 1);
    assign_client_fd(server, 0, 1);
    ck_assert_int_eq(server->pfdsCount, 1);
    ck_assert_int_eq(server->clientsCount, 1);

    unassign_fd(server, 0);
    unassign_client_fd(server, 0);
    ck_assert_int_eq(server->pfdsCount, 0);
    ck_assert_int_eq(server->clientsCount, 0);
    ck_assert_int_eq(server->pfds[0].fd, -1);
    ck_assert_ptr_eq(server->clients[0]->fd, NULL);

    delete_server(server);
}
END_TEST

START_TEST(test_is_fd_ready) {

    TCPServer *server = create_server(0, SERVER_NAME);
   
    int fdIndex = 0;

    assign_fd(server, fdIndex, 1);
    server->pfds[fdIndex].revents = POLLIN;

    int fdReady = is_fd_ready(server, fdIndex);

    ck_assert_int_eq(fdReady, 1);

    delete_server(server);
}
END_TEST

START_TEST(test_add_client) {

    set_mock_fd(3);
    set_mock_port(50200);
    set_initial_fd_count(get_mock_fd());

    struct sockaddr_in sa;
    memset((struct sockaddr_in *) &sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(50200);
    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

    set_mock_sockaddr(&sa);

    const int FD_INDEX = 0;

    TCPServer *server = create_server(0, SERVER_NAME);

    assign_fd(server, FD_INDEX, get_mock_fd());

    add_client(server);
    
    ck_assert_int_eq(server->pfdsCount, 2);
    ck_assert_int_eq(server->clientsCount, 1);
    ck_assert_int_eq(server->pfds[FD_INDEX + 1].fd, get_mock_fd() + 1);
    ck_assert_int_eq(server->pfds[FD_INDEX + 1].events, POLLIN);

    ck_assert_int_eq(*server->clients[FD_INDEX + 1]->fd, get_mock_fd() + 1);
    ck_assert_str_eq(server->clients[FD_INDEX + 1]->ipv4Address, "127.0.0.1");
    ck_assert_int_eq(server->clients[FD_INDEX + 1]->port, get_mock_port());
    ck_assert_int_eq(server->clients[FD_INDEX + 1]->registered, 0);

    delete_server(server);
}
END_TEST

START_TEST(test_remove_client) {

    set_mock_fd(3);
    set_mock_port(50200);
    set_initial_fd_count(get_mock_fd());

    const int LISTEN_FD_INDEX = 0;

    TCPServer *server = create_server(0, SERVER_NAME);

    assign_fd(server, LISTEN_FD_INDEX, get_mock_fd());

    add_client(server);
    
    ck_assert_int_eq(server->pfdsCount, 2);
    ck_assert_int_eq(server->clientsCount, 1);
    ck_assert_int_eq(server->pfds[LISTEN_FD_INDEX + 1].fd, get_mock_fd() + 1);
    ck_assert_int_eq(server->pfds[LISTEN_FD_INDEX + 1].events, POLLIN);
    ck_assert_int_eq(*server->clients[LISTEN_FD_INDEX + 1]->fd, get_mock_fd() + 1);

    remove_client(server, LISTEN_FD_INDEX + 1);

    ck_assert_int_eq(server->pfdsCount, 1);
    ck_assert_int_eq(server->clientsCount, 0);
    ck_assert_int_eq(server->pfds[LISTEN_FD_INDEX + 1].fd, -1);
    ck_assert_ptr_eq(server->clients[LISTEN_FD_INDEX + 1]->fd, NULL);
    ck_assert_str_eq(server->clients[LISTEN_FD_INDEX + 1]->nickname, "");

    delete_server(server);
}
END_TEST

START_TEST(test_find_client) {

    TCPServer *server = create_server(0, SERVER_NAME);

    const int INDEX1 = 0, INDEX2 = 1, FD1 = 3, FD2 = 4;

    assign_fd(server, INDEX1, FD1);
    assign_client_fd(server, INDEX1, FD1);    
    set_client_nickname(server->clients[INDEX1], "john");

    assign_fd(server, INDEX2, FD2);
    assign_client_fd(server, INDEX2, FD2);
    set_client_nickname(server->clients[INDEX2], "mark");

    Client *client = find_client(server, "mark");

    ck_assert_int_eq(*get_client_fd(client), FD2);

    delete_server(server);
}
END_TEST

// START_TEST(test_remove_unregistered_clients) {

//     set_mock_fd(3);
//     set_mock_port(50200);
//     set_initial_fd_count(get_mock_fd());

//     const int LISTEN_FD_INDEX = 0;
//     const int waitingTime = 2;

//     TCPServer *server = create_server(0, SERVER_NAME);

//     assign_fd(server, LISTEN_FD_INDEX, get_mock_fd());

//     add_client(server);
//     ck_assert_int_eq(server->pfdsCount, 2);
//     ck_assert_int_eq(server->clientsCount, 1);

//     sleep(2);

//     remove_unregistered_clients(server, waitingTime);

//     ck_assert_int_eq(server->pfdsCount, 1);
//     ck_assert_int_eq(server->clientsCount, 0);

//     delete_server(server);
// }
// END_TEST


START_TEST(test_add_message_to_queue) {

    set_mock_fd(4);

    TCPServer *server = create_server(0, SERVER_NAME);

    assign_fd(server, 0, get_mock_fd());
    assign_client_fd(server, 0, get_mock_fd());  

    char fdStr[MAX_DIGITS + 1] = {'\0'};
    uint_to_str(fdStr, ARR_SIZE(fdStr), *server->clients[0]->fd);
 
    set_client_nickname(server->clients[0], "john");
    set_client_registered(server->clients[0], 0);

    add_message_to_queue(server, server->clients[0], "message");
    ck_assert_int_eq(server->outQueue->count, 1);

    ExtMessage *message = dequeue_from_server_queue(server);

    ck_assert_str_eq(get_ext_message_recipient(message), fdStr);
    ck_assert_str_eq(get_ext_message_content(message), "message");

    delete_server(server);
}
END_TEST

START_TEST(test_enqueue_dequeue_server) {

    TCPServer *server = create_server(0, SERVER_NAME);

    enqueue_to_server_queue(server, "message");

    ck_assert_int_eq(server->outQueue->count, 1);

    char *message = dequeue_from_server_queue(server);

    ck_assert_int_eq(server->outQueue->count, 0);
    ck_assert_str_eq(message, "message");

    delete_server(server);
}
END_TEST

START_TEST(test_find_fd_index) {

    TCPServer *server = create_server(0, SERVER_NAME);
    
    assign_fd(server, 0, 1);

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
    set_mock_buffer_size(strlen(input1));
    set_mock_buffer(input1);

    const int FD_INDEX = 1;

    TCPServer *server = create_server(0, SERVER_NAME);
    
    assign_fd(server, FD_INDEX, get_mock_fd());
    assign_client_fd(server, FD_INDEX, get_mock_fd());

    int fullRead = server_read(server, FD_INDEX);

    ck_assert_int_eq(fullRead, 0);
    ck_assert_str_eq(input1, server->clients[FD_INDEX]->inBuffer);

    char input2[] = " full sentence\r\n";

    set_mock_buffer(input2);
    set_mock_buffer_size(strlen(input2));

    fullRead = server_read(server, FD_INDEX);

    ck_assert_int_eq(fullRead, 1);
    ck_assert_str_eq(strcat(input1, input2), server->clients[FD_INDEX]->inBuffer);

    delete_server(server);
}
END_TEST


START_TEST(test_server_write) {

    char input[] = "This is a full sentence";
    char output[MAX_CHARS + 1] = {'\0'};

    set_mock_fd(5);
    set_mock_buffer_size(sizeof(output));
    set_mock_buffer(output);

    const int FD_INDEX = 1;

    TCPServer *server = create_server(0, SERVER_NAME);
    
    assign_fd(server, FD_INDEX, get_mock_fd());
    assign_client_fd(server, FD_INDEX, get_mock_fd());

    server_write(server, FD_INDEX, input);

    ck_assert_str_eq(output, "This is a full sentence\r\n");
    ck_assert_int_eq(server->clients[FD_INDEX]->inBuffer[0], '\0');

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
    tcase_add_test(tc_core, test_assign_fd);
    tcase_add_test(tc_core, test_unassign_fd);
    tcase_add_test(tc_core, test_is_fd_ready);
    tcase_add_test(tc_core, test_add_client);
    tcase_add_test(tc_core, test_remove_client);
    tcase_add_test(tc_core, test_find_client);
    // tcase_add_test(tc_core, test_remove_unregistered_clients);
    tcase_add_test(tc_core, test_add_message_to_queue);
    tcase_add_test(tc_core, test_enqueue_dequeue_server);
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