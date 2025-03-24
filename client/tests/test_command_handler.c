#ifndef TEST
#define TEST
#endif

#include "../src/config.h"
#include "../src/priv_command_handler.h"
#include "../src/priv_input_window.h"
#include "../src/priv_tcp_client.h"
#include "../../libs/src/priv_event.h"
#include "../../libs/src/priv_queue.h"
#include "../../libs/src/command.h"
#include "../../libs/src/message.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/print_utils.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/mock.h"

#include <check.h>
#include <arpa/inet.h>
#include <assert.h>

static SCREEN *screen = NULL;
static WindowManager *windowManager = NULL;
static EventManager *eventManager = NULL;
static Settings *settings = NULL;
static TCPClient *tcpClient = NULL;
static CommandTokens *cmdTokens = NULL;

#define CLIENT_FD 3

static void initialize_test_suite(void) {

    screen = create_terminal();
    windowManager = create_window_manager(0, 0);

    eventManager = create_event_manager(0);

    settings = create_settings(CLIENT_OT_COUNT);
    initialize_client_settings();

    tcpClient = create_client();
    cmdTokens = create_command_tokens(1);
}

static void cleanup_test_suite(void) {

    delete_command_tokens(cmdTokens);
    delete_client(tcpClient);
    delete_settings(settings);
    delete_event_manager(eventManager);
    delete_window_manager(windowManager);
    endwin();
    delete_terminal(screen);
}

static void set_le_content(LineEditor *lnEditor, const char *content) {

    Message *message = create_message(content, "", MSG_STANDARD, NORMAL_PRIORTY);
    enqueue(lnEditor->frontCmdQueue, message);

    lnEditor->cursor = PROMPT_SIZE + strlen(content);
    lnEditor->charCount = strlen(content);
    lnEditor->frontCmdQueue->currentIdx--;

    delete_message(message);
}

static void execute_command(CommandFunc commandFunc, const char *content) {

    set_le_content(windowManager->inputwin->lineEditor, content);
    parse_cli_input(windowManager->inputwin, cmdTokens);

    commandFunc(eventManager, windowManager, tcpClient, cmdTokens); 
}

START_TEST(test_parse_cli_input) {

    char *content = "/PRIVMSG #general hello";

    set_le_content(windowManager->inputwin->lineEditor, content);
    parse_cli_input(windowManager->inputwin, cmdTokens);

    ck_assert_int_eq(cmdTokens->argCount, 2);
    ck_assert_str_eq(cmdTokens->command, "PRIVMSG");
    ck_assert_str_eq(cmdTokens->args[0], "#general");
    ck_assert_str_eq(cmdTokens->args[1], "hello");

    content = "/CONNECT";

    set_le_content(windowManager->inputwin->lineEditor, content);
    parse_cli_input(windowManager->inputwin, cmdTokens);

    ck_assert_int_eq(cmdTokens->argCount, 0);
    ck_assert_str_eq(cmdTokens->command, "CONNECT");

    content = "/PART #general see you tomorrow mates";

    set_le_content(windowManager->inputwin->lineEditor, content);
    parse_cli_input(windowManager->inputwin, cmdTokens);

    ck_assert_int_eq(cmdTokens->argCount, 4);
    ck_assert_str_eq(cmdTokens->command, "PART");
    ck_assert_str_eq(cmdTokens->args[0], "#general");
    ck_assert_str_eq(cmdTokens->args[1], "see");
    ck_assert_str_eq(cmdTokens->args[2], "you");
    ck_assert_str_eq(cmdTokens->args[3], "tomorrow mates");

    reset_command_tokens(cmdTokens);

}
END_TEST

START_TEST(test_get_command_function) {

    ck_assert_ptr_eq(get_command_function(CONNECT), cmd_connect);
    ck_assert_ptr_ne(get_command_function(DISCONNECT), cmd_connect);
}
END_TEST


START_TEST(test_cmd_connect) {

    set_mock_fd(1);

    struct sockaddr_in sa;

    memset((struct sockaddr_in *) &sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(50100);
    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

    set_mock_sockaddr(&sa);

    execute_command(get_command_function(CONNECT), "/CONNECT");
    char *message = dequeue_from_client_queue(tcpClient);

    const char *tokens[2] = {"NICK", get_char_option_value(OT_NICKNAME)};  
    char buffer[MAX_CHARS + 1] = {'\0'};

    concat_tokens(buffer, MAX_CHARS, tokens, 2, " ");

    ck_assert_str_eq(message, buffer);

    message = dequeue_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "USER pmatkov 127.0.0.1 * :anonymous");

}
END_TEST

