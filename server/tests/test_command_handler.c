#ifndef TEST
#define TEST
#endif

#include "../src/priv_command_handler.h"
#include "../src/priv_tcp_server.h"
#include "../src/priv_session.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/message.h"
#include "../../libs/src/linked_list.h"
#include "../../libs/src/string_utils.h"

#include <check.h>

#define DEFAULT_FD 4
#define DEFAULT_FD_IDX 0

static TCPServer *server;

static void execute_command(Client *client, CommandFunc cmdFunction, const char *content) {

    CommandTokens *cmdTokens = create_command_tokens(1);

    set_client_inbuffer(client, content);
    parse_message(server, client, cmdTokens);
    cmdFunction(server, client, cmdTokens); 

    delete_command_tokens(cmdTokens);
}

static void initalize_user_session(const char *name, User **user, UserChannels **userChannels) {

    *user = create_user(name, NULL, NULL, NULL, 0);
    add_user_to_hash_table(server->session, *user);
    *userChannels = create_user_channels(*user);
    add_user_channels(server->session, *userChannels);

}

static void initalize_channel_session(const char *name, const char *topic, Channel **channel, ChannelUsers **channelUsers) {

    *channel = create_channel(name, topic, TEMPORARY, MAX_USERS_PER_CHANNEL);
    add_channel_to_hash_table(server->session, *channel);
    *channelUsers = create_channel_users(*channel);
    add_channel_users(server->session, *channelUsers);

}

static void initialize_test(void) {

    server = create_server(0, "irc.server.com");
    assign_fd(server, DEFAULT_FD_IDX, DEFAULT_FD);
    assign_client_fd(server, DEFAULT_FD_IDX, DEFAULT_FD);
}

static void cleanup_test(void) {

    delete_server(server);
}

START_TEST(test_get_command_function) {

    ck_assert_ptr_eq(get_command_function(QUIT), cmd_quit);
    ck_assert_ptr_ne(get_command_function(JOIN), cmd_quit);

}
END_TEST

// START_TEST(test_create_user_info) {

//     char userInfo[MAX_CHARS + 1] = {'\0'};

//     create_user_info(userInfo, MAX_CHARS, "jdoe", "john", "irc.client.com");

//     ck_assert_str_eq(userInfo, "jdoe!john@irc.client.com");

// }
// END_TEST

START_TEST(test_parse_message) {

    initialize_test();

    set_client_nickname(server->clients[DEFAULT_FD_IDX], "john");
    set_client_inbuffer(server->clients[DEFAULT_FD_IDX], "PRIVMSG #general :Hello");

    CommandTokens *cmdTokens = create_command_tokens(1);

    parse_message(server, server->clients[DEFAULT_FD_IDX], cmdTokens);

    ck_assert_str_eq(cmdTokens->command, "PRIVMSG");
    ck_assert_int_eq(cmdTokens->argCount, 2);
    ck_assert_str_eq(cmdTokens->args[0], "#general");
    ck_assert_str_eq(cmdTokens->args[1], ":Hello");

    delete_command_tokens(cmdTokens);
    cleanup_test();

}
END_TEST

