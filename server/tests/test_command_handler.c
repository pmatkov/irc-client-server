#ifndef TEST
#define TEST
#endif

#include "../src/priv_command_handler.h"
#include "../src/priv_tcp_server.h"
#include "../src/priv_session.h"
#include "../src/config.h"
#include "../../libs/src/command.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/linked_list.h"
#include "../../libs/src/network_utils.h"
#include "../../libs/src/string_utils.h"

#include <check.h>

#define CLIENT_FD 3
#define CLIENT_FD_IDX 0
#define CLIENT_IDENTIFIER "irc1.client.com"

static TCPServer *server = NULL;
static Settings *settings = NULL;

static void initialize_test_suite(void) {

    settings = create_settings(SERVER_OT_COUNT);
    initialize_server_settings();
}

static void cleanup_test_suite(void) {

    delete_settings(settings);
}

static void initialize_test(void) {

    server = create_server(0);

    set_client_data(server, CLIENT_FD_IDX, CLIENT_FD, CLIENT_IDENTIFIER, HOSTNAME, 50101);
    set_client_state_type(server->clients[CLIENT_FD_IDX], CONNECTED);;
}

static void cleanup_test(void) {
    
    unset_client_data(server, CLIENT_FD_IDX);
    set_client_state_type(server->clients[CLIENT_FD_IDX], DISCONNECTED);

    delete_server(server);
}

static void execute_command(Client *client, CommandFunc cmdFunction, const char *content) {

    CommandTokens *cmdTokens = create_command_tokens(1);

    parse_message(content, cmdTokens);
    cmdFunction(NULL, server, client, cmdTokens); 

    delete_command_tokens(cmdTokens);
}

static void initialize_user_session(User **user, UserChannels **userChannels, const char *nickname, const char *username, const char *hostname, const char *realname) {

    *user = create_user(0, nickname, username, hostname, realname);
    add_user_to_hash_table(server->session, *user);

    *userChannels = create_user_channels(*user);
    add_user_channels(server->session, *userChannels);
}

static void cleanup_user_session(TCPServer *tcpServer, User *user, UserChannels *userChannels) {

    ReadyList *readyList = get_ready_list(get_session(tcpServer));

    remove_user_from_ready_list(get_ready_users(readyList), user);
    remove_user_channels(get_session(tcpServer), userChannels);
    remove_user_from_hash_table(get_session(tcpServer), user);
}

