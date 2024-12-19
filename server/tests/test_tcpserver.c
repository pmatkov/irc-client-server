#include "../src/priv_tcp_server.h"
#include "../src/priv_client.h"
#include "../../libs/src/common.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/poll_manager.h"
#include "../../libs/src/mock.h"

#include <check.h>
#include <unistd.h>

#define LISTEN_FD 3
#define CLIENT_FD 4
#define CLIENT_FD_IDX 0
#define CLIENT_IDENTIFIER "client.irc.com"
#define CLIENT_PORT 50101

static void set_mock_data(int fd, int port, int initialFdCount) {

    set_mock_fd(LISTEN_FD);
    set_mock_port(CLIENT_PORT);
    set_initial_fd_count(get_mock_fd());
}

static void decode_message(char *buffer, int size, int *fd, const char **content, const char *message) {

    if (message != NULL && count_delimiters(message, "|") == 1) {

        safe_copy(buffer, size, message);

        const char *tokens[2] = {NULL};
        tokenize_string(buffer, tokens, ARRAY_SIZE(tokens), "|");

        *fd = str_to_uint(tokens[0]);
        *content = tokens[1];
    }
}

START_TEST(test_create_server) {

    TCPServer *server = create_server(0);

    ck_assert_ptr_ne(server, NULL);
    ck_assert_int_eq(server->listenFd, UNASSIGNED);
    ck_assert_ptr_ne(server->clients, NULL);
    ck_assert_ptr_ne(server->session, NULL);
    ck_assert_ptr_ne(server->outQueue, NULL);
    ck_assert_ptr_ne(server->fdsIdxMap, NULL);
    ck_assert_int_eq(server->count, 0);
    ck_assert_int_eq(server->capacity, DEF_FDS);

    delete_server(server);
}
END_TEST

START_TEST(test_find_client_fd_idx) {

    TCPServer *server = create_server(0);

    int fdIdx = find_client_fd_idx(server, UNASSIGNED);

    ck_assert_int_eq(fdIdx, CLIENT_FD_IDX);

    delete_server(server);

}
END_TEST

START_TEST(test_set_unset_client_data) {

    TCPServer *server = create_server(0);

    set_client_data(server, CLIENT_FD_IDX, CLIENT_FD, CLIENT_IDENTIFIER, HOSTNAME, CLIENT_PORT);

    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->fd, CLIENT_FD);
    ck_assert_str_eq(server->clients[CLIENT_FD_IDX]->clientIdentifier, CLIENT_IDENTIFIER);
    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->identifierType, HOSTNAME);
    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->port, CLIENT_PORT);

    unset_client_data(server, CLIENT_FD_IDX);

    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->fd, UNASSIGNED);
    ck_assert_str_eq(server->clients[CLIENT_FD_IDX]->clientIdentifier, "");
    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->identifierType, UNKNOWN_HOST_IDENTIFIER);
    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->port, UNASSIGNED);

    delete_server(server);

}
END_TEST

START_TEST(test_is_server_empty) {

    TCPServer *server = create_server(0);

    ck_assert_int_eq(is_server_empty(server), 1);

    delete_server(server);
}
END_TEST

START_TEST(test_is_server_full) {

    TCPServer *server = create_server(0);

    ck_assert_int_eq(is_server_full(server), 0);

    delete_server(server);
}
END_TEST

START_TEST(test_add_client) {

    set_mock_data(LISTEN_FD, CLIENT_PORT, get_mock_fd());

    struct sockaddr_in sa;
    memset((struct sockaddr_in *) &sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(CLIENT_PORT);
    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

    set_mock_sockaddr(&sa);

    TCPServer *server = create_server(0);
    set_server_listen_fd(server, LISTEN_FD);

    add_client(server, NULL);
    
    ck_assert_int_eq(server->count, 1);

    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->fd, CLIENT_FD);
    ck_assert_str_eq(server->clients[CLIENT_FD_IDX]->clientIdentifier, "localhost");
    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->identifierType, HOSTNAME);
    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->port, CLIENT_PORT);
    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->stateType, CONNECTED);

    delete_server(server);
}
END_TEST

START_TEST(test_remove_client) {

    set_mock_data(LISTEN_FD, CLIENT_PORT, get_mock_fd());

    TCPServer *server = create_server(0);
    set_server_listen_fd(server, LISTEN_FD);

    add_client(server, NULL);

    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->fd, CLIENT_FD);

    remove_client(server, NULL, CLIENT_FD);

    ck_assert_int_eq(server->count, 0);
    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->fd, UNASSIGNED);
    ck_assert_str_eq(server->clients[CLIENT_FD_IDX]->clientIdentifier, "");
    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->identifierType, UNKNOWN_HOST_IDENTIFIER);
    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->port, UNASSIGNED);
    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->stateType, DISCONNECTED);

    delete_server(server);
}
END_TEST

