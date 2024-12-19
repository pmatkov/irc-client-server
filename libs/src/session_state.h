#ifndef SESSION_STATE_H
#define SESSION_STATE_H

#include "command.h"

#include <stdbool.h>

#define MAX_STATE_CMDS 12

typedef enum {
    DISCONNECTED,
    CONNECTED,
    START_REGISTRATION,
    REGISTERED,
    IN_CHANNEL,
    UNKNOWN_SESSION_STATE_TYPE,
    SESSION_STATE_TYPE_COUNT
} SessionStateType;

typedef struct {
    SessionStateType initialState;
    SessionStateType transitionalStates[SESSION_STATE_TYPE_COUNT];
    CommandType cmdTypes[MAX_STATE_CMDS];
} SessionState;

bool is_allowed_state_transition(const SessionState **sessionStates, SessionStateType initialState, SessionStateType nextState);
bool is_allowed_state_command(const SessionState **sessionStates, SessionStateType initialState, CommandType cmdType);

const SessionState * get_session_state(const SessionState **sessionStates, SessionStateType initialState);
const SessionState ** get_client_session_states(void);
const SessionState ** get_server_session_states(void);

#endif