#ifdef TEST
#include "priv_command_handler.h"
#else
#include "command_handler.h"
#include "../../shared/src/response_code.h"
#endif

#include "user.h"
#include "channel.h"
#include "session.h"
#include "tcpserver.h"
#include "../../shared/src/settings.h"
#include "../../shared/src/linked_list.h"
#include "../../shared/src/priv_message.h"
#include "../../shared/src/string_utils.h"
#include "../../shared/src/logger.h"
#include "../../shared/src/error_control.h"

#include <stddef.h>
#include <string.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_NICKNAME_LEN 9

STATIC void cmd_nick(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_user(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_join(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_part(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_privmsg(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_quit(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);
STATIC void cmd_unknown(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens);

STATIC void create_client_info(char *buffer, int size, const char *nickname, const char *username, const char *hostname);

static const CommandFunction COMMAND_FUNCTIONS[] = {
    NULL,
    NULL,
    NULL,
    cmd_nick,
    cmd_user,
    cmd_join,
    cmd_part,
    cmd_privmsg,
    cmd_quit,
    cmd_unknown
};

_Static_assert(sizeof(COMMAND_FUNCTIONS) / sizeof(COMMAND_FUNCTIONS[0]) == COMMAND_TYPE_COUNT, "Array size mismatch");

STATIC void cmd_nick(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + 1] = {'\0'};
    const char *servername = get_property_value(HOSTNAME);

    if (!cmdTokens->argCount) {

        // :server 431 * :No nickname given
        const char *code = get_response_code(ERR_NONICKNAMEGIVEN);
        create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, "*"}, {get_response_message(code)}, 1});
    }
    else if (cmdTokens->argCount >= 1) {

        if (strlen(cmdTokens->args[0]) > MAX_NICKNAME_LEN || !is_valid_name(cmdTokens->args[0], 0)) {

            // :server 432 <nickname> :Erroneous nickname
            const char *code = get_response_code(ERR_ERRONEUSNICKNAME);
            create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, cmdTokens->args[0]}, {get_response_message(code)}, 1});
        }
        else if (find_user_in_hash_table(get_session(tcpServer), cmdTokens->args[0]) != NULL) {

            // :server 433 <nickname> :Nickname is already in use
            const char *code = get_response_code(ERR_NICKNAMEINUSE);
            create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, cmdTokens->args[0]}, {get_response_message(code)}, 1});
        }
        else {

            if (is_client_registered(client)) {

                User *user = find_user_in_hash_table(get_session(tcpServer), get_client_nickname(client));
                User *newUser = create_user(cmdTokens->args[0], get_username(user), get_hostname(user), get_realname(user), get_client_fd(client));

                UserChannels *userChannels = find_user_channels(get_session(tcpServer), user);

                 // notify channels of nickname change
                if (userChannels != NULL) {

                    char clientInfo[MAX_CHARS + 1] = {'\0'};
                    create_client_info(clientInfo, MAX_CHARS, get_user_nickname(user), get_username(user), get_hostname(user));

                    char fwdMessage[MAX_CHARS + 1] = {'\0'};

                    create_message(fwdMessage, MAX_CHARS, &(MessageTokens){{clientInfo}, {cmdTokens->command}, {cmdTokens->args[0]}, 0});

                    iterate_list(get_channels_from_user_channels(userChannels), add_message_to_channel_queue, fwdMessage);
                    iterate_list(get_channels_from_user_channels(userChannels), add_channel_to_ready_channels, get_ready_list(get_session(tcpServer)));

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
    const char *servername = get_property_value(HOSTNAME);
    const char *nickname = get_client_nickname(client);

    if (!strlen(get_client_nickname(client))) {

        // :server 451 * :You have not registered
        const char *code = get_response_code(ERR_NOTREGISTERED);
        create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, "*"}, {get_response_message(code)}, 1});
    }
    else if (is_client_registered(client)) {

        // :server 462 <nickname> :Already registered
        const char *code = get_response_code(ERR_ALREADYREGISTRED);
        create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname}, {get_response_message(code)}, 1});
    }
    else {
        if (cmdTokens->argCount < 4) {

            // create_message(message, MAX_CHARS, servername, msgBody, ERR_NEEDMOREPARAMS);
            const char *code = get_response_code(ERR_NEEDMOREPARAMS);
            create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->command}, {get_response_message(code)}, 1});
        }
        else {

            User *user = create_user(nickname, cmdTokens->args[0], cmdTokens->args[1], cmdTokens->args[3], get_client_fd(client));

            add_user_to_hash_table(get_session(tcpServer), user);

            UserChannels *userChannels = create_user_channels(user);
            add_user_channels(get_session(tcpServer), userChannels);

            set_client_registered(client, 1);

            // :server 001 <nickname> :Welcome to the IRC Network
            const char *code = get_response_code(RPL_WELCOME);
            create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname}, {get_response_message(code)}, 1});
        }
    }
    add_message_to_queue(tcpServer, client, message);
}

