#ifndef DISPATCHER_H
#define DISPATCHER_H

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

/* RFCC 2812 1.2.1 Users (each user is distinguished
 from other users by a unique nickname having a
  maximum length of nine (9) characters) */
#define MAX_NICKNAME_LEN 9

/* RFCC 2812 1.3 Channels (channels names are strings
 (beginning with a '&', '#', '+' or '!' character) 
 of length up to fifty (50) characters) */
#define MAX_CHANNEL_LEN 50
#define MAX_MSG_LENGTH 256

#define MSG_QUEUE_LEN 20

typedef struct {
    char sender[MAX_NICKNAME_LEN + 1];
    char recipient[MAX_CHANNEL_LEN + 1];
    char content[MAX_MSG_LENGTH + 1];
} Message;

typedef struct {
    Message *messages;
    int tail;
    int head;
    int allocatedSize;
    int usedSize;
} MessageQueue;

typedef struct {
    char channel_name[MAX_CHANNEL_LEN];
    struct list* users;
    MessageQueue *messageQueue;
} Channel;

MessageQueue * create_message_queue(int size);
void delete_message_queue(MessageQueue *messageQueue);

int is_empty(MessageQueue *messageQueue);
int is_full(MessageQueue *messageQueue) ;

int enqueue(MessageQueue *messageQueue, char *sender, char *recipient, char *content);
Message *dequeue(MessageQueue *messageQueue);

STATIC int set_message(Message *message, char *sender, char *recipient, char *content);

#endif