static void initialize_channel_session(const char *name, const char *topic, Channel **channel, ChannelUsers **channelUsers) {

    *channel = create_channel(name, topic, TEMPORARY, MAX_USERS_PER_CHANNEL);
    add_channel_to_hash_table(server->session, *channel);
    *channelUsers = create_channel_users(*channel);
    add_channel_users(server->session, *channelUsers);

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

START_TEST(test_get_command_function) {

    ck_assert_ptr_eq(get_command_function(QUIT), cmd_quit);
    ck_assert_ptr_ne(get_command_function(JOIN), cmd_quit);

}
END_TEST

START_TEST(test_parse_message) {

    CommandTokens *cmdTokens = create_command_tokens(1);

    parse_message("PRIVMSG #general :Hello everybody", cmdTokens);

    ck_assert_str_eq(get_command(cmdTokens), "PRIVMSG");
    ck_assert_int_eq(get_command_argument_count(cmdTokens), 2);
    ck_assert_str_eq(get_command_argument(cmdTokens, 0), "#general");
    ck_assert_str_eq(get_command_argument(cmdTokens, 1), ":Hello everybody");

    delete_command_tokens(cmdTokens);
}
END_TEST

START_TEST(test_cmd_nick) {

    initialize_test();

    char buffer[MAX_CHARS + 1] = {'\0'};
    int fd;
    const char *content = NULL;

    set_client_nickname(server->clients[CLIENT_FD_IDX], "john");

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(NICK), "NICK");
    const char *message = dequeue_from_server_queue(server);

    decode_message(buffer, ARRAY_SIZE(buffer), &fd, &content, message);
    ck_assert_int_eq(fd, CLIENT_FD);
    ck_assert_str_eq(content, ":irc.server.com 431 * :No nickname given");

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(NICK), "NICK john$");
    message = dequeue_from_server_queue(server);

    decode_message(buffer, ARRAY_SIZE(buffer), &fd, &content, message);
    ck_assert_int_eq(fd, CLIENT_FD);
    ck_assert_str_eq(content, ":irc.server.com 432 john$ :Erroneous nickname");
    
    User *user = NULL;
    UserChannels *userChannels = NULL;
    initialize_user_session(&user, &userChannels, "john", NULL, NULL, NULL);

    Channel *channel1 = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    add_channel_to_user_channels(userChannels, channel1);
    Channel *channel2 = create_channel("#linux", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    add_channel_to_user_channels(userChannels, channel2); 

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(NICK), "NICK john");
    message = dequeue_from_server_queue(server);

    decode_message(buffer, ARRAY_SIZE(buffer), &fd, &content, message);
    ck_assert_int_eq(fd, CLIENT_FD);
    ck_assert_str_eq(content, ":irc.server.com 433 john :Nickname is already in use");
    
    set_client_state_type(server->clients[CLIENT_FD_IDX], REGISTERED);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(NICK), "NICK john707");
    content = dequeue_from_channel_queue(channel1);
    ck_assert_str_eq(content, ":john!@ NICK john707");
    ck_assert_int_eq(get_list_count(get_ready_channels(get_ready_list(server->session))), 2);
    
    User *newUser = find_user_in_hash_table(server->session, "john707");
    ck_assert_int_eq(get_total_items(server->session->users), 1);
    ck_assert_ptr_ne(user, newUser);

    delete_channel(channel1);
    delete_channel(channel2);

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_user) {

    initialize_test();

    char buffer[MAX_CHARS + 1] = {'\0'};
    int fd;
    const char *content = NULL;

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(USER), "USER john 127.0.0.1 * :john jones");
    const char *message = dequeue_from_server_queue(server);

    decode_message(buffer, ARRAY_SIZE(buffer), &fd, &content, message);
    ck_assert_int_eq(fd, CLIENT_FD);
    ck_assert_str_eq(content, ":irc.server.com 451 * :You have not registered");
    
    set_client_nickname(server->clients[CLIENT_FD_IDX], "john");
    set_client_state_type(server->clients[CLIENT_FD_IDX], REGISTERED);

    User *user = NULL;
    UserChannels *userChannels = NULL;
    initialize_user_session(&user, &userChannels, "john", NULL, NULL, NULL);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(USER), "USER john 127.0.0.1 * :john jones");
    content = dequeue_from_user_queue(user);
    ck_assert_str_eq(content, ":irc.server.com 462 john :Already registered");
    
    set_client_state_type(server->clients[CLIENT_FD_IDX], START_REGISTRATION);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(USER), "USER john");
    message = dequeue_from_server_queue(server);
    decode_message(buffer, ARRAY_SIZE(buffer), &fd, &content, message);
    ck_assert_int_eq(fd, CLIENT_FD);
    ck_assert_str_eq(content, ":irc.server.com 461 john USER :Not enough parameters");
    
    cleanup_user_session(server, user, userChannels);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(USER), "USER john 127.0.0.1 * :john jones");

    user = find_user_in_hash_table(server->session, "john");
    content = dequeue_from_user_queue(user);
    ck_assert_str_eq(content, ":irc.server.com 001 john :Welcome to the IRC Network");

    ck_assert_int_eq(get_client_state_type(server->clients[CLIENT_FD_IDX]), REGISTERED);

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_join) {

    initialize_test();

    set_client_nickname(server->clients[CLIENT_FD_IDX], "john");
    set_client_state_type(server->clients[CLIENT_FD_IDX], CONNECTED);

    char buffer[MAX_CHARS + 1] = {'\0'};
    int fd;
    const char *content = NULL;

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(JOIN), "JOIN #general");
    const char *message = dequeue_from_server_queue(server);
    
    decode_message(buffer, ARRAY_SIZE(buffer), &fd, &content, message);
    ck_assert_int_eq(fd, CLIENT_FD);
    ck_assert_str_eq(content, ":irc.server.com 451 * :You have not registered");
    
    set_client_state_type(server->clients[CLIENT_FD_IDX], REGISTERED);

    User *user = NULL;
    UserChannels *userChannels = NULL;
    initialize_user_session(&user, &userChannels, "john", NULL, NULL, NULL);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(JOIN), "JOIN");
    content = dequeue_from_user_queue(user);
    ck_assert_str_eq(content, ":irc.server.com 461 john JOIN :Not enough parameters");
    
    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(JOIN), "JOIN $linux");
    content = dequeue_from_user_queue(user);
    ck_assert_str_eq(content, ":irc.server.com 479 john $linux :Illegal channel name");  

    Channel *channel = NULL;
    ChannelUsers *channelUsers = NULL;
    initialize_channel_session("#general", "football weekend", &channel, &channelUsers);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(JOIN), "JOIN #general");
    content = dequeue_from_channel_queue(channel);
    ck_assert_str_eq(content, ":john!@ JOIN #general");

    content = dequeue_from_user_queue(user);
    ck_assert_str_eq(content, ":irc.server.com 332 john #general :football weekend");
    
    content = dequeue_from_user_queue(user);
    ck_assert_str_eq(content, ":irc.server.com 353 john #general :john");
      
    content = dequeue_from_user_queue(user);
    ck_assert_str_eq(content, ":irc.server.com 366 john #general :End of /NAMES list");
      
    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(JOIN), "JOIN #linux");
    Channel *newChannel = find_channel_in_hash_table(server->session, "#linux");
    content = dequeue_from_channel_queue(newChannel);
    ck_assert_str_eq(content, ":john!@ JOIN #linux");
    content = dequeue_from_user_queue(user);
    ck_assert_str_eq(content, ":irc.server.com 331 john #linux :No topic is set");
      
    cleanup_test();

}
END_TEST