START_TEST(test_cmd_nick) {

    initialize_test();

    char fdStr[MAX_DIGITS + 1] = {'\0'};
    uint_to_str(fdStr, sizeof(fdStr), *get_client_fd(server->clients[DEFAULT_FD_IDX]));

    set_client_nickname(server->clients[DEFAULT_FD_IDX], "john");

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(NICK), "NICK");
    ExtMessage *extMessage = dequeue_from_server_queue(server);
    ck_assert_str_eq(get_ext_message_recipient(extMessage), fdStr);
    ck_assert_str_eq(get_ext_message_content(extMessage), ":irc.server.com 431 * :No nickname given");

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(NICK), "NICK john$");
    extMessage = dequeue_from_server_queue(server);
    ck_assert_str_eq(get_ext_message_recipient(extMessage), fdStr);
    ck_assert_str_eq(get_ext_message_content(extMessage), ":irc.server.com 432 john$ :Erroneous nickname");
    
    User *user = NULL;
    UserChannels *userChannels = NULL;
    initalize_user_session("john", &user, &userChannels);

    Channel *channel1 = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    add_channel_to_user_channels(userChannels, channel1);
    Channel *channel2 = create_channel("#linux", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    add_channel_to_user_channels(userChannels, channel2); 

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(NICK), "NICK john");
    extMessage = dequeue_from_server_queue(server);
    ck_assert_str_eq(get_ext_message_recipient(extMessage), fdStr);
    ck_assert_str_eq(get_ext_message_content(extMessage), ":irc.server.com 433 john :Nickname is already in use");
    
    set_client_registered(server->clients[DEFAULT_FD_IDX], 1);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(NICK), "NICK john707");
    char *message = dequeue_from_channel_queue(channel1);
    ck_assert_str_eq(message, ":john!@ NICK john707");
    ck_assert_int_eq(get_list_count(get_ready_channels(get_ready_list(server->session))), 2);
    
    User *newUser = find_user_in_hash_table(server->session, "john707");
    ck_assert_int_eq(server->session->usersCount, 1);
    ck_assert_ptr_ne(user, newUser);

    delete_channel(channel1);
    delete_channel(channel2);

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_user) {

    initialize_test();

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(USER), "USER john 127.0.0.1 * :john jones");
    ExtMessage *extMessage = dequeue_from_server_queue(server);
    ck_assert_str_eq(get_ext_message_content(extMessage), ":irc.server.com 451 * :You have not registered");
    
    set_client_nickname(server->clients[DEFAULT_FD_IDX], "john");
    set_client_registered(server->clients[DEFAULT_FD_IDX], 1);

    User *user = create_user("john", NULL, NULL, NULL, 0);
    add_user_to_hash_table(server->session, user);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(USER), "USER john 127.0.0.1 * :john jones");
    char *message = dequeue_from_user_queue(user);
    ck_assert_str_eq(message, ":irc.server.com 462 john :Already registered");
    
    set_client_registered(server->clients[DEFAULT_FD_IDX], 0);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(USER), "USER john");
    extMessage = dequeue_from_server_queue(server);
    ck_assert_str_eq(get_ext_message_content(extMessage), ":irc.server.com 461 john USER :Not enough parameters");
    
    remove_user_from_hash_table(server->session, user);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(USER), "USER john 127.0.0.1 * :john jones");

    user = find_user_in_hash_table(server->session, "john");
    message = dequeue_from_user_queue(user);
    ck_assert_str_eq(message, ":irc.server.com 001 john :Welcome to the IRC Network");

    ck_assert_int_eq(is_client_registered(server->clients[DEFAULT_FD_IDX]), 1);

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_join) {

    initialize_test();

    set_client_nickname(server->clients[DEFAULT_FD_IDX], "john");
    set_client_registered(server->clients[DEFAULT_FD_IDX], 0);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(JOIN), "JOIN #general");
    ExtMessage *extMessage = dequeue_from_server_queue(server);
    ck_assert_str_eq(get_ext_message_content(extMessage), ":irc.server.com 451 * :You have not registered");
    
    set_client_registered(server->clients[DEFAULT_FD_IDX], 1);

    User *user = NULL;
    UserChannels *userChannels = NULL;
    initalize_user_session("john", &user, &userChannels);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(JOIN), "JOIN");
    char *message = dequeue_from_user_queue(user);
    ck_assert_str_eq(message, ":irc.server.com 461 john JOIN :Not enough parameters");
    
    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(JOIN), "JOIN $linux");
    message = dequeue_from_user_queue(user);
    ck_assert_str_eq(message, ":irc.server.com 479 john $linux :Illegal channel name");  

    Channel *channel = NULL;
    ChannelUsers *channelUsers = NULL;
    initalize_channel_session("#general", "football weekend", &channel, &channelUsers);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(JOIN), "JOIN #general");
    message = dequeue_from_channel_queue(channel);
    ck_assert_str_eq(message, ":john!@ JOIN #general");

    message = dequeue_from_user_queue(user);
    ck_assert_str_eq(message, ":irc.server.com 332 john #general :football weekend");
    
    message = dequeue_from_user_queue(user);
    ck_assert_str_eq(message, ":irc.server.com 353 john #general :john");
      
    message = dequeue_from_user_queue(user);
    ck_assert_str_eq(message, ":irc.server.com 366 john #general :End of /NAMES list");
      
    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(JOIN), "JOIN #linux");
    Channel *newChannel = find_channel_in_hash_table(server->session, "#linux");
    message = dequeue_from_channel_queue(newChannel);
    ck_assert_str_eq(message, ":john!@ JOIN #linux");
    message = dequeue_from_user_queue(user);
    ck_assert_str_eq(message, ":irc.server.com 331 john #linux :No topic is set");
      
    cleanup_test();

}
END_TEST

