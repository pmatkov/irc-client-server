#include "session.h"
#include "../../shared/src/errorctrl.h"
#include "../../shared/src/logger.h"

#define MSG_QUEUE_LEN 10

static ClientSession *clientSession;

ClientSession * create_client_session(void) {

    ClientSession *clientSession = (ClientSession *) malloc(sizeof(ClientSession));
    if (clientSession == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    clientSession->inMsgQueue = create_message_queue(REGULAR_MSG, MSG_QUEUE_LEN);

    clientSession->outMsgQueue = create_message_queue(REGULAR_MSG, MSG_QUEUE_LEN);

    clientSession->usedInQueue = 0;
    clientSession->usedOutQueue = 0;

    clientSession->connected = 0;
    clientSession->inChannel = 0;

    return clientSession;
}

void delete_client_session(ClientSession *clientSession) {

    if (clientSession != NULL) {

        delete_message_queue(clientSession->inMsgQueue);
        delete_message_queue(clientSession->outMsgQueue);
    }
    free(clientSession);
}

ClientSession * get_client_session(void) {

    return clientSession;
}
