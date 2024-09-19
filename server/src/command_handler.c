#ifdef TEST
#include "priv_command_handler.h"
#else
#include "command_handler.h"
#endif

#include "user.h"
#include "channel.h"
#include "session.h"
#include "../../shared/src/linked_list.h"
#include "../../shared/src/priv_message.h"
#include "../../shared/src/response_code.h"
#include "../../shared/src/string_utils.h"
#include "../../shared/src/logger.h"
#include "../../shared/src/error_control.h"

#include <stddef.h>
#include <string.h>

#define ARR_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MSG_PARTS 2
#define MAX_COMMAND_LEN 32
#define MAX_RESPONSE_CODE_LEN 32
#define MAX_NICKNAME_LEN 9

STATIC void cmd_nick(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_user(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_join(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_part(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_privmsg(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_quit(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_unknown(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

STATIC void create_fwd_message(char *buffer, int size, const char *prefix, const char *body, const char *suffix);
STATIC void create_response(char *buffer, int size, const char *prefix, const char *body, ResponseType responseType);
STATIC void create_client_info(char *buffer, int size, const char *nickname, const char *username, const char *hostname);

typedef struct {
    const char *prefix;
    const char *param1;
    const char *param2;
    ResponseType responseType;
} ErrorMessage;

static const CommandFunction COMMAND_FUNCTIONS[] = {
    cmd_nick,
    cmd_user,
    cmd_join,
    cmd_part,
    cmd_privmsg,
    cmd_quit,
    cmd_unknown
};

STATIC void cmd_nick(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + 1] = {'\0'};
    const char *servername = get_server_name(tcpServer);

    if (!cmdTokens->argCount) {

        // :server 431 * :No nickname given
        create_response(message, MAX_CHARS, servername, "*", ERR_NONICKNAMEGIVEN);
    }
    else if (cmdTokens->argCount >= 1) {

        // if (strlen(cmdTokens->args[0]) > MAX_NICKNAME_LEN) {

        //     cmdTokens->args[0][MAX_NICKNAME_LEN] = '\0';
        // }
        if (!is_valid_name(cmdTokens->args[0], 0)) {

            // :server 432 <nickname> :Erroneous nickname
            create_response(message, MAX_CHARS, servername, cmdTokens->args[0], ERR_ERRONEUSNICKNAME);
        }
        else if (find_user_in_hash_table(get_session(tcpServer), cmdTokens->args[0]) != NULL) {

            // :server 433 <nickname> :Nickname is already in use
            create_response(message, MAX_CHARS, servername, cmdTokens->args[0], ERR_NICKNAMEINUSE);
        }
        else {

            if (is_client_registered(client)) {

                User *user = find_user_in_hash_table(get_session(tcpServer), cmdTokens->args[0]);
                User *newUser = create_user(cmdTokens->args[0], get_username(user), get_hostname(user), get_realname(user), get_client_fd(client));

                UserChannels *userChannels = find_user_channels(get_session(tcpServer), user);

                 /* notify channels that the user
                  has changed his nickname */

                if (userChannels != NULL) {

                    // :old_nick!user@host NICK :new_nick
                    char clientInfo[MAX_CHARS + 1] = {'\0'};
                    char fwdMessage[MAX_CHARS + 1] = {'\0'};

                    create_client_info(clientInfo, MAX_CHARS, get_user_nickname(user), get_username(user), get_hostname(user));

                    create_fwd_message(fwdMessage, MAX_CHARS, clientInfo, cmdTokens->command, cmdTokens->args[0]);

                    iterate_list(get_channels_from_user_channels(userChannels), add_message_to_channel, fwdMessage);

                    change_user_in_user_channels(userChannels, newUser);

                    iterate_list(get_channel_users_ll(get_session(tcpServer)), change_user_in_channel_users, newUser);

                }
            }
            set_client_nickname(client, cmdTokens->args[0]);
        }
    }
    add_message_to_queue(tcpServer, client, message);
}

STATIC void cmd_user(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + 1] = {'\0'};
    const char *servername = get_server_name(tcpServer);
    const char *nickname = get_client_nickname(client);

    if (!strlen(get_client_nickname(client))) {

        // :server 451 <nickname> :You have not registered
        create_response(message, MAX_CHARS, servername, "*", ERR_NOTREGISTERED);
    }
    else if (is_client_registered(client)) {

        // :server 462 <nickname> :You may not reregister
        create_response(message, MAX_CHARS, servername, nickname, ERR_ALREADYREGISTRED);
    }
    else {
        if (cmdTokens->argCount < 4) {
            char msgBody[MAX_CHARS + 1] = {'\0'};
            const char *tokens[] = {nickname, cmdTokens->command};  

            concat_tokens(msgBody, MAX_CHARS + 1, tokens, ARR_SIZE(tokens), " ");

            // :server 461 <nickname> <cmd> :Not enough parameters
            create_response(message, MAX_CHARS, servername, msgBody, ERR_NEEDMOREPARAMS);

        }
        else {

            User *user = create_user(nickname, cmdTokens->args[0], cmdTokens->args[1], cmdTokens->args[3], get_client_fd(client));

            add_user_to_hash_table(get_session(tcpServer), user);

            UserChannels *userChannels = create_user_channels(user);
            add_user_channels(get_session(tcpServer), userChannels);

            set_client_registered(client, 1);

            create_response(message, MAX_CHARS, servername, nickname, RPL_WELCOME);
        }
    }
    add_message_to_queue(tcpServer, client, message);
}

STATIC void cmd_join(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + 1] = {'\0'};
    const char *servername = get_server_name(tcpServer);
    const char *nickname = get_client_nickname(client);

    if (!is_client_registered(client)) {

        create_response(message, MAX_CHARS, servername, "*", ERR_NOTREGISTERED); 
    }
    else {

        if (!cmdTokens->argCount) {

            char msgBody[MAX_CHARS + 1] = {'\0'};
            const char *tokens[] = {nickname, cmdTokens->command};  

            concat_tokens(msgBody, MAX_CHARS + 1, tokens, ARR_SIZE(tokens), " ");

            // :server 461 <nickname> <cmd> :Not enough parameters
            create_response(message, MAX_CHARS, servername, msgBody, ERR_NEEDMOREPARAMS);
        }
        else {

            User *user = find_user_in_hash_table(get_session(tcpServer), nickname);
            Channel *channel = find_channel_in_hash_table(get_session(tcpServer), cmdTokens->args[0]);

            char clientInfo[MAX_CHARS + 1] = {'\0'};
            char fwdMessage[MAX_CHARS + 1] = {'\0'};

            create_client_info(clientInfo, MAX_CHARS, get_user_nickname(user), get_username(user), get_hostname(user));

            create_fwd_message(fwdMessage, MAX_CHARS, clientInfo, cmdTokens->command, cmdTokens->args[0]);
            
            if (channel != NULL) {

                ChannelUsers *channelUsers = find_channel_users(get_session(tcpServer), channel);

                if (find_user_in_channel_users(channelUsers, user) == NULL) {

                    int added = add_user_to_channel_users(channelUsers, user);

                    if (!added) {
                        char msgBody[MAX_CHARS + 1] = {'\0'};
                        const char *tokens[] = {nickname, cmdTokens->args[0]};  

                        concat_tokens(msgBody, MAX_CHARS + 1, tokens, ARR_SIZE(tokens), " ");

                        // :server 471 <nickname> <channel> :Cannot join channel
                        create_response(message, MAX_CHARS, servername, msgBody, ERR_CHANNELISFULL);
                    }
                    else {
                        add_message_to_channel(channel, fwdMessage);
                    }
                }
            }
            else {

                Channel *channel = create_channel(cmdTokens->args[0], TEMPORARY, MAX_USERS_PER_CHANNEL);

                add_channel_to_hash_table(get_session(tcpServer), channel);
                ChannelUsers *channelUsers = create_channel_users(channel);
                add_channel_users(get_session(tcpServer), channelUsers);
                add_user_to_channel_users(channelUsers, user);

                add_message_to_channel(channel, fwdMessage);
            }
        }
    }

    add_message_to_queue(tcpServer, client, message);
}

STATIC void cmd_part(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + 1] = {'\0'};
    const char *servername = get_server_name(tcpServer);
    const char *nickname = get_client_nickname(client);

    if (!is_client_registered(client)) {

        create_response(message, MAX_CHARS, servername, "*", ERR_NOTREGISTERED); 
    }
    else {

        if (!cmdTokens->argCount) {

            char msgBody[MAX_CHARS + 1] = {'\0'};
            const char *tokens[] = {nickname, cmdTokens->command};  

            concat_tokens(msgBody, MAX_CHARS + 1, tokens, ARR_SIZE(tokens), " ");

            // :server 461 <nickname> <cmd> :Not enough parameters
            create_response(message, MAX_CHARS, servername, msgBody, ERR_NEEDMOREPARAMS);
        }
        else {
            User *user = find_user_in_hash_table(get_session(tcpServer), nickname);
            Channel *channel = find_channel_in_hash_table(get_session(tcpServer), cmdTokens->args[0]);

            char clientInfo[MAX_CHARS + 1] = {'\0'};
            char fwdMessage[MAX_CHARS + 1] = {'\0'};

            create_client_info(clientInfo, MAX_CHARS, get_user_nickname(user), get_username(user), get_hostname(user));

            if (cmdTokens->argCount == 1) {

                create_fwd_message(fwdMessage, MAX_CHARS, clientInfo, cmdTokens->command, cmdTokens->args[0]);
            }
            else {

                char msgSuffix[MAX_CHARS + 1] = {'\0'};
                const char *tokens[] = {cmdTokens->args[0], cmdTokens->args[1]};  

                concat_tokens(msgSuffix, MAX_CHARS + 1, tokens, ARR_SIZE(tokens), " ");
                create_fwd_message(fwdMessage, MAX_CHARS, clientInfo, "PART", msgSuffix);
            }

            char msgBody[MAX_CHARS + 1] = {'\0'};
            const char *tokens[] = {nickname, cmdTokens->args[0]};  

            concat_tokens(msgBody, MAX_CHARS + 1, tokens, ARR_SIZE(tokens), " ");
            
            if (channel == NULL) {

                // :server 403  <nickname> <channel> :No such channel
                create_response(message, MAX_CHARS, servername, msgBody, ERR_NOSUCHCHANNEL);
            }
            else {

                ChannelUsers *channelUsers = find_channel_users(get_session(tcpServer), channel);

                if (find_user_in_channel_users(channelUsers, user) == NULL) {

                    // :server 442  <nickname> <channel> :You're not on that channel
                    create_response(message, MAX_CHARS, servername, msgBody, ERR_NOTONCHANNEL);
                }
                else {

                    remove_user_in_channel_users(channelUsers, user);

                    if (get_channel_type(channel) == TEMPORARY && !get_users_count_from_channel_users(channelUsers)) {

                        remove_channel_users(get_session(tcpServer), channelUsers);
                        remove_channel_from_hash_table(get_session(tcpServer), channel);
                    }
                    else {
                        add_message_to_channel(channel, fwdMessage);
                    }

                    UserChannels *userChannels = find_user_channels(get_session(tcpServer), user);

                    remove_channel_in_user_channels(userChannels, channel);

                }
            }
        }
    }
    add_message_to_queue(tcpServer, client, message);
}

STATIC void cmd_privmsg(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + 1] = {'\0'};
    const char *servername = get_server_name(tcpServer);
    const char *nickname = get_client_nickname(client);

    if (!is_client_registered(client)) {

        create_response(message, MAX_CHARS, servername, "*", ERR_NOTREGISTERED);
    }
    else {

        if (cmdTokens->argCount < 2) {

            char msgBody[MAX_CHARS + 1] = {'\0'};
            const char *tokens[] = {nickname, cmdTokens->command};  

            concat_tokens(msgBody, MAX_CHARS + 1, tokens, ARR_SIZE(tokens), " ");

            // :server 461 <nickname> <cmd> :Not enough parameters
            create_response(message, MAX_CHARS, servername, msgBody, ERR_NEEDMOREPARAMS);
        }
        else {

            User *user = find_user_in_hash_table(get_session(tcpServer), nickname);

            char msgBody[MAX_CHARS + 1] = {'\0'};
            const char *tokens[] = {cmdTokens->command, cmdTokens->args[0]};  

            concat_tokens(msgBody, MAX_CHARS + 1, tokens, ARR_SIZE(tokens), " ");

            char clientInfo[MAX_CHARS + 1] = {'\0'};
            char fwdMessage[MAX_CHARS + 1] = {'\0'};

            create_client_info(clientInfo, MAX_CHARS, get_user_nickname(user), get_username(user), get_hostname(user));

            create_fwd_message(fwdMessage, MAX_CHARS, clientInfo, msgBody, cmdTokens->args[1]);

            if (cmdTokens->args[0][0] == '#') {

                Channel *channel = find_channel_in_hash_table(get_session(tcpServer), cmdTokens->args[0]);

                if (channel == NULL) {

                    // :server 403  <nickname> <channel> :No such channel
                    create_response(message, MAX_CHARS, servername, msgBody, ERR_NOSUCHCHANNEL);
                }
                else {

                    ChannelUsers *channelUsers = find_channel_users(get_session(tcpServer), channel);

                    if (find_user_in_channel_users(channelUsers, user) == NULL) {

                        // :server 442  <nickname> <channel> :You're not on that channel
                        create_response(message, MAX_CHARS, servername, msgBody, ERR_NOTONCHANNEL);
                    }
                    else {

                        add_message_to_channel(channel, fwdMessage);
                    }
                } 
            }
        }
    }
    add_message_to_queue(tcpServer, client, message);
}

STATIC void cmd_quit(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + 1] = {'\0'};
    const char *servername = get_server_name(tcpServer);
    const char *nickname = get_client_nickname(client);

    if (!is_client_registered(client)) {

        create_response(message, MAX_CHARS, servername, "*", ERR_NOTREGISTERED);
    }
    else {

        User *user = find_user_in_hash_table(get_session(tcpServer), nickname);

        char clientInfo[MAX_CHARS + 1] = {'\0'};
        char fwdMessage[MAX_CHARS + 1] = {'\0'};

        create_client_info(clientInfo, MAX_CHARS, get_user_nickname(user), get_username(user), get_hostname(user));

        create_fwd_message(fwdMessage, MAX_CHARS, clientInfo, "QUIT", cmdTokens->args[0]);

        UserChannels *userChannels = find_user_channels(get_session(tcpServer), user);
        
        if (userChannels != NULL) {

            struct Data {
                Session *session;
                Channel *channels[MAX_CHANNELS_PER_USER];
                int index;
            };

            struct Data data = {get_session(tcpServer), {NULL}, 0};

            iterate_list(get_channels_from_user_channels(userChannels), find_single_user_channels, &data);

            for (int i = 0; i < data.index; i++) {

                remove_channel_in_user_channels(userChannels, data.channels[i]);
                ChannelUsers *channelUsers = find_channel_users(data.session, data.channels[i]);
                remove_channel_users(data.session, channelUsers);
                remove_channel_from_hash_table(data.session, data.channels[i]);
            }

            iterate_list(get_channels_from_user_channels(userChannels), add_message_to_channel, fwdMessage);

            remove_user_channels(get_session(tcpServer), userChannels);

        }
        remove_user_from_hash_table(get_session(tcpServer), user);
    }
    add_message_to_queue(tcpServer, client, message);
}

STATIC void cmd_unknown(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

}

// parse received messages
void parse_message(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    if (tcpServer == NULL || client == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    // input format "<COMMAND> [param 1] ... [param n] [:notice]"
    char *input = get_client_inbuffer(client);

    strcpy(cmdTokens->input, input);

    int tkCount = count_tokens(cmdTokens->input, ':');

    const char *tokens[MAX_TOKENS] = {NULL};

    split_input_string(cmdTokens->input, tokens, tkCount, ' ');

    cmdTokens->command = tokens[0];

    for (int i = 0; i < tkCount - 1; i++) {
        cmdTokens->args[i] = tokens[i+1];
    }
}

CommandFunction get_command_function(CommandType commandType) {

    if (is_valid_command(commandType)) {
        return COMMAND_FUNCTIONS[commandType];
    }
    return cmd_unknown;
}


/*  Message formats
    - server response (when client sends an invalid command):
        " <:prefix> <response code> <param 1> ... [param n] [:response message]"
        example: ":irc.server.com 431 * :No nickname given"
    - forwarded message (when client sends messsage to the channel or another user directly): 
        " <:prefix> <message>"
        example: "john!john@irc.client.com PRIVMSG #general :Hello"  */

STATIC void create_fwd_message(char *buffer, int size, const char *prefix, const char *body, const char *suffix) {

    if (buffer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char msgPrefix[MAX_CHARS + 1] = {'\0'};  
    prepend_char(msgPrefix, MAX_CHARS, prefix, ':');

    const char *noticeTokens[] = {msgPrefix, body, suffix};  

    concat_tokens(buffer, size, noticeTokens, ARR_SIZE(noticeTokens), " ");

}

STATIC void create_response(char *buffer, int size, const char *prefix, const char *body, ResponseType responseType) {

    if (buffer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    char msgPrefix[MAX_CHARS + 1] = {'\0'};  
    prepend_char(msgPrefix, MAX_CHARS, prefix, ':');

    char responseCode[MAX_RESPONSE_CODE_LEN + 1];
    uint_to_str(responseCode, MAX_RESPONSE_CODE_LEN + 1, get_response_code(responseType));

    char msgSuffix[MAX_CHARS + 1] = {'\0'}; 
    prepend_char(msgSuffix, MAX_CHARS, get_response_message(get_response_code(responseType)), ':');

    const char *msgTokens[] = {msgPrefix, responseCode, body, msgSuffix};  

    concat_tokens(buffer, size, msgTokens, ARR_SIZE(msgTokens), " ");
}

STATIC void create_client_info(char *buffer, int size, const char *nickname, const char *username, const char *hostname) {

    if (buffer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    const char *tokens[] = {nickname, "!", username, "@", hostname};  

    concat_tokens(buffer, size, tokens, ARR_SIZE(tokens), "");
}