#include "session_state.h"

#include "common.h"
#include "enum_utils.h"
#include "error_control.h"
#include "logger.h"

#include <assert.h>

static const SessionState *CLIENT_STATES[] = {

    &(SessionState){
        DISCONNECTED, 
        {CONNECTED, UNASSIGNED},
        {HELP, NICK, USER, CONNECT, ADDRESS, PORT, QUIT, UNASSIGNED}
    },
    &(SessionState){
        CONNECTED, 
        {START_REGISTRATION, DISCONNECTED, UNASSIGNED},
        {HELP, NICK, DISCONNECT, ADDRESS, PORT, QUIT, UNASSIGNED}
    },
    &(SessionState){
        START_REGISTRATION,
        {REGISTERED, DISCONNECTED, UNASSIGNED},
        {HELP, USER, DISCONNECT, ADDRESS, PORT, QUIT, UNASSIGNED}
    },
    &(SessionState){
        REGISTERED, 
        {IN_CHANNEL, DISCONNECTED, UNASSIGNED},
        {HELP, NICK, JOIN, PRIVMSG, DISCONNECT, ADDRESS, PORT, WHOIS, QUIT, UNASSIGNED}
    },
    &(SessionState){
        IN_CHANNEL, 
        {REGISTERED, DISCONNECTED, UNASSIGNED},
        {HELP, NICK, JOIN, PRIVMSG, PART, DISCONNECT, ADDRESS, PORT, WHOIS, QUIT, UNASSIGNED}
    },
    NULL      
};

static const SessionState *SERVER_STATES[] = {

    &(SessionState){
        DISCONNECTED, 
        {CONNECTED, UNASSIGNED},
        {CONNECT, UNASSIGNED}
    },
    &(SessionState){
        CONNECTED, 
        {START_REGISTRATION, DISCONNECTED, UNASSIGNED},
        {NICK, QUIT, UNASSIGNED}
    },
    &(SessionState){
        START_REGISTRATION,
        {REGISTERED, DISCONNECTED, UNASSIGNED},
        {USER, QUIT, UNASSIGNED}
    },
    &(SessionState){
        REGISTERED, 
        {IN_CHANNEL, DISCONNECTED, UNASSIGNED},
        {NICK, JOIN, PRIVMSG, WHOIS, QUIT, UNASSIGNED}
    },
    &(SessionState){
        IN_CHANNEL, 
        {REGISTERED, DISCONNECTED, UNASSIGNED},
        {NICK, JOIN, PRIVMSG, PART, WHOIS, QUIT, UNASSIGNED}
    },
    NULL      
};

ASSERT_ARRAY_SIZE(SERVER_STATES, SESSION_STATE_TYPE_COUNT)

bool is_allowed_state_transition(const SessionState **sessionStates, SessionStateType initialState, SessionStateType nextState) {

    if (sessionStates == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    bool allowed = 0;

    if (is_valid_enum_type(initialState, SESSION_STATE_TYPE_COUNT) && is_valid_enum_type(nextState, SESSION_STATE_TYPE_COUNT)) {

        for (int i = 0; i < SESSION_STATE_TYPE_COUNT && sessionStates[initialState]->transitionalStates[i] != UNASSIGNED; i++) {
            if (sessionStates[initialState]->transitionalStates[i] == nextState) {
                allowed = 1;
                break;
            }
        }
    }
    return allowed;
}

bool is_allowed_state_command(const SessionState **sessionStates, SessionStateType initialState, CommandType cmdType) {

    if (sessionStates == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    bool allowed = 0;

    if (is_valid_enum_type(initialState, SESSION_STATE_TYPE_COUNT) && is_valid_enum_type(cmdType, COMMAND_TYPE_COUNT) && sessionStates[initialState] != NULL) {

        for (int i = 0; i < MAX_STATE_CMDS && sessionStates[initialState]->cmdTypes[i] != UNASSIGNED; i++) {
            if (sessionStates[initialState]->cmdTypes[i] == cmdType) {
                allowed = 1;
                break;
            }
        }
    }
    return allowed;
}

const SessionState * get_session_state(const SessionState **sessionStates, SessionStateType initialState) {

    if (sessionStates == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    const SessionState *sessionState = NULL;
    
    if (is_valid_enum_type(initialState, SESSION_STATE_TYPE_COUNT)) {
        sessionState = sessionStates[initialState];
    }

    return sessionState;
}

const SessionState ** get_client_session_states(void) {

    if (CLIENT_STATES == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return CLIENT_STATES;
}

const SessionState ** get_server_session_states(void) {

    if (SERVER_STATES == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return SERVER_STATES;
}