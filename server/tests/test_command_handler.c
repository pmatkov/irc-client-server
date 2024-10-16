#ifndef TEST
#define TEST
#endif

#include "../src/priv_command_handler.h"
#include "../src/priv_tcp_server.h"
#include "../src/priv_session.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/command.h"
#include "../../libs/src/message.h"
#include "../../libs/src/linked_list.h"
#include "../../libs/src/string_utils.h"

#include <check.h>

#define INT_DIGITS 10
#define FD 4
#define FD_INDEX 0

void execute_command(TCPServer *server, CommandTokens *cmdTokens, CommandFunc cmdFunction, const char *content) {

    set_client_inbuffer(&server->clients[FD_INDEX], content);
    parse_message(server, &server->clients[FD_INDEX], cmdTokens);
    cmdFunction(server, &server->clients[FD_INDEX], cmdTokens); 
}

START_TEST(test_get_command_function) {

    ck_assert_ptr_eq(get_command_function(QUIT), cmd_quit);
    ck_assert_ptr_ne(get_command_function(JOIN), cmd_quit);

}
END_TEST

START_TEST(test_create_client_info) {

    char clientInfo[MAX_CHARS + 1] = {'\0'};

    create_client_info(clientInfo, MAX_CHARS, "jdoe", "john", "client.example.com");

    ck_assert_str_eq(clientInfo, "jdoe!john@client.example.com");

}
END_TEST

START_TEST(test_parse_message) {

    TCPServer *server = create_server(0);

    set_fd(server, FD_INDEX, FD);
    set_client_nickname(&server->clients[FD_INDEX], "john");
    set_client_inbuffer(&server->clients[FD_INDEX], "PRIVMSG #general :Hello");

    CommandTokens *cmdTokens = create_command_tokens();

    parse_message(server, &server->clients[FD_INDEX], cmdTokens);

    ck_assert_str_eq(cmdTokens->command, "PRIVMSG");
    ck_assert_int_eq(cmdTokens->argCount, 2);
    ck_assert_str_eq(cmdTokens->args[0], "#general");
    ck_assert_str_eq(cmdTokens->args[1], ":Hello");

    delete_command_tokens(cmdTokens);

    delete_server(server);

}
END_TEST

START_TEST(test_cmd_nick) {

    set_default_settings();
    read_settings(NULL, SERVER_PROPERTY);

    TCPServer *server = create_server(0);

    CommandTokens *cmdTokens = create_command_tokens();

    set_fd(server, FD_INDEX, FD);

    char fdBuffer[INT_DIGITS + 1] = {'\0'};
    uint_to_str(fdBuffer, INT_DIGITS + 1, *server->clients[FD_INDEX].fd);

    set_client_nickname(&server->clients[FD_INDEX], "john");

    execute_command(server, cmdTokens, get_command_function(NICK), "NICK");
    ExtMessage *extMessage = remove_message_from_server_queue(server);
    ck_assert_str_eq(get_ext_message_recipient(extMessage), fdBuffer);
    ck_assert_str_eq(get_ext_message_content(extMessage), ":irc.example.com 431 * :No nickname given");
    reset_cmd_tokens(cmdTokens);

    execute_command(server, cmdTokens, get_command_function(NICK), "NICK john$");
    extMessage = remove_message_from_server_queue(server);
    ck_assert_str_eq(get_ext_message_recipient(extMessage), fdBuffer);
    ck_assert_str_eq(get_ext_message_content(extMessage), ":irc.example.com 432 john$ :Erroneous nickname");
    reset_cmd_tokens(cmdTokens);

    User *user = create_user("john", NULL, NULL, NULL, 0);
    add_user_to_hash_table(server->session, user);
    UserChannels *userChannels = create_user_channels(user);
    add_user_channels(server->session, userChannels);

    Channel *channel1 = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    add_channel_to_user_channels(userChannels, channel1);

    Channel *channel2 = create_channel("#linux", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    add_channel_to_user_channels(userChannels, channel2); 

    execute_command(server, cmdTokens, get_command_function(NICK), "NICK john");
    extMessage = remove_message_from_server_queue(server);
    ck_assert_str_eq(get_ext_message_recipient(extMessage), fdBuffer);
    ck_assert_str_eq(get_ext_message_content(extMessage), ":irc.example.com 433 john :Nickname is already in use");
    reset_cmd_tokens(cmdTokens);

    set_client_registered(&server->clients[FD_INDEX], 1);

    execute_command(server, cmdTokens, get_command_function(NICK), "NICK john707");
    RegMessage *regMessage = remove_message_from_channel_queue(channel1);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":john!@ NICK john707");
    ck_assert_int_eq(get_list_count(get_ready_channels(get_ready_list(server->session))), 2);
    reset_cmd_tokens(cmdTokens);

    User *newUser = find_user_in_hash_table(server->session, "john707");
    ck_assert_int_eq(server->session->usersCount, 1);
    ck_assert_ptr_ne(user, newUser);

    delete_channel(channel1);
    delete_channel(channel2);
    delete_command_tokens(cmdTokens);

    delete_server(server);

}
END_TEST

