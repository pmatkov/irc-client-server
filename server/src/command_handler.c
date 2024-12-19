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
#include "../../libs/src/common.h"
#include "../../libs/src/session_state.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/irc_message.h"
#include "../../libs/src/linked_list.h"
#include "../../libs/src/network_utils.h"
#include "../../libs/src/enum_utils.h"
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

#define USER_CMD_PARAMS 4

STATIC void cmd_nick(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_user(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_join(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_part(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_privmsg(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_whois(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_quit(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_unknown(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

STATIC void handle_nickname_change(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void handle_user_registration(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

STATIC void handle_join_existing_channel(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void handle_join_new_channel(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void add_topic_message_to_queue(TCPServer *tcpServer, Client *client, Channel *channel, CommandTokens *cmdTokens);
STATIC void handle_leave_channel(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

STATIC void send_privmsg_to_channel(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void send_privmsg_to_user(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

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
    cmd_whois,
    cmd_quit,
    cmd_unknown
};

ASSERT_ARRAY_SIZE(COMMAND_FUNCTIONS, COMMAND_TYPE_COUNT)

void parse_message(const char *message, CommandTokens *cmdTokens) {

    if (message == NULL || cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    // input format "<COMMAND_INFO> [param 1] ... [param n] [:notice]"
    safe_copy(get_command_input(cmdTokens), MAX_CHARS + 1, message);

    int tkCount = count_tokens(get_command_input(cmdTokens), ":");
    if (tkCount > MAX_TOKENS) {
        tkCount = MAX_TOKENS;
    }

    const char *tokens[MAX_TOKENS] = {NULL};

    tokenize_string(get_command_input(cmdTokens), tokens, tkCount, " ");

    set_command(cmdTokens, tokens[0]);

    for (int i = 0; i < tkCount - 1; i++) {
        set_command_argument(cmdTokens, tokens[i + 1], i);
    }

    set_command_argument_count(cmdTokens, tkCount - 1);
}

STATIC void cmd_nick(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_server_session_states(), get_client_state_type(client), NICK)) {
        // :server 451 * :You have not registered
        const char *code = get_response_code(ERR_NOTREGISTERED);         
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        return;
    }

    if (!get_command_argument_count(cmdTokens)) {

        // :server 431 * :No nickname given
        const char *code = get_response_code(ERR_NONICKNAMEGIVEN);

        add_irc_message_to_queue(tcpServer, client, &(IRCMessage) {{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {

        const char *nickname = get_command_argument(cmdTokens, 0);

        if (strlen(nickname) > MAX_NICKNAME_LEN || !is_valid_user_name(nickname)) {

            // :server 432 <nickname> :Erroneous nickname
            const char *code = get_response_code(ERR_ERRONEUSNICKNAME);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname}, {get_response_message(code)}, 1, create_server_info, tcpServer});
            LOG(DEBUG, "Nickname <%s> invalid", nickname);
        }
        else if (find_user_in_hash_table(get_session(tcpServer), nickname) != NULL) {

            // :server 433 <nickname> :Nickname is already in use
            const char *code = get_response_code(ERR_NICKNAMEINUSE);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname}, {get_response_message(code)}, 1, create_server_info, tcpServer});
            LOG(DEBUG, "Nickname <%s> in use", nickname);
        }
        else {
            if (get_client_state_type(client) == CONNECTED) {
                set_client_state_type(client, START_REGISTRATION);
            }
            if (get_client_state_type(client) >= REGISTERED) {  
                handle_nickname_change(tcpServer, client, cmdTokens);
            }
            set_client_nickname(client, nickname);
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
        LOG(DEBUG, "Nickname changed from <%s> to <%s>", get_client_nickname(client), get_command_argument(cmdTokens, 0));
    }
}

STATIC void cmd_user(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_server_session_states(), get_client_state_type(client), USER)) {
       
        if (get_client_state_type(client) == DISCONNECTED || get_client_state_type(client) == CONNECTED) {

            // :server 451 * :You have not registered
            const char *code = get_response_code(ERR_NOTREGISTERED);         
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
        else {

            // :server 462 <nickname> :Already registered
            const char *code = get_response_code(ERR_ALREADYREGISTRED);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, get_client_nickname(client)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
            LOG(DEBUG, "User <%s> already registered", get_client_nickname(client));
        }
    }
    else {
        if (get_command_argument_count(cmdTokens) < USER_CMD_PARAMS) {

            // :server 461 <nickname> <cmd> :Not enough parameters
            const char *code = get_response_code(ERR_NEEDMOREPARAMS);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, get_client_nickname(client), get_command(cmdTokens)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
        else {
            handle_user_registration(tcpServer, client, cmdTokens);           
        }
    }
}

STATIC void handle_user_registration(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    const char *nickname = get_client_nickname(client);
    const char *clientIdentifier = get_client_identifier(client);
    char hostname[MAX_CHARS + 1] = {'\0'};

    if (get_client_identifier_type(client) == IP_ADDRESS) {
        ip_to_hostname(hostname, ARRAY_SIZE(hostname), get_client_identifier(client));
        clientIdentifier = hostname;
    }

    const char *realname = NULL;

    if (find_delimiter(get_command_argument(cmdTokens, 3), ":") != NULL) {
        realname = &get_command_argument(cmdTokens, 3)[1];
    }
    else {
        realname = get_command_argument(cmdTokens, 3);
    }

    User *user = create_user(get_client_fd(client), nickname, get_command_argument(cmdTokens, 0), clientIdentifier, realname);

    register_user(get_session(tcpServer), user);
    set_client_state_type(client, REGISTERED);

    // :server 001 <nickname> :Welcome to the IRC Network
    const char *code = get_response_code(RPL_WELCOME);
    add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    LOG(DEBUG, "User <%s> registered", nickname);
}

STATIC void cmd_join(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_server_session_states(), get_client_state_type(client), JOIN)) {
        // :server 451 * :You have not registered
        const char *code = get_response_code(ERR_NOTREGISTERED);         
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        return;
    }

    if (!get_command_argument_count(cmdTokens)) {

        // :server 461 <nickname> <cmd> :Not enough parameters
        const char *code = get_response_code(ERR_NEEDMOREPARAMS);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, get_client_nickname(client), get_command(cmdTokens)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
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
            LOG(DEBUG, "Channel <%s> is full", get_command_argument(cmdTokens, 0));

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
                char nicknameList[MAX_CHARS + 1];
            } data = {{'\0'}};

            iterate_list(get_users_from_channel_users(channelUsers), add_nickname_to_list, &data);

            // :server 353 <nickname> <channel> :<nicknames list>
            const char *code = get_response_code(RPL_NAMREPLY);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {data.nicknameList}, 1, create_server_info, tcpServer});

            // :server 366 <nickname> <channel> :<End of NAMES list>
            code = get_response_code(RPL_ENDOFNAMES);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});

            if (get_client_state_type(client) == REGISTERED) {
                set_client_state_type(client, IN_CHANNEL);
            }
            LOG(DEBUG, "Joined channel <%s>", get_command_argument(cmdTokens, 0));
        }
    }
}

STATIC void handle_join_new_channel(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    const char *nickname = get_client_nickname(client);

    User *user = find_user_in_hash_table(get_session(tcpServer), nickname);

    if (strlen(get_command_argument(cmdTokens, 0)) > MAX_CHANNEL_LEN || !is_valid_channel_name(get_command_argument(cmdTokens, 0))) {

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

        if (get_client_state_type(client) == REGISTERED) {
            set_client_state_type(client, IN_CHANNEL);
        }
        LOG(DEBUG, "Joined channel <%s>", get_command_argument(cmdTokens, 0));
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

STATIC void cmd_part(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_server_session_states(), get_client_state_type(client), PART)) {
       
        if (get_client_state_type(client) == DISCONNECTED || get_client_state_type(client) == CONNECTED) {

            // :server 451 * :You have not registered
            const char *code = get_response_code(ERR_NOTREGISTERED);         
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
        else {
            // :server 442 <nickname> <channel> :You're not on that channel
            const char *code = get_response_code(ERR_NOTONCHANNEL);       
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, get_client_nickname(client), get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
            LOG(DEBUG, "Not on channel <%s>", get_command_argument(cmdTokens, 0));
        }
        return;
    }

    const char *nickname = get_client_nickname(client);

    if (!get_command_argument_count(cmdTokens)) {
        // :server 461 <nickname> <cmd> :Not enough parameters
        const char *code = get_response_code(ERR_NEEDMOREPARAMS);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command(cmdTokens)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {
        handle_leave_channel(tcpServer, client, cmdTokens);
    
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

            char fwdMessage[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

            create_irc_message(fwdMessage, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {get_command_argument(cmdTokens, 1)}, 0, create_user_info, user});

            register_channel_leave(get_session(tcpServer), channel, user);

            if (get_channel_type(channel) == TEMPORARY && !get_channel_users_count(channelUsers)) {

                remove_channel_data(get_session(tcpServer), channel);
                add_message_to_queue(tcpServer, client, fwdMessage);
            }
            else {
                enqueue_to_channel_queue(channel, fwdMessage);
                add_channel_to_ready_list(channel, get_ready_list(get_session(tcpServer)));
            }        
            
            if (get_client_state_type(client) == IN_CHANNEL && !get_channel_users_count(channelUsers)) {
                set_client_state_type(client, REGISTERED);
            }
            LOG(DEBUG, "Left channel <%s>", get_command_argument(cmdTokens, 0));
        }
    }
}

STATIC void cmd_privmsg(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_server_session_states(), get_client_state_type(client), PRIVMSG)) {
        // :server 451 * :You have not registered
        const char *code = get_response_code(ERR_NOTREGISTERED);         
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        return;
    }

    const char *nickname = get_client_nickname(client);

    if (get_command_argument_count(cmdTokens) < 2) {

        // :server 461 <nickname> <cmd> :Not enough parameters
        const char *code = get_response_code(ERR_NEEDMOREPARAMS);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command(cmdTokens)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {
        // if (get_command_argument(cmdTokens, 0)[0] == '#') {
        if (is_valid_channel_name(get_command_argument(cmdTokens, 0))) {
            send_privmsg_to_channel(tcpServer, client, cmdTokens);
        }
        else {
            send_privmsg_to_user(tcpServer, client, cmdTokens);
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

            // <:nickname!username@hostname> PRIVMSG <channel> <:message>        
            char fwdMessage[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

            create_irc_message(fwdMessage, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {get_command_argument(cmdTokens, 1)}, 0, create_user_info, user});
            enqueue_to_channel_queue(channel, fwdMessage);
            add_channel_to_ready_list(channel, get_ready_list(get_session(tcpServer)));

            LOG(DEBUG, "Sent message to channel <%s>", get_command_argument(cmdTokens, 0));
        }
    }
}

STATIC void send_privmsg_to_user(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    const char *nickname = get_client_nickname(client);

    User *user = find_user_in_hash_table(get_session(tcpServer), nickname);
    User *recipient = find_user_in_hash_table(get_session(tcpServer), get_command_argument(cmdTokens, 0));

    if (recipient == NULL) {

        // :server 401 <client nickname> <nickname> :No such nick
        const char *code = get_response_code(ERR_NOSUCHNICK);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {
        // <:nickname!username@hostname> PRIVMSG <nickname> <:message>       
        char fwdMessage[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

        create_irc_message(fwdMessage, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens), get_command_argument(cmdTokens, 0)}, {get_command_argument(cmdTokens, 1)}, 0, create_user_info, user});
        enqueue_to_user_queue(recipient, fwdMessage);
        add_user_to_ready_list(user, get_ready_list(get_session(tcpServer)));

        LOG(DEBUG, "Sent message to user <%s>", get_command_argument(cmdTokens, 0));
    }
}


STATIC void cmd_whois(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_server_session_states(), get_client_state_type(client), WHOIS)) {
        // :server 451 * :You have not registered
        const char *code = get_response_code(ERR_NOTREGISTERED);         
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        return;
    }

    const char *nickname = get_client_nickname(client);

    if (!get_command_argument_count(cmdTokens)) {

        // :server 461 <nickname> <cmd> :Not enough parameters
        const char *code = get_response_code(ERR_NEEDMOREPARAMS);
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command(cmdTokens)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
    }
    else {

        User *whoisUser = find_user_in_hash_table(get_session(tcpServer), get_command_argument(cmdTokens, 0));

        if (whoisUser == NULL) {

            // :server 401 <client nickname> <nickname> :No such nick
            const char *code = get_response_code(ERR_NOSUCHNICK);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0)}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        }
        else {

            // :server 311 <client nickname> <nickname> <username> <hostname> <realname>
            const char *code = get_response_code(RPL_WHOISUSER);
            add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, nickname, get_command_argument(cmdTokens, 0), get_user_username(whoisUser), get_user_hostname(whoisUser)}, {get_user_realname(whoisUser)}, 1, create_server_info, tcpServer});

            LOG(DEBUG, "Performed WHOIS for user <%s>", get_command_argument(cmdTokens, 0));
        }
    }
}

