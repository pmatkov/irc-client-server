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
    int itemSize;
    int front;
    int rear;
    int currentIdx;
    int count;
    int capacity;
};

#endif

Queue * create_queue(int capacity, int itemSize) {

    if (capacity <= 0) {
        LOG(NO_ERRCODE, "Invalid capacity");
        return NULL;
    }

    Queue *queue = (Queue*) malloc(sizeof(Queue));
    if (queue == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    queue->items = (void*) malloc(capacity * itemSize);
    if (queue->items == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    queue->itemSize = itemSize;
    queue->front = 0;
    queue->rear = 0;
    queue->currentIdx = 0;
    queue->count = 0;
    queue->capacity = capacity;

    return queue;
}

void delete_queue(Queue *queue) {

    if (queue != NULL) {
        free(queue->items);
    }
    free(queue); 
}

bool is_queue_empty(Queue *queue) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return queue->count == 0;
}

bool is_queue_full(Queue *queue) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return queue->count == queue->capacity;
}

void enqueue(Queue *queue, void *item) {

    if (queue == NULL || item == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    unsigned char *target = (unsigned char *) queue->items + queue->rear * queue->itemSize;

    memcpy(target, item, queue->itemSize);

    if (is_queue_full(queue)) {
        queue->front = (queue->front + 1) % queue->capacity;

    }
    else {
        queue->count++;
    }

    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->currentIdx = queue->rear;
}

void * dequeue(Queue *queue) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    unsigned char *item = NULL;

    if (!is_queue_empty(queue)) {

        item = (unsigned char *)queue->items + queue->front * queue->itemSize;

        queue->front = (queue->front + 1) % queue->capacity;
        queue->count--;
    }

    return item;
}

/* get the previous item relative to the 
    current item in the queue */
void * get_previous_item(Queue *queue) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    unsigned char *item = NULL;

    if (queue->currentIdx > queue->front) {

        queue->currentIdx = (queue->currentIdx - 1) % queue->capacity;
        item = (unsigned char *)queue->items + queue->currentIdx * queue->itemSize;
    }

    return item;
}

/* get the next item relative to the 
    current item in the queue */
void * get_next_item(Queue *queue) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    unsigned char *item = NULL;

    if (queue->currentIdx < queue->rear) {

        queue->currentIdx = (queue->currentIdx + 1) % queue->capacity;
        item = (unsigned char *)queue->items + queue->currentIdx * queue->itemSize;
    }

    return item;
}

/* get the current item in the queue */
void * get_current_item(Queue *queue) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return (unsigned char *)queue->items + queue->currentIdx * queue->itemSize;

}

/* get the item at position idx in the queue */
void * get_item_at_idx(Queue *queue, int idx) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return (unsigned char *)queue->items + idx * queue->itemSize;

}

int get_queue_capacity(Queue *queue) {

    if (queue == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    return queue->capacity;
}