STATIC void cmd_join(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + 1] = {'\0'};
    const char *servername = get_property_value(HOSTNAME);
    const char *nickname = get_client_nickname(client);

    if (!is_client_registered(client)) {

        const char *code = get_response_code(ERR_NOTREGISTERED);
        create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, "*"}, {get_response_message(code)}, 1});
    }
    else {

        if (!cmdTokens->argCount) {

            // :server 461 <nickname> <cmd> :Not enough parameters
            const char *code = get_response_code(ERR_NEEDMOREPARAMS);
            create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->command}, {get_response_message(code)}, 1});
        }
        else {

            User *user = find_user_in_hash_table(get_session(tcpServer), nickname);

            char clientInfo[MAX_CHARS + 1] = {'\0'};
            create_client_info(clientInfo, MAX_CHARS, get_user_nickname(user), get_username(user), get_hostname(user));

            char fwdMessage[MAX_CHARS + 1] = {'\0'};

            // <:nickname!username@hostname> JOIN <channel>
            create_message(fwdMessage, MAX_CHARS, &(MessageTokens){{clientInfo}, {cmdTokens->command}, {cmdTokens->args[0]}, 0});

            UserChannels *userChannels = find_user_channels(get_session(tcpServer), user);
            Channel *channel = find_channel_in_hash_table(get_session(tcpServer), cmdTokens->args[0]);

            if (channel != NULL) {

                ChannelUsers *channelUsers = find_channel_users(get_session(tcpServer), channel);

                if (find_user_in_channel_users(channelUsers, user) == NULL) {

                    if (is_channel_full(channelUsers)) {

                        // :server 471 <nickname> <channel> :Cannot join channel
                        const char *code = get_response_code(ERR_CHANNELISFULL);
                        create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->args[0]}, {get_response_message(code)}, 1});
                    }
                    else {

                        add_user_to_channel_users(channelUsers, user);
                        add_channel_to_user_channels(userChannels, channel);
                        add_message_to_channel_queue(channel, fwdMessage);
        
                        char topicMessage[MAX_CHARS + 1] = {'\0'};

                        if (!strlen(get_channel_topic(channel))) {

                            // :server 331 <nickname> <channel> :No topic is set
                            const char *code = get_response_code(RPL_NOTOPIC);
                            create_message(topicMessage, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->args[0]}, {get_response_message(code)}, 1});
                        }
                        else {

                            // :server 332 <nickname> <channel> :<topic>
                            const char *code = get_response_code(RPL_TOPIC);
                            create_message(topicMessage, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->args[0]}, {get_channel_topic(channel)}, 1});

                        }
                        add_message_to_queue(tcpServer, client, topicMessage);

                        int usersCount = get_users_count_from_channel_users(channelUsers);
                        StringList *stringlist = create_string_list(usersCount, MAX_NICKNAME_LEN);

                        iterate_list(get_users_from_channel_users(channelUsers), add_nickname_to_list, stringlist);

                        char names[MAX_CHARS + 1] = {'\0'}; 
                        concat_tokens(names, MAX_CHARS + 1, (const char**) stringlist->strings, stringlist->count, " ");

                        delete_string_list(stringlist);

                        char namesMessage1[MAX_CHARS + 1] = {'\0'};

                        // :server 353 <nickname> <channel> :<nicknames list>
                        const char *code = get_response_code(RPL_NAMREPLY);
                        create_message(namesMessage1, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->args[0]}, {names}, 1});

                        add_message_to_queue(tcpServer, client, namesMessage1);

                        char namesMessage2[MAX_CHARS + 1] = {'\0'};

                        // :server 366 <nickname> <channel> :<End of NAMES list>
                        code = get_response_code(RPL_ENDOFNAMES);
                        create_message(namesMessage2, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->args[0]}, {get_response_message(code)}, 1});

                        add_message_to_queue(tcpServer, client, namesMessage2);
                    }
                }
            }
            else {

                if (strlen(cmdTokens->args[0]) > MAX_CHANNEL_LEN || !is_valid_name(cmdTokens->args[0], 1)) {

                    // :server 479 <nickname> <channel> :Illegal channel name
                    const char *code = get_response_code(ERR_BADCHANNAME);
                    create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->args[0]}, {get_response_message(code)}, 1});
                }
                else {

                    Channel *newChannel = create_channel(cmdTokens->args[0], NULL, TEMPORARY, MAX_USERS_PER_CHANNEL);
                    add_channel_to_hash_table(get_session(tcpServer), newChannel);

                    ChannelUsers *channelUsers = create_channel_users(newChannel);
                    add_channel_users(get_session(tcpServer), channelUsers);

                    add_user_to_channel_users(channelUsers, user);
                    add_channel_to_user_channels(userChannels, newChannel);
                    add_message_to_channel_queue(newChannel, fwdMessage);

                    // :server 331 <nickname> <channel> :No topic is set
                    const char *code = get_response_code(RPL_NOTOPIC);
                    create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->args[0]}, {get_response_message(code)}, 1});

                }
            }

            if (channel != NULL && !is_queue_empty(get_channel_queue(channel))) {
                add_channel_to_ready_channels(channel, get_ready_list(get_session(tcpServer)));
            }
        }
    }

    add_message_to_queue(tcpServer, client, message);
}