START_TEST(test_cmd_user) {

    set_default_settings();
    read_settings(NULL, SERVER_PROPERTY);

    TCPServer *server = create_server(0);

    CommandTokens *cmdTokens = create_command_tokens();

    set_fd(server, FD_INDEX, FD);

    execute_command(server, cmdTokens, get_command_function(USER), "USER john 127.0.0.1 * :john jones");
    ExtMessage *extMessage = remove_message_from_server_queue(server);
    ck_assert_str_eq(get_ext_message_content(extMessage), ":irc.example.com 451 * :You have not registered");
    reset_cmd_tokens(cmdTokens);

    set_client_nickname(&server->clients[FD_INDEX], "john");
    set_client_registered(&server->clients[FD_INDEX], 1);

    User *user = create_user("john", NULL, NULL, NULL, 0);
    add_user_to_hash_table(server->session, user);

    execute_command(server, cmdTokens, get_command_function(USER), "USER john 127.0.0.1 * :john jones");
    RegMessage *regMessage = remove_message_from_user_queue(user);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":irc.example.com 462 john :Already registered");
    reset_cmd_tokens(cmdTokens);

    set_client_registered(&server->clients[FD_INDEX], 0);

    execute_command(server, cmdTokens, get_command_function(USER), "USER john");
    extMessage = remove_message_from_server_queue(server);
    ck_assert_str_eq(get_ext_message_content(extMessage), ":irc.example.com 461 john USER :Not enough parameters");
    reset_cmd_tokens(cmdTokens);

    remove_user_from_hash_table(server->session, user);

    execute_command(server, cmdTokens, get_command_function(USER), "USER john 127.0.0.1 * :john jones");

    user = find_user_in_hash_table(server->session, "john");

    regMessage = remove_message_from_user_queue(user);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":irc.example.com 001 john :Welcome to the IRC Network");

    ck_assert_int_eq(is_client_registered(&server->clients[FD_INDEX]), 1);

    delete_command_tokens(cmdTokens);

    delete_server(server);

}
END_TEST

START_TEST(test_cmd_join) {

    set_default_settings();
    read_settings(NULL, SERVER_PROPERTY);

    TCPServer *server = create_server(0);

    CommandTokens *cmdTokens = create_command_tokens();

    set_fd(server, FD_INDEX, FD);

    set_client_nickname(&server->clients[FD_INDEX], "john");
    set_client_registered(&server->clients[FD_INDEX], 0);

    execute_command(server, cmdTokens, get_command_function(JOIN), "JOIN #general");
    ExtMessage *extMessage = remove_message_from_server_queue(server);
    ck_assert_str_eq(get_ext_message_content(extMessage), ":irc.example.com 451 * :You have not registered");
    reset_cmd_tokens(cmdTokens);

    set_client_registered(&server->clients[FD_INDEX], 1);

    User *user = create_user("john", NULL, NULL, NULL, 0);
    add_user_to_hash_table(server->session, user);
    UserChannels *userChannels = create_user_channels(user);
    add_user_channels(server->session, userChannels);

    execute_command(server, cmdTokens, get_command_function(JOIN), "JOIN");
    RegMessage *regMessage = remove_message_from_user_queue(user);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":irc.example.com 461 john JOIN :Not enough parameters");
    reset_cmd_tokens(cmdTokens);

    execute_command(server, cmdTokens, get_command_function(JOIN), "JOIN $linux");
    regMessage = remove_message_from_user_queue(user);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":irc.example.com 479 john $linux :Illegal channel name");  

    Channel *channel = create_channel("#general", "football weekend", TEMPORARY, MAX_USERS_PER_CHANNEL);
    add_channel_to_hash_table(server->session, channel);
    ChannelUsers *channelUsers = create_channel_users(channel);
    add_channel_users(server->session, channelUsers);

    execute_command(server, cmdTokens, get_command_function(JOIN), "JOIN #general");
    regMessage = remove_message_from_channel_queue(channel);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":john!@ JOIN #general");

    regMessage = remove_message_from_user_queue(user);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":irc.example.com 332 john #general :football weekend");
    reset_cmd_tokens(cmdTokens);
    regMessage = remove_message_from_user_queue(user);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":irc.example.com 353 john #general :john");
    reset_cmd_tokens(cmdTokens);  
    regMessage = remove_message_from_user_queue(user);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":irc.example.com 366 john #general :End of NAMES list");
    reset_cmd_tokens(cmdTokens);  

    execute_command(server, cmdTokens, get_command_function(JOIN), "JOIN #linux");
    Channel *newChannel = find_channel_in_hash_table(server->session, "#linux");
    regMessage = remove_message_from_channel_queue(newChannel);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":john!@ JOIN #linux");
    regMessage = remove_message_from_user_queue(user);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":irc.example.com 331 john #linux :No topic is set");
    reset_cmd_tokens(cmdTokens);  

    delete_command_tokens(cmdTokens);

    delete_server(server);

}
END_TEST