START_TEST(test_find_client) {

    set_mock_data(LISTEN_FD, CLIENT_PORT, get_mock_fd());

    TCPServer *server = create_server(0);
    set_server_listen_fd(server, LISTEN_FD);
  
    add_client(server, NULL);
    ck_assert_int_ne(server->clients[CLIENT_FD_IDX]->fd, UNASSIGNED);
    set_client_nickname(server->clients[CLIENT_FD_IDX], "john");

    add_client(server, NULL);
    ck_assert_int_ne(server->clients[CLIENT_FD_IDX + 1]->fd, UNASSIGNED);
    set_client_nickname(server->clients[CLIENT_FD_IDX + 1], "mark");

    Client *client = find_client(server, "mark");

    ck_assert_int_eq(client->fd, CLIENT_FD + 1);

    delete_server(server);
}
END_TEST

START_TEST(test_add_message_to_queue) {

    set_mock_data(LISTEN_FD, CLIENT_PORT, get_mock_fd());

    TCPServer *server = create_server(0);
    set_server_listen_fd(server, LISTEN_FD);

    add_client(server, NULL);
 
    set_client_nickname(server->clients[CLIENT_FD_IDX], "john");
    set_client_state_type(server->clients[CLIENT_FD_IDX], CONNECTED);

    add_message_to_queue(server, server->clients[CLIENT_FD_IDX], "message");
    ck_assert_int_eq(server->outQueue->count, 1);

    const char *message = dequeue_from_server_queue(server);

    char buffer[MAX_CHARS + 1] = {'\0'};
    int fd;
    const char *content = NULL;

    decode_message(buffer, ARRAY_SIZE(buffer), &fd, &content, message);

    ck_assert_int_eq(fd, CLIENT_FD);
    ck_assert_str_eq(content, "message");

    delete_server(server);
}
END_TEST

START_TEST(test_enqueue_dequeue_server) {

    TCPServer *server = create_server(0);
    set_server_listen_fd(server, LISTEN_FD);

    enqueue_to_server_queue(server, "message");

    ck_assert_int_eq(server->outQueue->count, 1);

    const char *content = dequeue_from_server_queue(server);

    ck_assert_int_eq(server->outQueue->count, 0);
    ck_assert_str_eq(content, "message");

    delete_server(server);
}
END_TEST


START_TEST(test_server_read) {

    set_mock_data(LISTEN_FD, CLIENT_PORT, get_mock_fd());

    char input1[MAX_CHARS + 1] = "This is a";

    set_mock_buffer_size(strlen(input1));
    set_mock_buffer(input1);

    TCPServer *server = create_server(0);
    set_server_listen_fd(server, LISTEN_FD);

    add_client(server, NULL);

    set_mock_fd(CLIENT_FD);
    
    int readStatus = server_read(server, NULL, CLIENT_FD);

    ck_assert_int_eq(readStatus, 0);
    ck_assert_str_eq(input1, server->clients[CLIENT_FD_IDX]->inBuffer);

    char input2[] = " full sentence\r\n";

    set_mock_buffer(input2);
    set_mock_buffer_size(strlen(input2));

    readStatus = server_read(server, NULL, CLIENT_FD);

    ck_assert_int_eq(readStatus, 1);
    ck_assert_str_eq(strcat(input1, input2), server->clients[CLIENT_FD_IDX]->inBuffer);

    delete_server(server);
}
END_TEST


START_TEST(test_server_write) {

    set_mock_data(LISTEN_FD, CLIENT_PORT, get_mock_fd());

    char input[] = "This is a full sentence";
    char output[MAX_CHARS + 1] = {'\0'};

    set_mock_buffer_size(sizeof(output));
    set_mock_buffer(output);

    TCPServer *server = create_server(0);
    set_server_listen_fd(server, LISTEN_FD);

    add_client(server, NULL);

    set_mock_fd(CLIENT_FD);

    server_write(server, NULL, CLIENT_FD, input);

    ck_assert_str_eq(output, "This is a full sentence\r\n");
    ck_assert_int_eq(server->clients[CLIENT_FD_IDX]->inBuffer[0], '\0');

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
    tcase_add_test(tc_core, test_find_client_fd_idx);
    tcase_add_test(tc_core, test_set_unset_client_data);
    tcase_add_test(tc_core, test_is_server_empty);
    tcase_add_test(tc_core, test_is_server_full);
    tcase_add_test(tc_core, test_add_client);
    tcase_add_test(tc_core, test_remove_client);
    tcase_add_test(tc_core, test_find_client);
    tcase_add_test(tc_core, test_add_message_to_queue);
    tcase_add_test(tc_core, test_enqueue_dequeue_server);
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