#include "../src/priv_command_handler.h"
#include "../src/priv_line_editor.h"
#include "../src/priv_scrollback.h"
#include "../src/priv_tcpclient.h"
#include "../../shared/src/priv_command.h"
#include "../../shared/src/priv_message.h"
#include "../../shared/src/priv_settings.h"
#include "../../shared/src/string_utils.h"
#include "../../shared/src/mock.h"

#include <check.h>
#include <arpa/inet.h>

#define PROMPT_SIZE 2
#define SOCKET_FD_INDEX 1

void set_message(LineEditor *lnEditor, const char *content) {

    RegMessage *message = create_reg_message(content);
    enqueue(lnEditor->buffer, message);

    lnEditor->cursor = PROMPT_SIZE + strlen(content);
    lnEditor->charCount = strlen(content);
    lnEditor->buffer->currentItem--;

    delete_message(message);
}

void process_command(TCPClient *tcpClient, CommandTokens *cmdTokens, CommandFunction cmdFunction, const char *content) {

    Scrollback *sb = create_scrollback(stdscr, 0);
    LineEditor *lnEditor = create_line_editor(NULL);

    set_message(lnEditor, content);
    parse_input(lnEditor, cmdTokens);

    cmdFunction(sb, tcpClient, cmdTokens); 

    delete_line_editor(lnEditor);
    delete_scrollback(sb);

}

START_TEST(test_parse_input) {

    LineEditor *lnEditor = create_line_editor(NULL);
    CommandTokens *cmdTokens = create_command_tokens();

    char *content = "/PRIVMSG #general hello";

    set_message(lnEditor, content);
    parse_input(lnEditor, cmdTokens);

    ck_assert_int_eq(cmdTokens->argCount, 2);
    ck_assert_str_eq(cmdTokens->command, "PRIVMSG");
    ck_assert_str_eq(cmdTokens->args[0], "#general");
    ck_assert_str_eq(cmdTokens->args[1], "hello");

    content = "/CONNECT";

    set_message(lnEditor, content);
    parse_input(lnEditor, cmdTokens);

    ck_assert_int_eq(cmdTokens->argCount, 0);
    ck_assert_str_eq(cmdTokens->command, "CONNECT");

    content = "/PART #general see you tomorrow mates";

    set_message(lnEditor, content);
    parse_input(lnEditor, cmdTokens);

    ck_assert_int_eq(cmdTokens->argCount, 4);
    ck_assert_str_eq(cmdTokens->command, "PART");
    ck_assert_str_eq(cmdTokens->args[0], "#general");
    ck_assert_str_eq(cmdTokens->args[1], "see");
    ck_assert_str_eq(cmdTokens->args[2], "you");
    ck_assert_str_eq(cmdTokens->args[3], "tomorrow mates");

    delete_command_tokens(cmdTokens);
    delete_line_editor(lnEditor);

}
END_TEST


