#include "../src/priv_tcp_client.h"
#include "../../libs/src/priv_message.h"
#include "../../libs/src/mock.h"

#include <check.h>
#include <poll.h>

#define DEFAULT_ADDRESS "localhost"
#define DEFAULT_PORT 50100
#define SOCKET_FD_INDEX 1

START_TEST(test_create_client) {

    TCPClient *tcpClient = create_client();

    ck_assert_ptr_ne(tcpClient, NULL);
    ck_assert_ptr_ne(tcpClient->pfds, NULL);
    ck_assert_str_eq(tcpClient->servername, "");
    ck_assert_str_eq(tcpClient->inBuffer, "");
    ck_assert_ptr_ne(tcpClient->msgQueue, NULL);

    delete_client(tcpClient);
}
END_TEST

START_TEST(test_client_connect) {

    set_mock_fd(1);

    TCPClient *tcpClient = create_client();

    int status = client_connect(tcpClient, DEFAULT_ADDRESS, DEFAULT_PORT);

    ck_assert_int_eq(status, 0);
    ck_assert_str_eq(tcpClient->servername, DEFAULT_ADDRESS);
    ck_assert_int_eq(tcpClient->pfds[SOCKET_FD_INDEX].fd, get_mock_fd());

    delete_client(tcpClient);
}
END_TEST

START_TEST(test_client_disconnect) {

    set_mock_fd(1);

    TCPClient *tcpClient = create_client();

    client_connect(tcpClient, DEFAULT_ADDRESS, DEFAULT_PORT);
    client_disconnect(tcpClient);

    ck_assert_int_eq(tcpClient->pfds[SOCKET_FD_INDEX].fd, -1);

    delete_client(tcpClient);
}
END_TEST

START_TEST(test_client_read) {

    char input1[MAX_CHARS + 1] = "This is a";

    set_mock_fd(1);
    set_mock_buffer(input1);
    set_mock_buffer_size(strlen(input1));

    TCPClient *tcpClient = create_client();
    set_fd(tcpClient, SOCKET_FD_INDEX, get_mock_fd());

    int readStatus = client_read(tcpClient);

    ck_assert_int_eq(readStatus, 0);
    ck_assert_str_eq(input1, tcpClient->inBuffer);

    char input2[] = " full sentence\r\n";
    set_mock_buffer_size(strlen(input2));
    set_mock_buffer(input2);

    readStatus = client_read(tcpClient);

    ck_assert_int_eq(readStatus, 1);
    ck_assert_str_eq(strncat(input1, input2, strlen(input2)), tcpClient->inBuffer);

    delete_client(tcpClient);
}
END_TEST

START_TEST(test_client_write) {

    char input[MAX_CHARS + 1] = {'\0'};
    char output[MAX_CHARS + 1] = {'\0'};

    set_mock_fd(5);
    set_mock_buffer_size(ARR_SIZE(output));
    set_mock_buffer(output);

    terminate_string(input, ARR_SIZE(input), "This is a full sentence", CRLF);

    TCPClient *tcpClient = create_client();

    client_write(tcpClient, get_mock_fd(), input);

    ck_assert_str_eq(input, output);
    ck_assert_int_eq(is_terminated(output, CRLF), 1);

    delete_client(tcpClient);
}
END_TEST


START_TEST(test_add_remove_message_from_client_queue) {

    TCPClient *tcpClient = create_client();

    add_message_to_client_queue(tcpClient, &(RegMessage){"message"});

    ck_assert_int_eq(tcpClient->msgQueue->count, 1);

    RegMessage *message = remove_message_from_client_queue(tcpClient);

    ck_assert_int_eq(tcpClient->msgQueue->count, 0);
    ck_assert_str_eq(get_reg_message_content(message), "message");

    delete_client(tcpClient);
}
END_TEST

START_TEST(test_get_set_client_values) {

    TCPClient *tcpClient = create_client();

    ck_assert_ptr_ne(get_fds(tcpClient), NULL);

    set_servername(tcpClient, "irc.example.com");
    ck_assert_str_eq(get_servername(tcpClient), "irc.example.com");

    safe_copy(tcpClient->inBuffer, sizeof(tcpClient->inBuffer), "message");
    ck_assert_str_eq(get_client_inbuffer(tcpClient), "message");
    ck_assert_int_eq(get_char_from_inbuffer(tcpClient, strlen(tcpClient->inBuffer) - 1), 'e');

    set_char_in_inbuffer(tcpClient, '1', strlen(tcpClient->inBuffer));
    ck_assert_int_eq(get_char_from_inbuffer(tcpClient, strlen(tcpClient->inBuffer) - 1), '1');

    ck_assert_ptr_ne(get_client_queue(tcpClient), NULL);

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);
    ck_assert_int_eq(get_socket_fd(tcpClient), 1);
    
    unset_fd(tcpClient, SOCKET_FD_INDEX);
    ck_assert_int_eq(get_socket_fd(tcpClient), -1);

    int connected = is_client_connected(tcpClient);
    ck_assert_int_eq(connected, 0);

    tcpClient->pfds[SOCKET_FD_INDEX].revents = POLLIN;
    ck_assert_int_eq(is_socket_event(tcpClient), 1);

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
    tcase_add_test(tc_core, test_client_connect);
    tcase_add_test(tc_core, test_client_disconnect);
    tcase_add_test(tc_core, test_client_read);
    tcase_add_test(tc_core, test_client_write);
    tcase_add_test(tc_core, test_add_remove_message_from_client_queue);
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