START_TEST(test_cmd_disconnect) {

    set_client_fd(tcpClient, CLIENT_FD);
    set_client_state_type(tcpClient, CONNECTED);
    
    execute_command(get_command_function(DISCONNECT), "/DISCONNECT see you tomorrow again");
    char *message = dequeue_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "QUIT :see you tomorrow again");

}
END_TEST

START_TEST(test_cmd_nick) {

    set_client_fd(tcpClient, CLIENT_FD);
    set_client_state_type(tcpClient, CONNECTED);

    execute_command(get_command_function(NICK), "/NICK john");
    char *message = dequeue_from_client_queue(tcpClient);

    ck_assert_str_eq(message, "NICK john");
    ck_assert_str_eq(get_char_option_value(OT_NICKNAME), "john");

}
END_TEST

START_TEST(test_cmd_user) {

    set_client_fd(tcpClient, CLIENT_FD);

    execute_command(get_command_function(USER), "/USER jjones");
    ck_assert_str_eq(get_char_option_value(OT_USERNAME), "jjones");

    execute_command(get_command_function(USER), "/USER jjones john jones");
    ck_assert_str_eq(get_char_option_value(OT_USERNAME), "jjones");
    ck_assert_str_eq(get_char_option_value(OT_REALNAME), "john jones");

}
END_TEST

START_TEST(test_cmd_join) {

    set_client_fd(tcpClient, CLIENT_FD);
    set_client_state_type(tcpClient, REGISTERED);

    execute_command(get_command_function(JOIN), "/JOIN #general");
    char *message = dequeue_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "JOIN #general");

}
END_TEST

START_TEST(test_cmd_part) {

    set_client_fd(tcpClient, CLIENT_FD);
    set_client_state_type(tcpClient, IN_CHANNEL);

    execute_command(get_command_function(PART), "/PART #general");
    char *message = dequeue_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "PART #general");

    set_client_state_type(tcpClient, IN_CHANNEL);

    execute_command(get_command_function(PART), "/PART #general see you tomorrow again");
    message = dequeue_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "PART #general :see you tomorrow again");

}
END_TEST

START_TEST(test_cmd_privmsg) {

    set_client_fd(tcpClient, CLIENT_FD);
    set_client_state_type(tcpClient, REGISTERED);

    execute_command(get_command_function(PRIVMSG), "/PRIVMSG #general hello");
    char *message = dequeue_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "PRIVMSG #general :hello");

}
END_TEST

START_TEST(test_cmd_address) {

    set_client_fd(tcpClient, CLIENT_FD);

    execute_command(get_command_function(ADDRESS), "/ADDRESS irc.freenode.net");
    ck_assert_str_eq(get_char_option_value(OT_SERVER_ADDRESS), "irc.freenode.net");

}
END_TEST

START_TEST(test_cmd_port) {

    set_client_fd(tcpClient, CLIENT_FD);

    execute_command(get_command_function(PORT), "/PORT 50100");
    ck_assert_int_eq(get_int_option_value(OT_SERVER_PORT), 50100);

}
END_TEST

START_TEST(test_whois) {

    set_client_fd(tcpClient, CLIENT_FD);
    set_client_state_type(tcpClient, REGISTERED);

    execute_command(get_command_function(WHOIS), "/WHOIS john");
    char *message = dequeue_from_client_queue(tcpClient);
    ck_assert_str_eq(message, "WHOIS john");

}
END_TEST

START_TEST(test_set_connection_params) {

    char ipv4address[INET_ADDRSTRLEN] = {'\0'};
    int port = 0;

    reset_command_tokens(cmdTokens);
    set_connection_params(cmdTokens, ipv4address, &port);

    ck_assert_str_eq(ipv4address, "127.0.0.1");
    ck_assert_int_eq(port, 50100);
}
END_TEST


Suite* command_handler_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Command handler");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_parse_cli_input);
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
    tcase_add_test(tc_core, test_whois);
    tcase_add_test(tc_core, test_set_connection_params);

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
