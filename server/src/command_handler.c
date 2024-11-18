#ifdef TEST
#include "priv_command_handler.h"
#else
#include "command_handler.h"
#include "../../libs/src/response_code.h"
#endif

#include "user.h"
#include "channel.h"
#include "session.h"
#include "tcp_server.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/irc_message.h"
#include "../../libs/src/linked_list.h"
#include "../../libs/src/network_utils.h"
#include "../../libs/src/string_utils.h"
#include "../../libs/src/logger.h"
#include "../../libs/src/error_control.h"

#include <stddef.h>
#include <string.h>
#include <assert.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_USER_INFO 64
#define MAX_CHANNEL_LEN 50
#define MAX_NICKNAME_LEN 9
#define USER_MSG_PARAMS 4

STATIC void cmd_nick(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_user(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_join(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_part(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_privmsg(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_quit(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_unknown(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

STATIC void handle_nickname_change(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void handle_user_registration(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

STATIC void handle_join_existing_channel(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void handle_join_new_channel(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void add_topic_message_to_queue(TCPServer *tcpServer, Client *client, Channel *channel, CommandTokens *cmdTokens);
STATIC void handle_leave_channel(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void add_part_message_to_queue(TCPServer *tcpServer, Client *client, Channel *channel, CommandTokens *cmdTokens);

STATIC void send_privmsg_to_channel(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void send_privmsg_to_user(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

STATIC void remove_channel_data(Session *session, UserChannels *userChannels, Channel *channel);

static const CommandFunc COMMAND_FUNCTIONS[] = {
    NULL,
    NULL,
    NULL,
    cmd_nick,
    cmd_user,
    cmd_join,
    cmd_part,
    cmd_privmsg,
    NULL,
    NULL,
    cmd_quit,
    cmd_unknown
};

static_assert(ARR_SIZE(COMMAND_FUNCTIONS) == COMMAND_TYPE_COUNT, "Array size mismatch");

// parse received messages
void parse_message(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    if (tcpServer == NULL || client == NULL || cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    // input format "<COMMAND_INFO> [param 1] ... [param n] [:notice]"
    char *input = get_client_inbuffer(client);

    strcpy(get_command_input(cmdTokens), input);

    int tkCount = count_tokens(get_command_input(cmdTokens), ":");
    if (tkCount > MAX_TOKENS) {
        tkCount = MAX_TOKENS;
    }

    const char *tokens[MAX_TOKENS] = {NULL};

    tokenize_string(get_command_input(cmdTokens), tokens, tkCount, " ");

    set_command(cmdTokens, tokens[0]);
    set_command_argument_count(cmdTokens, tkCount - 1);

    for (int i = 0; i < tkCount - 1; i++) {
        set_command_argument(cmdTokens, tokens[i + 1], i);
    }
}

STATIC void cmd_nick(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    if (!get_command_argument_count(cmdTokens)) {

        // :server 431 * :No nickname given
        const char *code = get_response_code(ERR_NONICKNAMEGIVEN);

        add_irc_message_to_queue(tcpServer, client, &(IRCMessage) {{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {

        if (strlen(get_command_argument(cmdTokens, 0)) > MAX_NICKNAME_LEN || !is_valid_name(get_command_argument(cmdTokens, 0), 0)) {

            // :server 432 <nickname> :Erroneous nickname
            const char *code = get_response_code(ERR_ERRONEUSNICKNAME);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
        else if (find_user_in_hash_table(get_session(tcpServer), get_command_argument(cmdTokens, 0)) != NULL) {

            // :server 433 <nickname> :Nickname is already in use
            const char *code = get_response_code(ERR_NICKNAMEINUSE);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
        else {

            if (is_client_registered(client)) {

                handle_nickname_change(tcpServer, client, cmdTokens);
            }
            set_client_nickname(client, get_command_argument(cmdTokens, 0));
        }
    }
}

STATIC void handle_nickname_change(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    User *user = find_user_in_hash_table(get_session(tcpServer), get_client_nickname(client));
    User *userCopy = copy_user(user);
    set_user_nickname(userCopy, get_command_argument(cmdTokens, 0));

    UserChannels *userChannels = find_user_channels(get_session(tcpServer), user);

    if (userChannels != NULL) {

        // <:old nickname!username@hostname> NICK <new nickname>
        char fwdMessage[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

        create_irc_message(fwdMessage, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens)}, {get_command_argument(cmdTokens, 0)}, 0, create_user_info, user});

        iterate_list(get_channels_from_user_channels(userChannels), enqueue_to_channel_queue, fwdMessage);
        iterate_list(get_channels_from_user_channels(userChannels), add_channel_to_ready_list, get_ready_list(get_session(tcpServer)));

        change_user_in_user_channels(userChannels, userCopy);
        iterate_list(get_channel_users_ll(get_session(tcpServer)), change_user_in_channel_users, userCopy);

        change_user_in_hash_table(get_session(tcpServer), user, userCopy);
    }
}

STATIC void cmd_user(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    const char *nickname = get_client_nickname(client);

    if (!strlen(get_client_nickname(client))) {

        // :server 451 * :You have not registered
        const char *code = get_response_code(ERR_NOTREGISTERED);         
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        
    }
    else if (is_client_registered(client)) {

        // :server 462 <nickname> :Already registered
        const char *code = get_response_code(ERR_ALREADYREGISTRED);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {
        if (get_command_argument_count(cmdTokens) < USER_MSG_PARAMS) {

            // :server 461 <nickname> <cmd> :Not enough parameters
            const char *code = get_response_code(ERR_NEEDMOREPARAMS);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command(cmdTokens)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
        else {
            handle_user_registration(tcpServer, client, cmdTokens);
        }
    }
}

STATIC void handle_user_registration(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    // const char *servername = get_servername(tcpServer);
    const char *nickname = get_client_nickname(client);

    const char *address = NULL;
    const char *ipv4Address = get_client_ipv4address(client);
    char hostname[MAX_CHARS + 1] = {'\0'};

    if (ip_to_hostname(hostname, sizeof(hostname), ipv4Address)) {
        address = hostname;
    }
    else {
        address = ipv4Address;
    }

    User *user = create_user(nickname, get_command_argument(cmdTokens, 0), address, get_command_argument(cmdTokens, 3), *get_client_fd(client));

    register_user(get_session(tcpServer), user);
    set_client_registered(client, 1);

    // :server 001 <nickname> :Welcome to the IRC Network
    const char *code = get_response_code(RPL_WELCOME);
    add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname}, {get_response_message(code)}, 1, create_server_info, tcpServer});
}

STATIC void cmd_join(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    const char *nickname = get_client_nickname(client);

    if (!is_client_registered(client)) {

        const char *code = get_response_code(ERR_NOTREGISTERED);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {

        if (!get_command_argument_count(cmdTokens)) {

            // :server 461 <nickname> <cmd> :Not enough parameters
            const char *code = get_response_code(ERR_NEEDMOREPARAMS);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command(cmdTokens)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
        else {
            Channel *channel = find_channel_in_hash_table(get_session(tcpServer), get_command_argument(cmdTokens, 0));

            if (channel != NULL) {
                handle_join_existing_channel(tcpServer, client, cmdTokens); 
            }
            else {
                handle_join_new_channel(tcpServer, client, cmdTokens);
            }
        }
    }
}

STATIC void handle_join_existing_channel(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    const char *nickname = get_client_nickname(client);

    User *user = find_user_in_hash_table(get_session(tcpServer), nickname);
    Channel *channel = find_channel_in_hash_table(get_session(tcpServer), get_command_argument(cmdTokens, 0));
    ChannelUsers *channelUsers = find_channel_users(get_session(tcpServer), channel);

    if (find_user_in_channel_users(channelUsers, user) == NULL) {

        if (is_channel_full(channelUsers)) {

            // :server 471 <nickname> <channel> :Cannot join channel
            const char *code = get_response_code(ERR_CHANNELISFULL);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
        else {
            register_existing_channel_join(get_session(tcpServer), channel, user);

            // <:nickname!username@hostname> JOIN <channel>
            char fwdMessage[MAX_CHARS + CRLF_LEN + 1] = {'\0'};
            create_irc_message(fwdMessage, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens)}, {get_command_argument(cmdTokens, 0)}, 0, create_user_info, user});
            enqueue_to_channel_queue(channel, fwdMessage);
            add_channel_to_ready_list(channel, get_ready_list(get_session(tcpServer)));

            add_topic_message_to_queue(tcpServer, client, channel, cmdTokens);

            struct Data {
                char buffer[MAX_CHARS + 1];
            } data = {{'\0'}};

            iterate_list(get_users_from_channel_users(channelUsers), add_nickname_to_list, &data);

            // :server 353 <nickname> <channel> :<nicknames list>
            const char *code = get_response_code(RPL_NAMREPLY);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {data.buffer}, 1, create_server_info, tcpServer});

            // :server 366 <nickname> <channel> :<End of NAMES list>
            code = get_response_code(RPL_ENDOFNAMES);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
    }
}

STATIC void handle_join_new_channel(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    const char *nickname = get_client_nickname(client);

    User *user = find_user_in_hash_table(get_session(tcpServer), nickname);

    if (strlen(get_command_argument(cmdTokens, 0)) > MAX_CHANNEL_LEN || !is_valid_name(get_command_argument(cmdTokens, 0), 1)) {

        // :server 479 <nickname> <channel> :Illegal channel name
        const char *code = get_response_code(ERR_BADCHANNAME);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {

        Channel *channel = create_channel(get_command_argument(cmdTokens, 0), NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
        register_new_channel_join(get_session(tcpServer), channel, user);

        // <:nickname!username@hostname> JOIN <channel>
        char fwdMessage[MAX_CHARS + CRLF_LEN + 1] = {'\0'};
        create_irc_message(fwdMessage, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens)}, {get_command_argument(cmdTokens, 0)}, 0, create_user_info, user});
        enqueue_to_channel_queue(channel, fwdMessage);
        add_channel_to_ready_list(channel, get_ready_list(get_session(tcpServer)));

        // :server 331 <nickname> <channel> :No topic is set
        const char *code = get_response_code(RPL_NOTOPIC);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});

    }
}

STATIC void add_topic_message_to_queue(TCPServer *tcpServer, Client *client, Channel *channel, CommandTokens *cmdTokens) {
    const char *nickname = get_client_nickname(client);

    if (!strlen(get_channel_topic(channel))) {

        // :server 331 <nickname> <channel> :No topic is set
        const char *code = get_response_code(RPL_NOTOPIC);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {
        // :server 332 <nickname> <channel> :<topic>
        const char *code = get_response_code(RPL_TOPIC);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_channel_topic(channel)}, 1, create_server_info, tcpServer});
    }
}

STATIC void cmd_part(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    // const char *servername = get_servername(tcpServer);
    const char *nickname = get_client_nickname(client);

    if (!is_client_registered(client)) {

        const char *code = get_response_code(ERR_NOTREGISTERED);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {

        if (!get_command_argument_count(cmdTokens)) {

            // :server 461 <nickname> <cmd> :Not enough parameters
            const char *code = get_response_code(ERR_NEEDMOREPARAMS);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command(cmdTokens)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
        else {
           handle_leave_channel(tcpServer, client, cmdTokens);
        }
    }
}


STATIC void handle_leave_channel(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    const char *nickname = get_client_nickname(client);

    User *user = find_user_in_hash_table(get_session(tcpServer), nickname);
    Channel *channel = find_channel_in_hash_table(get_session(tcpServer), get_command_argument(cmdTokens, 0));

    if (channel == NULL) {

        // :server 403 <nickname> <channel> :No such channel
        const char *code = get_response_code(ERR_NOSUCHCHANNEL);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {

        ChannelUsers *channelUsers = find_channel_users(get_session(tcpServer), channel);

        if (find_user_in_channel_users(channelUsers, user) == NULL) {

            // :server 442 <nickname> <channel> :You're not on that channel
            const char *code = get_response_code(ERR_NOTONCHANNEL);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
        else {
            register_channel_leave(get_session(tcpServer), channel, user);

            if (get_channel_type(channel) == TEMPORARY && !get_channel_users_count(channelUsers)) {

                remove_channel_users(get_session(tcpServer), channelUsers);
                remove_channel_from_hash_table(get_session(tcpServer), channel);
            }
            else {
                add_part_message_to_queue(tcpServer, client, channel, cmdTokens);
            }
        }
    }
}

STATIC void add_part_message_to_queue(TCPServer *tcpServer, Client *client, Channel *channel, CommandTokens *cmdTokens) {

    const char *nickname = get_client_nickname(client);

    User *user = find_user_in_hash_table(get_session(tcpServer), nickname);

    char fwdMessage[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

    if (get_command_argument_count(cmdTokens) == 1) {
        // <nickname!username@hostname> PART <channel_name>
        create_irc_message(fwdMessage, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {NULL}, 0, create_user_info, user});
    }
    else {
        // <:nickname!username@hostname> PART <channel_name> <:message>
        create_irc_message(fwdMessage, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {get_command_argument(cmdTokens, 1)}, 0, create_user_info, user});
    }
    enqueue_to_channel_queue(channel, fwdMessage);
    add_channel_to_ready_list(channel, get_ready_list(get_session(tcpServer)));
}

STATIC void cmd_privmsg(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    const char *nickname = get_client_nickname(client);

    if (!is_client_registered(client)) {

        const char *code = get_response_code(ERR_NOTREGISTERED);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {

        if (get_command_argument_count(cmdTokens) < 2) {

            // :server 461 <nickname> <cmd> :Not enough parameters
            const char *code = get_response_code(ERR_NEEDMOREPARAMS);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command(cmdTokens)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
        else {

            if (get_command_argument(cmdTokens, 0)[0] == '#') {

                send_privmsg_to_channel(tcpServer, client, cmdTokens);
            }
            else {
                send_privmsg_to_user(tcpServer, client, cmdTokens);
            }
        }
    }
}

STATIC void send_privmsg_to_channel(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    const char *nickname = get_client_nickname(client);

    User *user = find_user_in_hash_table(get_session(tcpServer), nickname);
    Channel *channel = find_channel_in_hash_table(get_session(tcpServer), get_command_argument(cmdTokens, 0));

    if (channel == NULL) {

        // :server 403 <nickname> <channel> :No such channel
        const char *code = get_response_code(ERR_NOSUCHCHANNEL);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {

        ChannelUsers *channelUsers = find_channel_users(get_session(tcpServer), channel);

        if (find_user_in_channel_users(channelUsers, user) == NULL) {

            // :server 442 <nickname> <channel> :You're not on that channel
            const char *code = get_response_code(ERR_NOTONCHANNEL);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
        else {

            // <:nickname!username@hostname> PRIVMSG <nickname | channel> <:message>        
            char fwdMessage[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

            create_irc_message(fwdMessage, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {get_command_argument(cmdTokens, 1)}, 0, create_user_info, user});
            enqueue_to_channel_queue(channel, fwdMessage);
            add_channel_to_ready_list(channel, get_ready_list(get_session(tcpServer)));
        }
    }
}

STATIC void send_privmsg_to_user(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    const char *nickname = get_client_nickname(client);

    User *user = find_user_in_hash_table(get_session(tcpServer), nickname);
    User *recipient = find_user_in_hash_table(get_session(tcpServer), get_command_argument(cmdTokens, 0));

    if (recipient == NULL) {

        // :server 401 <nickname> <nickname> :No such nick
        const char *code = get_response_code(ERR_NOSUCHNICK);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {
        // <:nickname!username@hostname> PRIVMSG <nickname | channel> <:message>       
        char fwdMessage[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

        create_irc_message(fwdMessage, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {get_command_argument(cmdTokens, 1)}, 0, create_user_info, user});
        enqueue_to_user_queue(recipient, fwdMessage);
        add_user_to_ready_list(user, get_ready_list(get_session(tcpServer)));
    }
}

STATIC void cmd_quit(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    // const char *servername = get_servername(tcpServer);
    const char *nickname = get_client_nickname(client);

    if (!is_client_registered(client)) {

        const char *code = get_response_code(ERR_NOTREGISTERED);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {

        User *user = find_user_in_hash_table(get_session(tcpServer), nickname);

        // <:nickname!username@hostname> QUIT <:message>
        char fwdMessage[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

        create_irc_message(fwdMessage, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens)}, {get_command_argument(cmdTokens, 0)}, 0, create_user_info, user});

        /* remove and notify channels */
        UserChannels *userChannels = find_user_channels(get_session(tcpServer), user);
        
        if (userChannels != NULL) {

            struct Data {
                Session *session;
                Channel *channels[MAX_CHANNELS_PER_USER];
                int count;
            } data = {get_session(tcpServer), {NULL}, 0};

            iterate_list(get_channels_from_user_channels(userChannels), find_removable_channels, &data);

            for (int i = 0; i < data.count; i++) {
                remove_channel_data(data.session, userChannels, data.channels[i]);
            }
            iterate_list(get_channels_from_user_channels(userChannels), enqueue_to_channel_queue, fwdMessage);
            iterate_list(get_channels_from_user_channels(userChannels), add_channel_to_ready_list, get_ready_list(get_session(tcpServer)));
            remove_user_channels(get_session(tcpServer), userChannels);
        }
        remove_user_from_hash_table(get_session(tcpServer), user);
        remove_client(tcpServer, find_fd_index(tcpServer, *get_client_fd(client)));
    }
}

STATIC void remove_channel_data(Session *session, UserChannels *userChannels, Channel *channel) {

    remove_channel_in_user_channels(userChannels, channel);
    ChannelUsers *channelUsers = find_channel_users(session, channel);
    remove_channel_users(session, channelUsers);
    remove_channel_from_hash_table(session, channel);
}

STATIC void cmd_unknown(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {
    return;
}

CommandFunc get_command_function(CommandType commandType) {

    if (is_valid_command(commandType)) {
        return COMMAND_FUNCTIONS[commandType];
    }
    return cmd_unknown;
}