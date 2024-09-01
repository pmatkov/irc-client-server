#include "session.h"
#include "../../shared/src/queue.h"
#include "../../shared/src/error_control.h"
#include "../../shared/src/logger.h"

#include <stdlib.h>
#include <string.h>

#define MSG_QUEUE_LEN 10
#define MAX_SERVER_NAME 50
#define MAX_CHANNEL_NAME 50

struct Session {
    char serverName[MAX_SERVER_NAME + 1];
    char channelName[MAX_CHANNEL_NAME + 1];
    MessageQueue *inQueue;
    MessageQueue *outQueue;
    int fd;
    int connected;
    int inChannel;
};

Session * create_session(void) {

    Session *session = (Session *) malloc(sizeof(Session));
    if (session == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    session->inQueue = create_message_queue(REGULAR_MSG, MSG_QUEUE_LEN);

    session->outQueue = create_message_queue(REGULAR_MSG, MSG_QUEUE_LEN);

    session->fd = -1;
    session->connected = 0;
    session->inChannel = 0;

    return session;
}

void delete_session(Session *session) {

    if (session != NULL) {

        delete_message_queue(session->inQueue);
        delete_message_queue(session->outQueue);
    }
    free(session);
}

MessageQueue * session_get_inqueue(Session *session) {

    if (session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return session->inQueue;
}

MessageQueue * session_get_outqueue(Session *session) {

    if (session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return session->outQueue;
}

int session_get_fd(Session *session) {

    if (session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return session->fd;
}

void session_set_fd(Session *session, int fd) {
      
    if (session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    session->fd = fd;
}

int session_is_connected(Session *session) {

    if (session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return session->connected;
}

void session_set_connected(Session *session, int connected) {

    if (session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    session->connected = connected;
}

int session_is_inchannel(Session *session) {

    if (session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return session->inChannel;
}

void session_set_inchannel(Session *session, int inChannel) {

    if (session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    session->inChannel = inChannel;
}

const char * session_get_server_name(Session *session) {

    if (session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return session->serverName;
}

void session_set_server_name(Session *session, const char *serverName) {

    if (session == NULL || serverName == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (strnlen(serverName, MAX_SERVER_NAME + 1) != MAX_SERVER_NAME + 1) {

        strcpy(session->serverName, serverName);
    }
}

const char * session_get_channel_name(Session *session) {

    if (session == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    return session->channelName;
}


void session_set_channel_name(Session *session, const char *channelName) {

    if (session == NULL || channelName == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (strnlen(channelName, MAX_CHANNEL_NAME + 1) != MAX_CHANNEL_NAME + 1) {

        strcpy(session->channelName, channelName);
    }
}

void add_message(MessageQueue *queue, const char *content) {

    if (queue == NULL || content == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    RegMessage message;

    if (set_reg_message(&message, content)) {
        enqueue(queue, &message);
    }
}

void remove_message(MessageQueue *queue) {

    if (queue == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    dequeue(queue);
}