START_TEST(test_cmd_connect) {

    set_default_settings();

    TCPClient *tcpClient = create_client();
    CommandTokens *cmdTokens = create_command_tokens();

    set_mock_fd(1);

    struct sockaddr_in sa;

    memset((struct sockaddr_in *) &sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(50100);
    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

    set_sockaddr(&sa);

    process_command(tcpClient, cmdTokens, get_command_function(CONNECT), "/CONNECT");
    RegMessage *regMessage = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(get_reg_message_content(regMessage), "NICK pmatkov");
    regMessage = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(get_reg_message_content(regMessage), "USER pmatkov 127.0.0.1 * :anonymous");
    reset_cmd_tokens(cmdTokens);

    delete_command_tokens(cmdTokens);
    delete_client(tcpClient);

}
END_TEST

START_TEST(test_cmd_disconnect) {

    TCPClient *tcpClient = create_client();
    CommandTokens *cmdTokens = create_command_tokens();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    process_command(tcpClient, cmdTokens, get_command_function(DISCONNECT), "/DISCONNECT see you tomorrow again");
    RegMessage *regMessage = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(get_reg_message_content(regMessage), "DISCONNECT :see you tomorrow again");
    reset_cmd_tokens(cmdTokens);

    delete_command_tokens(cmdTokens);
    delete_client(tcpClient);

}
END_TEST

START_TEST(test_cmd_nick) {

    set_default_settings();

    TCPClient *tcpClient = create_client();
    CommandTokens *cmdTokens = create_command_tokens();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    process_command(tcpClient, cmdTokens, get_command_function(NICK), "/NICK john");
    RegMessage *regMessage = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(get_reg_message_content(regMessage), "NICK john");
    ck_assert_str_eq(get_property_value(NICKNAME), "john");
    reset_cmd_tokens(cmdTokens);

    delete_command_tokens(cmdTokens);
    delete_client(tcpClient);

}
END_TEST

START_TEST(test_cmd_user) {

    set_default_settings();

    TCPClient *tcpClient = create_client();
    CommandTokens *cmdTokens = create_command_tokens();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    process_command(tcpClient, cmdTokens, get_command_function(USER), "/USER jjones");
    ck_assert_str_eq(get_property_value(USERNAME), "jjones");

    process_command(tcpClient, cmdTokens, get_command_function(USER), "/USER jjones john jones");
    ck_assert_str_eq(get_property_value(USERNAME), "jjones");
    ck_assert_str_eq(get_property_value(REALNAME), "john jones");
    reset_cmd_tokens(cmdTokens);

    delete_command_tokens(cmdTokens);
    delete_client(tcpClient);

}
END_TEST

START_TEST(test_cmd_join) {

    set_default_settings();

    TCPClient *tcpClient = create_client();
    CommandTokens *cmdTokens = create_command_tokens();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    process_command(tcpClient, cmdTokens, get_command_function(JOIN), "/JOIN #general");
    RegMessage *regMessage = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(get_reg_message_content(regMessage), "JOIN #general");
    reset_cmd_tokens(cmdTokens);

    delete_command_tokens(cmdTokens);
    delete_client(tcpClient);

}
END_TEST

START_TEST(test_cmd_part) {

    set_default_settings();

    TCPClient *tcpClient = create_client();
    CommandTokens *cmdTokens = create_command_tokens();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    process_command(tcpClient, cmdTokens, get_command_function(PART), "/PART #general");
    RegMessage *regMessage = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(get_reg_message_content(regMessage), "PART #general");

    process_command(tcpClient, cmdTokens, get_command_function(PART), "/PART #general see you tomorrow again");
    regMessage = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(get_reg_message_content(regMessage), "PART #general :see you tomorrow again");
    reset_cmd_tokens(cmdTokens);

    delete_command_tokens(cmdTokens);
    delete_client(tcpClient);

}
END_TEST

START_TEST(test_cmd_privmsg) {

    set_default_settings();

    TCPClient *tcpClient = create_client();
    CommandTokens *cmdTokens = create_command_tokens();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    process_command(tcpClient, cmdTokens, get_command_function(PRIVMSG), "/PRIVMSG #general hello");
    RegMessage *regMessage = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(get_reg_message_content(regMessage), "PRIVMSG #general :hello");

    reset_cmd_tokens(cmdTokens);

    delete_command_tokens(cmdTokens);
    delete_client(tcpClient);

}
END_TEST

Suite* command_handler_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Command handler");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_parse_input);
    tcase_add_test(tc_core, test_cmd_connect);
    tcase_add_test(tc_core, test_cmd_disconnect);
    tcase_add_test(tc_core, test_cmd_nick);
    tcase_add_test(tc_core, test_cmd_user);
    tcase_add_test(tc_core, test_cmd_join);
    tcase_add_test(tc_core, test_cmd_part);
    tcase_add_test(tc_core, test_cmd_privmsg);

    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = command_handler_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

#endif