START_TEST(test_cmd_part) {

    initialize_test();

    set_client_nickname(server->clients[DEFAULT_FD_IDX], "john");
    set_client_registered(server->clients[DEFAULT_FD_IDX], 1);

    assign_fd(server, DEFAULT_FD_IDX + 1, DEFAULT_FD + 1);

    set_client_nickname(server->clients[DEFAULT_FD_IDX + 1], "mark");
    set_client_registered(server->clients[DEFAULT_FD_IDX + 1], 1);

    User *user1 = NULL, *user2 = NULL;
    UserChannels *userChannels1 = NULL, *userChannels2 = NULL;
    initalize_user_session("john", &user1, &userChannels1);
    initalize_user_session("mark", &user2, &userChannels2);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(PART), "PART #general");
    char *message = dequeue_from_user_queue(user1);
    ck_assert_str_eq(message, ":irc.server.com 403 john #general :No such channel");

    Channel *channel = NULL;
    ChannelUsers *channelUsers = NULL;
    initalize_channel_session("#general", NULL, &channel, &channelUsers);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(PART), "PART #general");
    message = dequeue_from_user_queue(user1);
    ck_assert_str_eq(message, ":irc.server.com 442 john #general :You're not on that channel");

    add_user_to_channel_users(channelUsers, user1);
    add_channel_to_user_channels(userChannels1, channel);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(PART), "PART #general");
    ck_assert_int_eq(server->session->channelsCount, 0);
    ck_assert_int_eq(server->session->channelUsersLL->count, 0);
    ck_assert_int_eq(userChannels1->count, 0);

    initalize_channel_session("#general", NULL, &channel, &channelUsers);

    add_user_to_channel_users(channelUsers, user1);
    add_channel_to_user_channels(userChannels1, channel);
    add_user_to_channel_users(channelUsers, user2);
    add_channel_to_user_channels(userChannels2, channel);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(PART), "PART #general :bye");
    message = dequeue_from_channel_queue(channel);
    ck_assert_str_eq(message, ":john!@ PART #general :bye");

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_privmsg) {

    initialize_test();

    set_client_nickname(server->clients[DEFAULT_FD_IDX], "john");
    set_client_registered(server->clients[DEFAULT_FD_IDX], 1);

    assign_fd(server, DEFAULT_FD_IDX + 1, DEFAULT_FD + 1);

    set_client_nickname(server->clients[DEFAULT_FD_IDX + 1], "mark");
    set_client_registered(server->clients[DEFAULT_FD_IDX + 1], 1);

    User *user1 = NULL, *user2 = NULL;
    UserChannels *userChannels1 = NULL, *userChannels2 = NULL;
    initalize_user_session("john", &user1, &userChannels1);
    initalize_user_session("mark", &user2, &userChannels2);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(PRIVMSG), "PRIVMSG steve :hello");
    char *message = dequeue_from_user_queue(user1);
    ck_assert_str_eq(message, ":irc.server.com 401 john steve :No such nick");

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(PRIVMSG), "PRIVMSG mark :hello");
    message = dequeue_from_user_queue(user2);
    ck_assert_str_eq(message, ":john!@ PRIVMSG mark :hello");
    
    Channel *channel = NULL;
    ChannelUsers *channelUsers = NULL;
    initalize_channel_session("#general", NULL, &channel, &channelUsers);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(PRIVMSG), "PRIVMSG #linux :hello");
    message = dequeue_from_user_queue(user1);
    ck_assert_str_eq(message, ":irc.server.com 403 john #linux :No such channel");
    
    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(PRIVMSG), "PRIVMSG #general :hello");
    message = dequeue_from_user_queue(user1);
    ck_assert_str_eq(message, ":irc.server.com 442 john #general :You're not on that channel");
    
    add_user_to_channel_users(channelUsers, user1);
    add_channel_to_user_channels(userChannels1, channel);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(PRIVMSG), "PRIVMSG #general :hello");
    message = dequeue_from_channel_queue(channel);
    ck_assert_str_eq(message, ":john!@ PRIVMSG #general :hello");

    cleanup_test();

}
END_TEST

START_TEST(test_cmd_quit) {

    initialize_test();

    set_client_nickname(server->clients[DEFAULT_FD_IDX], "john");
    set_client_registered(server->clients[DEFAULT_FD_IDX], 1);

    assign_fd(server, DEFAULT_FD_IDX + 1, DEFAULT_FD + 1);

    set_client_nickname(server->clients[DEFAULT_FD_IDX + 1], "mark");
    set_client_registered(server->clients[DEFAULT_FD_IDX + 1], 1);

    User *user1 = NULL, *user2 = NULL;
    UserChannels *userChannels1 = NULL, *userChannels2 = NULL;
    initalize_user_session("john", &user1, &userChannels1);
    initalize_user_session("mark", &user2, &userChannels2);

    ck_assert_int_eq(server->session->usersCount, 2);

    Channel *channel = NULL;
    ChannelUsers *channelUsers = NULL;
    initalize_channel_session("#general", NULL, &channel, &channelUsers);

    add_user_to_channel_users(channelUsers, user1);
    add_channel_to_user_channels(userChannels1, channel);
    add_user_to_channel_users(channelUsers, user2);
    add_channel_to_user_channels(userChannels2, channel);

    execute_command(server->clients[DEFAULT_FD_IDX], get_command_function(QUIT), "QUIT :bye");
    char *message = dequeue_from_channel_queue(channel);
    ck_assert_str_eq(message, ":john!@ QUIT :bye");
    
    ck_assert_int_eq(server->session->usersCount, 1);

    cleanup_test();

}
END_TEST

Suite* command_handler_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("CommandInfo handler");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    tcase_add_test(tc_core, test_get_command_function);
    // tcase_add_test(tc_core, test_create_user_info);
    tcase_add_test(tc_core, test_parse_message);
    tcase_add_test(tc_core, test_cmd_nick);
    tcase_add_test(tc_core, test_cmd_user);
    tcase_add_test(tc_core, test_cmd_join);
    tcase_add_test(tc_core, test_cmd_part);
    tcase_add_test(tc_core, test_cmd_privmsg);
    tcase_add_test(tc_core, test_cmd_quit);

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