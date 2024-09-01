#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>

#define MAX_MSG 512
#define MAX_NICKNAME 9
#define MAX_CHANNEL 50

typedef enum {
    REGULAR_MSG,
    EXTENDED_MSG,
    DATA_TYPE_COUNT
} DataType;

typedef struct {
    char content[MAX_MSG + 1];
} RegMessage;

typedef struct {
    char sender[MAX_NICKNAME + 1];
    char recipient[MAX_CHANNEL + 1];
    char content[MAX_MSG + 1];
} ExtMessage;

typedef struct {
    void *messages;
    DataType dataType; 
    size_t itemSize;
    int head;
    int tail;
    int searchIndex;
    int allocatedSize;
    int usedSize;
} MessageQueue;

MessageQueue * create_message_queue(DataType dataType, int allocationSize);
void delete_message_queue(MessageQueue *messageQueue);

int mq_is_empty(MessageQueue *messageQueue);
int mq_is_full(MessageQueue *messageQueue);

char get_char_from_message(void *message, int index);
void set_char_in_message(void *message, char ch, int index);
int set_reg_message(void *message, const char *content);
int set_ext_message(void *message, const char *sender, const char *recipient, const char *content);
char *get_message_content(void *message);

void enqueue(MessageQueue *messageQueue, void *message);
void *dequeue(MessageQueue *messageQueue);
void *get_message(MessageQueue *messageQueue, int direction);

#ifdef TEST

int is_valid_data_type(DataType dataType);
size_t get_type_size(DataType dataType);

#endif

#endif