STATIC void cmd_quit(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    if (!is_allowed_state_command(get_server_session_states(), get_client_state_type(client), QUIT)) {
        // :server 451 * :You have not registered
        const char *code = get_response_code(ERR_NOTREGISTERED);         
        add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, "*"}, {get_response_message(code)}, 1, create_server_info, tcpServer});
        return;
    }

    User *user = find_user_in_hash_table(get_session(tcpServer), get_client_nickname(client));

    // <:nickname!username@hostname> QUIT <:message>
    char fwdMessage[MAX_CHARS + CRLF_LEN + 1] = {'\0'};

    create_irc_message(fwdMessage, MAX_CHARS, &(IRCMessage){{get_command(cmdTokens)}, {get_command_argument(cmdTokens, 0)}, 0, create_user_info, user});

    LOG(INFO, "User quit (fd: %d)", get_client_fd(client));

    leave_all_channels(get_session(tcpServer), user, fwdMessage);

    ReadyList *readyList = get_ready_list(get_session(tcpServer));
    remove_user_from_ready_list(get_ready_users(readyList), user);

    unregister_user(get_session(tcpServer), user);
    remove_client(tcpServer, eventManager, get_client_fd(client));
}

STATIC void cmd_unknown(EventManager *eventManager, TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    // :server 421 <command> :Unknown command
    const char *code = get_response_code(ERR_UNKNOWNCOMMAND);
    add_irc_message_to_queue(tcpServer, client, &(IRCMessage){{code, get_command(cmdTokens)}, {get_response_message(code)}, 1, create_server_info, tcpServer});

    LOG(DEBUG, "Unknown command <%s>", get_command(cmdTokens));
}

CommandFunc get_command_function(CommandType cmdType) {

    CommandFunc cmdFunc = NULL;

    if (is_valid_enum_type(cmdType, COMMAND_TYPE_COUNT) && COMMAND_FUNCTIONS[cmdType] != NULL) {
        cmdFunc = COMMAND_FUNCTIONS[cmdType];
    }
    else {
        cmdFunc = cmd_unknown;
    }

    return cmdFunc;
}