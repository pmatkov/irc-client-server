#ifndef SESSION_H
#define SESSION_H

#include "../../shared/src/queue.h"

typedef struct Session Session;

Session * create_session(void);
void delete_session(Session *session);

const char * session_get_server_name(Session *session);
void session_set_server_name(Session *session, const char *serverName);
const char * session_get_channel_name(Session *session);
void session_set_channel_name(Session *session, const char *channelName);

MessageQueue * session_get_inqueue(Session *session);
MessageQueue * session_get_outqueue(Session *session);
int session_get_fd(Session *session);
void session_set_fd(Session *session, int fd);
int session_is_connected(Session *session);
void session_set_connected(Session *session, int connected);
int session_is_inchannel(Session *session);
void session_set_inchannel(Session *session, int inChannel);

void add_message(MessageQueue *queue, const char *content);
void remove_message(MessageQueue *queue);

#endif