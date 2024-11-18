#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>

/* a fixed-size generic queue data structure, 
    implemented as a circular buffer */
typedef struct Queue Queue;

Queue * create_queue(int capacity, size_t itemSize);
void delete_queue(Queue *queue);

int is_queue_empty(Queue *queue);
int is_queue_full(Queue *queue);

void enqueue(Queue *queue, void *item);
void * dequeue(Queue *queue);

/* below functions access the queue items without 
    removing them from the queue. the item is 
    identified relative to the currently active item */
void * get_previous_item(Queue *queue);
void * get_next_item(Queue *queue);
void * get_current_item(Queue *queue);

#endif