START_TEST(test_cmd_part) {

    set_default_settings();
    read_settings(NULL, SERVER_PROPERTY);

    TCPServer *server = create_server(0);

    CommandTokens *cmdTokens = create_command_tokens();

    set_fd(server, FD_INDEX, FD);
    set_fd(server, FD_INDEX + 1, FD + 1);

    set_client_nickname(&server->clients[FD_INDEX], "john");
    set_client_registered(&server->clients[FD_INDEX], 1);
    set_client_nickname(&server->clients[FD_INDEX + 1], "mark");
    set_client_registered(&server->clients[FD_INDEX + 1], 1);

    User *user1 = create_user("john", NULL, NULL, NULL, 0);
    add_user_to_hash_table(server->session, user1);
    UserChannels *userChannels1 = create_user_channels(user1);
    add_user_channels(server->session, userChannels1);

    User *user2 = create_user("mark", NULL, NULL, NULL, 0);
    add_user_to_hash_table(server->session, user2);
    UserChannels *userChannels2 = create_user_channels(user2);
    add_user_channels(server->session, userChannels2);

    execute_command(server, cmdTokens, get_command_function(PART), "PART #general");
    RegMessage *regMessage = remove_message_from_user_queue(user1);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":irc.example.com 403 john #general :No such channel");
    reset_cmd_tokens(cmdTokens);

    Channel *channel = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    add_channel_to_hash_table(server->session, channel);
    ChannelUsers *channelUsers = create_channel_users(channel);
    add_channel_users(server->session, channelUsers);

    execute_command(server, cmdTokens, get_command_function(PART), "PART #general");
    regMessage = remove_message_from_user_queue(user1);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":irc.example.com 442 john #general :You're not on that channel");
    reset_cmd_tokens(cmdTokens);

    add_user_to_channel_users(channelUsers, user1);
    add_channel_to_user_channels(userChannels1, channel);

    execute_command(server, cmdTokens, get_command_function(PART), "PART #general");
    ck_assert_int_eq(server->session->channelsCount, 0);
    ck_assert_int_eq(server->session->channelUsersLL->count, 0);
    ck_assert_int_eq(userChannels1->count, 0);

    channel = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    add_channel_to_hash_table(server->session, channel);
    channelUsers = create_channel_users(channel);
    add_channel_users(server->session, channelUsers);

    add_user_to_channel_users(channelUsers, user1);
    add_channel_to_user_channels(userChannels1, channel);
    add_user_to_channel_users(channelUsers, user2);
    add_channel_to_user_channels(userChannels2, channel);

    execute_command(server, cmdTokens, get_command_function(PART), "PART #general :bye");
    regMessage = remove_message_from_channel_queue(channel);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":john!@ PART #general :bye");

    delete_command_tokens(cmdTokens);

    delete_server(server);

}
END_TEST

