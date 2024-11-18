#ifdef TEST
#include "priv_queue.h"
#else
#include "queue.h"
#endif

#include "string_utils.h"
#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

/* in addition to standard fields, this queue
    also has a cursor that can change the 
    position of the active item, apart from
    the standard operations that occur at 
    the front or rear of the queue */

struct Queue {
    void *items;
    size_t itemSize;
    int front;
    int rear;
    int currentItem;
    int capacity;
    int count;
};

#endif

Queue * create_queue(int capacity, size_t itemSize) {

    Queue *queue = (Queue*) malloc(sizeof(Queue));
    if (queue == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");
    }

    queue->items = (void*) malloc(capacity * itemSize);
    if (queue->items == NULL) {
        FAILED(NO_ERRCODE, "Error allocating memory");
    }

    queue->itemSize = itemSize;
    queue->front = 0;
    queue->rear = 0;
    queue->currentItem = 0;
    queue->capacity = capacity;
    queue->count = 0;

    return queue;
}

void delete_queue(Queue *queue) {

    if (queue != NULL) {
        free(queue->items);
    }
    free(queue); 
}

int is_queue_empty(Queue *queue) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return queue->count == 0;
}

int is_queue_full(Queue *queue) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return queue->count == queue->capacity;
}

void enqueue(Queue *queue, void *item) {

    if (queue == NULL || item == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    unsigned char *target = (unsigned char *)queue->items + queue->rear * queue->itemSize;

    memcpy(target, item, queue->itemSize);

    if (is_queue_full(queue)) {
        queue->front = (queue->front + 1) % queue->capacity;

    }
    else {
        queue->count++;
    }

    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->currentItem = queue->rear;

}

void * dequeue(Queue *queue) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    unsigned char *message = NULL;

    if (!is_queue_empty(queue)) {

        message = (unsigned char *)queue->items + queue->front * queue->itemSize;

        queue->front = (queue->front + 1) % queue->capacity;
        queue->count--;
    }

    return message;

}

/* get the previous item relative to the 
    current item in the queue */
void * get_previous_item(Queue *queue) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    unsigned char *message;

    if (queue->currentItem == queue->front) {
        message = NULL;
    }
    else {

        queue->currentItem = (queue->currentItem - 1) % queue->capacity;
  
        message = (unsigned char *)queue->items + queue->currentItem * queue->itemSize;
    }

    return message;
}

/* get the next item relative to the 
    current item in the queue */
void * get_next_item(Queue *queue) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    unsigned char *item;

    if (queue->currentItem == queue->rear) {
        item = NULL;
    }
    else {

        queue->currentItem = (queue->currentItem + 1) % queue->capacity;

        item = (unsigned char *)queue->items + queue->currentItem * queue->itemSize;
    }

    return item;
}

/* get the current item in the queue 
    (defined by the value of the 
    currentItem field) */
void * get_current_item(Queue *queue) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return (unsigned char *)queue->items + queue->currentItem * queue->itemSize;
}