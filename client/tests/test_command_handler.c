#ifndef TEST
#define TEST
#endif

#include "../src/priv_command_handler.h"
#include "../src/main.h"
#include "../src/priv_line_editor.h"
#include "../src/priv_scrollback.h"
#include "../src/priv_tcp_client.h"
#include "../../libs/src/priv_command.h"
#include "../../libs/src/priv_message.h"
#include "../../libs/src/priv_settings.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/mock.h"

#include <check.h>
#include <arpa/inet.h>
#include <assert.h>

#define PROMPT_SIZE 2
#define SOCKET_FD_INDEX 1

static Settings *settings = NULL;
static TCPClient *tcpClient = NULL;
static CommandTokens *cmdTokens = NULL;

static void initialize_test_suite(void) {

    settings = create_settings(CLIENT_OT_COUNT);
    initialize_client_settings();
}

static void cleanup_test_suite(void) {

    delete_settings(settings);
}

static void initialize_test(void) {

    tcpClient = create_client();
    cmdTokens = create_command_tokens(1);
}

static void cleanup_test(void) {

    delete_command_tokens(cmdTokens);
    delete_client(tcpClient);
}

static void set_message(LineEditor *lnEditor, const char *content) {

    RegMessage *message = create_reg_message(content);
    enqueue(lnEditor->cmdQueue, message);

    lnEditor->cursor = PROMPT_SIZE + strlen(content);
    lnEditor->charCount = strlen(content);
    lnEditor->cmdQueue->currentItem--;

    delete_message(message);
}

static void execute_command(TCPClient *tcpClient, CommandTokens *cmdTokens, CommandFunc commandFunc, const char *content) {

    SCREEN *screen = create_terminal();
    set_mock_stdscr(stdscr);

    WindowManager *windowManager = create_windows(0);

    // Scrollback *sb = create_scrollback(stdscr, 0);
    LineEditor *lnEditor = create_line_editor(NULL);

    set_message(lnEditor, content);
    parse_input(lnEditor, cmdTokens);

    commandFunc(windowManager, tcpClient, cmdTokens); 

    delete_line_editor(lnEditor);
    // delete_scrollback(sb);

    delete_windows(windowManager);

    endwin();
    delete_terminal(screen);

}

START_TEST(test_parse_input) {

    LineEditor *lnEditor = create_line_editor(NULL);
    CommandTokens *cmdTokens = create_command_tokens(1);

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

START_TEST(test_get_command_function) {

    ck_assert_ptr_eq(get_command_function(CONNECT), cmd_connect);
    ck_assert_ptr_ne(get_command_function(DISCONNECT), cmd_connect);
}
END_TEST


START_TEST(test_cmd_connect) {

    initialize_test();

    set_mock_fd(1);

    struct sockaddr_in sa;

    memset((struct sockaddr_in *) &sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(50100);
    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

    set_mock_sockaddr(&sa);

    execute_command(tcpClient, cmdTokens, get_command_function(CONNECT), "/CONNECT");
    char *message = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "NICK pmatkov");
    message = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "USER pmatkov 127.0.0.1 * :anonymous");
    reset_command_tokens(cmdTokens);

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_disconnect) {

    initialize_test();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    execute_command(tcpClient, cmdTokens, get_command_function(DISCONNECT), "/DISCONNECT see you tomorrow again");
    char *message = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "DISCONNECT :see you tomorrow again");
    reset_command_tokens(cmdTokens);

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_nick) {

    initialize_test();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    execute_command(tcpClient, cmdTokens, get_command_function(NICK), "/NICK john");
    char *message = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "NICK john");
    ck_assert_str_eq(get_char_option_value(OT_NICKNAME), "john");
    reset_command_tokens(cmdTokens);

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_user) {

    initialize_test();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    execute_command(tcpClient, cmdTokens, get_command_function(USER), "/USER jjones");
    ck_assert_str_eq(get_char_option_value(OT_USERNAME), "jjones");

    execute_command(tcpClient, cmdTokens, get_command_function(USER), "/USER jjones john jones");
    ck_assert_str_eq(get_char_option_value(OT_USERNAME), "jjones");
    ck_assert_str_eq(get_char_option_value(OT_REALNAME), "john jones");
    reset_command_tokens(cmdTokens);

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_join) {

    initialize_test();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    execute_command(tcpClient, cmdTokens, get_command_function(JOIN), "/JOIN #general");
    char *message = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "JOIN #general");
    reset_command_tokens(cmdTokens);

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_part) {

    initialize_test();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    execute_command(tcpClient, cmdTokens, get_command_function(PART), "/PART #general");
    char *message = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "PART #general");

    execute_command(tcpClient, cmdTokens, get_command_function(PART), "/PART #general see you tomorrow again");
    message = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "PART #general :see you tomorrow again");
    reset_command_tokens(cmdTokens);

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_privmsg) {

    initialize_test();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    execute_command(tcpClient, cmdTokens, get_command_function(PRIVMSG), "/PRIVMSG #general hello");
    char *message = remove_message_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "PRIVMSG #general :hello");

    reset_command_tokens(cmdTokens);

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_address) {

    initialize_test();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    execute_command(tcpClient, cmdTokens, get_command_function(ADDRESS), "/ADDRESS irc.freenode.net");
    ck_assert_str_eq(get_char_option_value(OT_SERVER_ADDRESS), "irc.freenode.net");

    reset_command_tokens(cmdTokens);

    cleanup_test();
}
END_TEST

START_TEST(test_cmd_port) {

    initialize_test();

    set_fd(tcpClient, SOCKET_FD_INDEX, 1);

    execute_command(tcpClient, cmdTokens, get_command_function(PORT), "/PORT 50100");
    ck_assert_int_eq(get_int_option_value(OT_SERVER_PORT), 50100);

    reset_command_tokens(cmdTokens);

    cleanup_test();
}
END_TEST



Suite* command_handler_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("CommandInfo handler");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_parse_input);
    tcase_add_test(tc_core, test_get_command_function);
    tcase_add_test(tc_core, test_cmd_connect);
    tcase_add_test(tc_core, test_cmd_disconnect);
    tcase_add_test(tc_core, test_cmd_nick);
    tcase_add_test(tc_core, test_cmd_user);
    tcase_add_test(tc_core, test_cmd_join);
    tcase_add_test(tc_core, test_cmd_part);
    tcase_add_test(tc_core, test_cmd_privmsg);
    tcase_add_test(tc_core, test_cmd_address);
    tcase_add_test(tc_core, test_cmd_port);

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

    initialize_test_suite();

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    cleanup_test_suite();

    return (number_failed == 0) ? 0 : 1;

}

#endif