STATIC void cmd_part(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + 1] = {'\0'};
    const char *servername = get_property_value(HOSTNAME);
    const char *nickname = get_client_nickname(client);

    if (!is_client_registered(client)) {

        const char *code = get_response_code(ERR_NOTREGISTERED);
        create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, "*"}, {get_response_message(code)}, 1});
    }
    else {

        if (!cmdTokens->argCount) {

            // :server 461 <nickname> <cmd> :Not enough parameters
            const char *code = get_response_code(ERR_NEEDMOREPARAMS);
            create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->command}, {get_response_message(code)}, 1});
        }
        else {
            User *user = find_user_in_hash_table(get_session(tcpServer), nickname);
            Channel *channel = find_channel_in_hash_table(get_session(tcpServer), cmdTokens->args[0]);

            char clientInfo[MAX_CHARS + 1] = {'\0'};
            create_client_info(clientInfo, MAX_CHARS, get_user_nickname(user), get_username(user), get_hostname(user));

            char fwdMessage[MAX_CHARS + 1] = {'\0'};

            if (cmdTokens->argCount == 1) {

                // <nickname!username@hostname> PART <channel_name>
                create_message(fwdMessage, MAX_CHARS, &(MessageTokens){{clientInfo}, {cmdTokens->command}, {cmdTokens->args[0]}, 0});
            }
            else {
                // <:nickname!username@hostname> PART <channel_name> <:message>
                create_message(fwdMessage, MAX_CHARS, &(MessageTokens){{clientInfo}, {cmdTokens->command, cmdTokens->args[0]}, {cmdTokens->args[1]}, 0});
            }
            
            if (channel == NULL) {

                // :server 403 <nickname> <channel> :No such channel
                const char *code = get_response_code(ERR_NOSUCHCHANNEL);
                create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->args[0]}, {get_response_message(code)}, 1});
            }
            else {

                ChannelUsers *channelUsers = find_channel_users(get_session(tcpServer), channel);

                if (find_user_in_channel_users(channelUsers, user) == NULL) {

                    // :server 442 <nickname> <channel> :You're not on that channel
                    const char *code = get_response_code(ERR_NOTONCHANNEL);
                    create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->args[0]}, {get_response_message(code)}, 1});
                }
                else {

                    remove_user_in_channel_users(channelUsers, user);

                    if (get_channel_type(channel) == TEMPORARY && !get_users_count_from_channel_users(channelUsers)) {

                        remove_channel_users(get_session(tcpServer), channelUsers);
                        remove_channel_from_hash_table(get_session(tcpServer), channel);
                    }
                    else {
                        add_message_to_channel_queue(channel, fwdMessage);
                    }

                    UserChannels *userChannels = find_user_channels(get_session(tcpServer), user);

                    remove_channel_in_user_channels(userChannels, channel);

                }
            }

            if (channel != NULL && !is_queue_empty(get_channel_queue(channel))) {
                add_channel_to_ready_channels(channel, get_ready_list(get_session(tcpServer)));
            }
        }
    }
    add_message_to_queue(tcpServer, client, message);
}

