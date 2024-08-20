#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_NICKNAME_LEN 9
#define MAX_CHANNEL_LEN 50
#define MAX_MSG_LEN 512

typedef struct {
    char content[MAX_MSG_LEN + 1];
} RegMessage;

typedef struct {
    char sender[MAX_NICKNAME_LEN + 1];
    char recipient[MAX_CHANNEL_LEN + 1];
    char content[MAX_MSG_LEN + 1];
} ExtMessage;

typedef enum {
    REGULAR_MSG,
    EXTENDED_MSG,
    DATA_TYPE_COUNT
} DataType;

typedef struct {
    void *messages;
    DataType dataType; 
    size_t itemSize;
    int head;
    int tail;
    int allocatedSize;
    int usedSize;
} MessageQueue;

MessageQueue * create_message_queue(DataType dataType, int allocationSize);
void delete_message_queue(MessageQueue *messageQueue);
int is_empty(MessageQueue *messageQueue);
int is_full(MessageQueue *messageQueue);
int set_reg_message(void *message, char *content);
int set_ext_message(void *message, char *sender, char *recipient, char *content);
int enqueue(MessageQueue *messageQueue, void *message);
void *dequeue(MessageQueue *messageQueue);

#ifdef TEST

STATIC int is_valid_data_type(DataType dataType);
STATIC size_t get_type_size(DataType dataType);

#endif

#endif
