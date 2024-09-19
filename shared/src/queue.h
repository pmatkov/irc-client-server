#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>

typedef struct Queue Queue;

Queue * create_queue(int capacity, size_t itemSize);
void delete_queue(Queue *queue);

int is_queue_empty(Queue *queue);
int is_queue_full(Queue *queue);

void enqueue(Queue *queue, void *item);
void * dequeue(Queue *queue);

void * get_previous_item(Queue *queue);
void * get_next_item(Queue *queue);
void * get_current_item(Queue *queue);

#endif
