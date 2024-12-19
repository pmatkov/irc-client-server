#include "../src/priv_tcp_client.h"
#include "../../libs/src/priv_message.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/mock.h"

#include <check.h>
#include <poll.h>

#define DEF_IP_ADDRESS "127.0.0.1"
#define DEF_PORT 50100

START_TEST(test_create_client) {

    TCPClient *tcpClient = create_client();

    ck_assert_ptr_ne(tcpClient, NULL);
    
    ck_assert_int_eq(tcpClient->fd, UNASSIGNED);
    ck_assert_str_eq(tcpClient->serverIdentifier, "");
    ck_assert_int_eq(tcpClient->identifierType, UNKNOWN_HOST_IDENTIFIER);
    ck_assert_int_eq(tcpClient->port, UNASSIGNED);
    ck_assert_str_eq(tcpClient->inBuffer, "");
    ck_assert_ptr_ne(tcpClient->msgQueue, NULL);
    ck_assert_ptr_ne(tcpClient->timer, NULL);
    ck_assert_int_eq(tcpClient->clientState, DISCONNECTED);

    delete_client(tcpClient);
}
END_TEST

START_TEST(test_client_connect) {

    set_mock_fd(1);

    TCPClient *tcpClient = create_client();

    int status = client_connect(tcpClient, NULL, DEF_IP_ADDRESS, DEF_PORT);

    ck_assert_int_eq(status, 0);
    ck_assert_int_eq(tcpClient->fd, get_mock_fd());
    ck_assert_str_eq(tcpClient->serverIdentifier, "localhost");
    ck_assert_int_eq(tcpClient->identifierType, HOSTNAME);

    delete_client(tcpClient);
}
END_TEST

START_TEST(test_client_disconnect) {

    set_mock_fd(1);

    TCPClient *tcpClient = create_client();

    client_connect(tcpClient, NULL, DEF_IP_ADDRESS, DEF_PORT);
    client_disconnect(tcpClient, NULL);

    ck_assert_int_eq(tcpClient->clientState, DISCONNECTED);

    delete_client(tcpClient);
}
END_TEST

START_TEST(test_client_read) {

    char input1[MAX_CHARS + 1] = "This is a";

    set_mock_fd(1);
    set_mock_buffer(input1);
    set_mock_buffer_size(strlen(input1));

    TCPClient *tcpClient = create_client();
    
    set_client_fd(tcpClient, get_mock_fd());

    int readStatus = client_read(tcpClient, NULL);

    ck_assert_int_eq(readStatus, 0);
    ck_assert_str_eq(input1, tcpClient->inBuffer);

    char input2[] = " full sentence\r\n";
    set_mock_buffer_size(strlen(input2));
    set_mock_buffer(input2);

    readStatus = client_read(tcpClient, NULL);

    ck_assert_int_eq(readStatus, 1);
    ck_assert_str_eq(strncat(input1, input2, strlen(input2)), tcpClient->inBuffer);

    delete_client(tcpClient);
}
END_TEST

START_TEST(test_client_write) {

    char input[MAX_CHARS + 1] = {'\0'};
    char output[MAX_CHARS + 1] = {'\0'};

    set_mock_fd(5);
    set_mock_buffer_size(ARRAY_SIZE(output));
    set_mock_buffer(output);

    terminate_string(input, ARRAY_SIZE(input), "This is a full sentence", CRLF);

    TCPClient *tcpClient = create_client();

    set_client_fd(tcpClient, get_mock_fd());

    client_write(tcpClient, NULL, input);

    ck_assert_str_eq(input, output);
    ck_assert_int_eq(is_terminated(output, CRLF), 1);

    delete_client(tcpClient);
}
END_TEST


START_TEST(test_enqueue_dequeue_client_queue) {

    TCPClient *tcpClient = create_client();

    enqueue_to_client_queue(tcpClient, "message");

    ck_assert_int_eq(tcpClient->msgQueue->count, 1);

    char *message = dequeue_from_client_queue(tcpClient);

    ck_assert_int_eq(tcpClient->msgQueue->count, 0);
    ck_assert_str_eq(message, "message");

    delete_client(tcpClient);
}
END_TEST

START_TEST(test_initialize_terminate_session) {

    TCPClient *tcpClient = create_client();

    initialize_session(tcpClient, 0, "irc.server.com", HOSTNAME, DEF_PORT);
    ck_assert_int_eq(tcpClient->fd, 0);
    ck_assert_str_eq(tcpClient->serverIdentifier, "irc.server.com");
    ck_assert_int_eq(tcpClient->identifierType, HOSTNAME);
    ck_assert_int_eq(tcpClient->port, 50100);

    terminate_session(tcpClient);
    ck_assert_int_eq(tcpClient->fd, UNASSIGNED);
    ck_assert_str_eq(tcpClient->serverIdentifier, "");
    ck_assert_int_eq(tcpClient->identifierType, UNKNOWN_HOST_IDENTIFIER);
    ck_assert_int_eq(tcpClient->port, UNASSIGNED);

    delete_client(tcpClient);

}
END_TEST

START_TEST(test_get_set_client_values) {

    TCPClient *tcpClient = create_client();

    set_client_fd(tcpClient, 5);
    ck_assert_int_eq(get_client_fd(tcpClient), 5);

    set_server_identifier(tcpClient, "irc.server.com", HOSTNAME);
    ck_assert_str_eq(get_server_identifier(tcpClient), "irc.server.com");

    set_client_inbuffer(tcpClient, "message");
    ck_assert_str_eq(get_client_inbuffer(tcpClient), "message");

    ck_assert_ptr_ne(get_client_queue(tcpClient), NULL);

    ck_assert_int_eq(is_client_connected(tcpClient), 1);


    delete_client(tcpClient);
}
END_TEST

Suite* tcp_client_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("TCP client");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_create_client);
    tcase_add_test(tc_core, test_client_connect);
    tcase_add_test(tc_core, test_client_disconnect);
    tcase_add_test(tc_core, test_client_read);
    tcase_add_test(tc_core, test_client_write);
    tcase_add_test(tc_core, test_enqueue_dequeue_client_queue);
    tcase_add_test(tc_core, test_initialize_terminate_session);
    tcase_add_test(tc_core, test_get_set_client_values);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = tcp_client_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