START_TEST(test_cmd_part) {

    initialize_test();

    set_client_nickname(server->clients[CLIENT_FD_IDX], "john");
    set_client_state_type(server->clients[CLIENT_FD_IDX], IN_CHANNEL);

    set_client_data(server, CLIENT_FD_IDX + 1, CLIENT_FD + 1, "irc2.client.com", HOSTNAME, 50102);

    set_client_nickname(server->clients[CLIENT_FD_IDX + 1], "mark");
    set_client_state_type(server->clients[CLIENT_FD_IDX + 1], IN_CHANNEL);

    User *user1 = NULL, *user2 = NULL;
    UserChannels *userChannels1 = NULL, *userChannels2 = NULL;
    initialize_user_session(&user1, &userChannels1, "john", NULL, NULL, NULL);
    initialize_user_session(&user2, &userChannels2, "mark", NULL, NULL, NULL);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(PART), "PART #general");
    char *content = dequeue_from_user_queue(user1);
    ck_assert_str_eq(content, ":irc.server.com 403 john #general :No such channel");

    Channel *channel = NULL;
    ChannelUsers *channelUsers = NULL;
    initialize_channel_session("#general", NULL, &channel, &channelUsers);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(PART), "PART #general");
    content = dequeue_from_user_queue(user1);
    ck_assert_str_eq(content, ":irc.server.com 442 john #general :You're not on that channel");

    add_user_to_channel_users(channelUsers, user1);
    add_channel_to_user_channels(userChannels1, channel);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(PART), "PART #general");
    ck_assert_int_eq(get_total_items(server->session->channels), 0);
    ck_assert_int_eq(server->session->channelUsersLL->count, 0);
    ck_assert_int_eq(userChannels1->count, 0);

    set_client_state_type(server->clients[CLIENT_FD_IDX], IN_CHANNEL);

    initialize_channel_session("#general", NULL, &channel, &channelUsers);

    add_user_to_channel_users(channelUsers, user1);
    add_channel_to_user_channels(userChannels1, channel);
    add_user_to_channel_users(channelUsers, user2);
    add_channel_to_user_channels(userChannels2, channel);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(PART), "PART #general :bye");
    ck_assert_int_eq(get_total_items(server->session->channels), 1);
    content = dequeue_from_channel_queue(channel);
    ck_assert_str_eq(content, ":john!@ PART #general :bye");

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_privmsg) {

    initialize_test();

    set_client_nickname(server->clients[CLIENT_FD_IDX], "john");
    set_client_state_type(server->clients[CLIENT_FD_IDX], REGISTERED);

    set_client_data(server, CLIENT_FD_IDX + 1, CLIENT_FD + 1, "irc2.client.com", HOSTNAME, 50102);

    set_client_nickname(server->clients[CLIENT_FD_IDX + 1], "mark");
    set_client_state_type(server->clients[CLIENT_FD_IDX + 1], REGISTERED);

    User *user1 = NULL, *user2 = NULL;
    UserChannels *userChannels1 = NULL, *userChannels2 = NULL;
    initialize_user_session(&user1, &userChannels1, "john", NULL, NULL, NULL);
    initialize_user_session(&user2, &userChannels2, "mark", NULL, NULL, NULL);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(PRIVMSG), "PRIVMSG steve :hello");
    char *content = dequeue_from_user_queue(user1);
    ck_assert_str_eq(content, ":irc.server.com 401 john steve :No such nick");

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(PRIVMSG), "PRIVMSG mark :hello");
    content = dequeue_from_user_queue(user2);
    ck_assert_str_eq(content, ":john!@ PRIVMSG mark :hello");
    
    Channel *channel = NULL;
    ChannelUsers *channelUsers = NULL;
    initialize_channel_session("#general", NULL, &channel, &channelUsers);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(PRIVMSG), "PRIVMSG #linux :hello");
    content = dequeue_from_user_queue(user1);
    ck_assert_str_eq(content, ":irc.server.com 403 john #linux :No such channel");
    
    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(PRIVMSG), "PRIVMSG #general :hello");
    content = dequeue_from_user_queue(user1);
    ck_assert_str_eq(content, ":irc.server.com 442 john #general :You're not on that channel");
    
    add_user_to_channel_users(channelUsers, user1);
    add_channel_to_user_channels(userChannels1, channel);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(PRIVMSG), "PRIVMSG #general :hello");
    content = dequeue_from_channel_queue(channel);
    ck_assert_str_eq(content, ":john!@ PRIVMSG #general :hello");

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_whois) {

    initialize_test();

    set_client_nickname(server->clients[CLIENT_FD_IDX], "john");
    set_client_state_type(server->clients[CLIENT_FD_IDX], REGISTERED);

    set_client_data(server, CLIENT_FD_IDX + 1, CLIENT_FD + 1, "irc2.client.com", HOSTNAME, 50102);

    set_client_nickname(server->clients[CLIENT_FD_IDX + 1], "mark");
    set_client_state_type(server->clients[CLIENT_FD_IDX + 1], REGISTERED);

    User *user1 = NULL, *user2 = NULL;
    UserChannels *userChannels1 = NULL, *userChannels2 = NULL;
    initialize_user_session(&user1, &userChannels1, "john", NULL, NULL, NULL);
    initialize_user_session(&user2, &userChannels2, "mark", "mjohnson", "irc2.client.com", "Mark Johnson");

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(WHOIS), "WHOIS steve");
    const char *content = dequeue_from_user_queue(user1);
    ck_assert_str_eq(content, ":irc.server.com 401 john steve :No such nick");
    
    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(WHOIS), "WHOIS mark");
    content = dequeue_from_user_queue(user1);
    ck_assert_str_eq(content, ":irc.server.com 311 john mark mjohnson irc2.client.com :Mark Johnson");
    
    cleanup_test();

}
END_TEST