STATIC void cmd_privmsg(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + 1] = {'\0'};
    const char *servername = get_property_value(HOSTNAME);
    const char *nickname = get_client_nickname(client);

    if (!is_client_registered(client)) {

        const char *code = get_response_code(ERR_NOTREGISTERED);
        create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, "*"}, {get_response_message(code)}, 1});
    }
    else {

        if (cmdTokens->argCount < 2) {

            // :server 461 <nickname> <cmd> :Not enough parameters
            const char *code = get_response_code(ERR_NEEDMOREPARAMS);
            create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->command}, {get_response_message(code)}, 1});
        }
        else {

            User *user = find_user_in_hash_table(get_session(tcpServer), nickname);

            char clientInfo[MAX_CHARS + 1] = {'\0'};
            create_client_info(clientInfo, MAX_CHARS, get_user_nickname(user), get_username(user), get_hostname(user));
                        
            char fwdMessage[MAX_CHARS + 1] = {'\0'};

            // <:nickname!username@hostname> PRIVMSG <nickname | channel> <:message>
            create_message(fwdMessage, MAX_CHARS, &(MessageTokens){{clientInfo}, {cmdTokens->command, cmdTokens->args[0]}, {cmdTokens->args[1]}, 0});

            if (cmdTokens->args[0][0] == '#') {

                Channel *channel = find_channel_in_hash_table(get_session(tcpServer), cmdTokens->args[0]);

                if (channel == NULL) {

                    // :server 403 <nickname> <channel> :No such channel
                    const char *code = get_response_code(ERR_NOSUCHCHANNEL);
                    create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->args[0]}, {get_response_message(code)}, 1});
                }
                else {

                    ChannelUsers *channelUsers = find_channel_users(get_session(tcpServer), channel);

                    if (find_user_in_channel_users(channelUsers, user) == NULL) {

                        // :server 442 <nickname> <channel> :You're not on that channel
                        const char *code = get_response_code(ERR_NOTONCHANNEL);
                        create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->args[0]}, {get_response_message(code)}, 1});
                    }
                    else {
                        add_message_to_channel_queue(channel, fwdMessage);
                    }
                } 

                if (channel != NULL && is_queue_empty(get_channel_queue(channel))) {
                    add_channel_to_ready_channels(channel, get_ready_list(get_session(tcpServer)));
                }
            }
            else {
                User *recipient = find_user_in_hash_table(get_session(tcpServer), cmdTokens->args[0]);

                if (recipient == NULL) {

                    // :server 401 <nickname> <nickname> :No such nick
                    const char *code = get_response_code(ERR_NOSUCHNICK);
                    create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, nickname, cmdTokens->args[0]}, {get_response_message(code)}, 1});
                }
                else {
                    add_message_to_user_queue(recipient, fwdMessage);
                }
            }
        }
    }
    add_message_to_queue(tcpServer, client, message);
}

