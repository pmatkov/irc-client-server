#ifndef SESSION_H
#define SESSION_H

#include "queue.h"

#define MAX_SERVER_LEN 50
#define MAX_CHANNEL_LEN 50

typedef struct {
    MessageQueue *inMsgQueue;
    MessageQueue *outMsgQueue;
    int usedInQueue;
    int usedOutQueue;
    char serverName[MAX_SERVER_LEN + 1];
    char channelName[MAX_CHANNEL_LEN + 1];
    int connected;
    int inChannel;
} ClientSession;

ClientSession * create_client_session(void);
void delete_client_session(ClientSession *clientSession);
ClientSession * get_client_session(void);

#endif