START_TEST(test_cmd_privmsg) {

    set_default_settings();
    read_settings(NULL, SERVER_PROPERTY);

    TCPServer *server = create_server(0);

    CommandTokens *cmdTokens = create_command_tokens();

    set_fd(server, FD_INDEX, FD);
    set_fd(server, FD_INDEX + 1, FD + 1);

    set_client_nickname(&server->clients[FD_INDEX], "john");
    set_client_registered(&server->clients[FD_INDEX], 1);
    set_client_nickname(&server->clients[FD_INDEX + 1], "mark");
    set_client_registered(&server->clients[FD_INDEX + 1], 1);

    User *user1 = create_user("john", NULL, NULL, NULL, 0);
    add_user_to_hash_table(server->session, user1);
    UserChannels *userChannels1 = create_user_channels(user1);
    add_user_channels(server->session, userChannels1);

    User *user2 = create_user("mark", NULL, NULL, NULL, 0);
    add_user_to_hash_table(server->session, user2);
    UserChannels *userChannels2 = create_user_channels(user2);
    add_user_channels(server->session, userChannels2);

    execute_command(server, cmdTokens, get_command_function(PRIVMSG), "PRIVMSG steve :hello");
    RegMessage *regMessage = remove_message_from_user_queue(user1);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":irc.example.com 401 john steve :No such nick");
    reset_cmd_tokens(cmdTokens);

    execute_command(server, cmdTokens, get_command_function(PRIVMSG), "PRIVMSG mark :hello");
    regMessage = remove_message_from_user_queue(user2);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":john!@ PRIVMSG mark :hello");
    reset_cmd_tokens(cmdTokens);

    Channel *channel = create_channel("#general", NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
    add_channel_to_hash_table(server->session, channel);
    ChannelUsers *channelUsers = create_channel_users(channel);
    add_channel_users(server->session, channelUsers);

    execute_command(server, cmdTokens, get_command_function(PRIVMSG), "PRIVMSG #linux :hello");
    regMessage = remove_message_from_user_queue(user1);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":irc.example.com 403 john #linux :No such channel");
    reset_cmd_tokens(cmdTokens);

    execute_command(server, cmdTokens, get_command_function(PRIVMSG), "PRIVMSG #general :hello");
    regMessage = remove_message_from_user_queue(user1);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":irc.example.com 442 john #general :You're not on that channel");
    reset_cmd_tokens(cmdTokens);

    add_user_to_channel_users(channelUsers, user1);
    add_channel_to_user_channels(userChannels1, channel);

    execute_command(server, cmdTokens, get_command_function(PRIVMSG), "PRIVMSG #general :hello");
    regMessage = remove_message_from_channel_queue(channel);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":john!@ PRIVMSG #general :hello");

    delete_command_tokens(cmdTokens);

    delete_server(server);

}
END_TEST

START_TEST(test_cmd_quit) {

    set_default_settings();
    read_settings(NULL, SERVER_PROPERTY);

    TCPServer *server = create_server(0);

    CommandTokens *cmdTokens = create_command_tokens();

    set_fd(server, FD_INDEX, FD);
    set_fd(server, FD_INDEX + 1, FD + 1);

    set_client_nickname(&server->clients[FD_INDEX], "john");
    set_client_registered(&server->clients[FD_INDEX], 1);
    set_client_nickname(&server->clients[FD_INDEX + 1], "mark");
    set_client_registered(&server->clients[FD_INDEX + 1], 1);

    User *user1 = create_user("john", NULL, NULL, NULL, 0);
    add_user_to_hash_table(server->session, user1);
    UserChannels *userChannels1 = create_user_channels(user1);
    add_user_channels(server->session, userChannels1);

    User *user2 = create_user("mark", NULL, NULL, NULL, 0);
    add_user_to_hash_table(server->session, user2);
    UserChannels *userChannels2 = create_user_channels(user2);
    add_user_channels(server->session, userChannels2);

    ck_assert_int_eq(server->session->usersCount, 2);

    Channel *channel = create_channel("#general", NULL, PERMANENT, MAX_USERS_PER_CHANNEL);
    add_channel_to_hash_table(server->session, channel);
    ChannelUsers *channelUsers = create_channel_users(channel);
    add_channel_users(server->session, channelUsers);

    add_user_to_channel_users(channelUsers, user1);
    add_channel_to_user_channels(userChannels1, channel);
    add_user_to_channel_users(channelUsers, user2);
    add_channel_to_user_channels(userChannels2, channel);

    execute_command(server, cmdTokens, get_command_function(QUIT), "QUIT :bye");
    RegMessage *regMessage = remove_message_from_channel_queue(channel);
    ck_assert_str_eq(get_reg_message_content(regMessage), ":john!@ QUIT :bye");
    reset_cmd_tokens(cmdTokens);
    ck_assert_int_eq(server->session->usersCount, 1);

    delete_command_tokens(cmdTokens);

    delete_server(server);

}
END_TEST

Suite* command_handler_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Command handler");
    tc_core = tcase_create("Core");

    // Add the test case to the test suite
    // tcase_add_test(tc_core, test_get_command_function);
    // tcase_add_test(tc_core, test_create_client_info);
    // tcase_add_test(tc_core, test_parse_message);
    // tcase_add_test(tc_core, test_cmd_nick);
    // tcase_add_test(tc_core, test_cmd_user);
    // tcase_add_test(tc_core, test_cmd_join);
    tcase_add_test(tc_core, test_cmd_part);
    // tcase_add_test(tc_core, test_cmd_privmsg);
    // tcase_add_test(tc_core, test_cmd_quit);

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