STATIC void cmd_quit(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    char message[MAX_CHARS + 1] = {'\0'};
    const char *servername = get_property_value(HOSTNAME);
    const char *nickname = get_client_nickname(client);

    if (!is_client_registered(client)) {

        const char *code = get_response_code(ERR_NOTREGISTERED);
        create_message(message, MAX_CHARS, &(MessageTokens){{servername}, {code, "*"}, {get_response_message(code)}, 1});
        add_message_to_queue(tcpServer, client, message);
    }
    else {

        User *user = find_user_in_hash_table(get_session(tcpServer), nickname);

        char clientInfo[MAX_CHARS + 1] = {'\0'};
        create_client_info(clientInfo, MAX_CHARS, get_user_nickname(user), get_username(user), get_hostname(user));

        char fwdMessage[MAX_CHARS + 1] = {'\0'};

        // <:nickname!username@hostname> QUIT <:message>
        create_message(fwdMessage, MAX_CHARS, &(MessageTokens){{clientInfo}, {cmdTokens->command}, {cmdTokens->args[0]}, 0});

        UserChannels *userChannels = find_user_channels(get_session(tcpServer), user);
        
        if (userChannels != NULL) {

            struct Data {
                Session *session;
                Channel *channels[MAX_CHANNELS_PER_USER];
                int count;
            };

            struct Data data = {get_session(tcpServer), {NULL}, 0};

            iterate_list(get_channels_from_user_channels(userChannels), find_removable_channels, &data);

            for (int i = 0; i < data.count; i++) {

                remove_channel_in_user_channels(userChannels, data.channels[i]);
                ChannelUsers *channelUsers = find_channel_users(data.session, data.channels[i]);
                remove_channel_users(data.session, channelUsers);
                remove_channel_from_hash_table(data.session, data.channels[i]);
            }

            iterate_list(get_channels_from_user_channels(userChannels), add_message_to_channel_queue, fwdMessage);
            iterate_list(get_channels_from_user_channels(userChannels), add_channel_to_ready_channels, get_ready_list(get_session(tcpServer)));
            remove_user_channels(get_session(tcpServer), userChannels);
        }
        remove_user_from_hash_table(get_session(tcpServer), user);
        remove_client(tcpServer, find_fd_index(tcpServer, get_client_fd(client)));
    }

}

STATIC void cmd_unknown(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

}

// parse received messages
void parse_message(TCPServer *tcpServer, Client *client, CommandTokens *cmdTokens) {

    if (tcpServer == NULL || client == NULL || cmdTokens == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    // input format "<COMMAND> [param 1] ... [param n] [:notice]"
    char *input = get_client_inbuffer(client);

    strcpy(cmdTokens->input, input);

    int tkCount = count_tokens(cmdTokens->input, ':');

    const char *tokens[MAX_TOKENS] = {NULL};

    split_input_string(cmdTokens->input, tokens, tkCount, ' ');

    cmdTokens->command = tokens[0];
    cmdTokens->argCount = tkCount - 1;

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



STATIC void create_client_info(char *buffer, int size, const char *nickname, const char *username, const char *hostname) {

    if (buffer == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    const char *tokens[] = {nickname, "!", username, "@", hostname};  

    concat_tokens(buffer, size, tokens, ARR_SIZE(tokens), "");
}