START_TEST(test_cmd_quit) {

    initialize_test();

    set_client_nickname(server->clients[CLIENT_FD_IDX], "john");
    set_client_state_type(server->clients[CLIENT_FD_IDX], REGISTERED);

    set_client_data(server, CLIENT_FD_IDX + 1, CLIENT_FD + 1, "irc2.client.com", HOSTNAME, 50102);

    set_client_nickname(server->clients[CLIENT_FD_IDX + 1], "mark");
    set_client_state_type(server->clients[CLIENT_FD_IDX + 1], REGISTERED);

    User *user1 = NULL, *user2 = NULL;
    UserChannels *userChannels1 = NULL, *userChannels2 = NULL;
    initialize_user_session(&user1, &userChannels1, "john", NULL, NULL, NULL);
    initialize_user_session(&user2, &userChannels2, "mark", NULL, NULL, NULL);

    ck_assert_int_eq(get_total_items(server->session->users), 2);

    Channel *channel = NULL;
    ChannelUsers *channelUsers = NULL;
    initialize_channel_session("#general", NULL, &channel, &channelUsers);

    add_user_to_channel_users(channelUsers, user1);
    add_channel_to_user_channels(userChannels1, channel);
    add_user_to_channel_users(channelUsers, user2);
    add_channel_to_user_channels(userChannels2, channel);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(QUIT), "QUIT :bye");
    const char *content = dequeue_from_channel_queue(channel);
    ck_assert_str_eq(content, ":john!@ QUIT :bye");
    
    ck_assert_int_eq(get_total_items(server->session->users), 1);

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_unknown) {

    initialize_test();

    set_client_nickname(server->clients[CLIENT_FD_IDX], "john");
    set_client_state_type(server->clients[CLIENT_FD_IDX], REGISTERED);

    User *user = NULL;
    UserChannels *userChannels = NULL;
    initialize_user_session(&user, &userChannels, "john", NULL, NULL, NULL);

    execute_command(server->clients[CLIENT_FD_IDX], get_command_function(UNKNOWN_COMMAND_TYPE), "TEST");
    const char *content = dequeue_from_user_queue(user);
    ck_assert_str_eq(content, ":irc.server.com 421 TEST :Unknown command");

    cleanup_test();

}
END_TEST

Suite* command_handler_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Command handler");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_get_command_function);
    tcase_add_test(tc_core, test_parse_message);
    tcase_add_test(tc_core, test_cmd_nick);
    tcase_add_test(tc_core, test_cmd_user);
    tcase_add_test(tc_core, test_cmd_join);
    tcase_add_test(tc_core, test_cmd_part);
    tcase_add_test(tc_core, test_cmd_privmsg);
    tcase_add_test(tc_core, test_cmd_whois);
    tcase_add_test(tc_core, test_cmd_quit);
    tcase_add_test(tc_core, test_cmd_unknown);

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