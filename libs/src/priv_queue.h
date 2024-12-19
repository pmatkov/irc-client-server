/* --INTERNAL HEADER--
   used for testing */
#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    void *items;
    size_t itemSize;
    int front;
    int rear;
    int currentIdx;
    int capacity;
    int count;
} Queue;

Queue * create_queue(int capacity, int itemSize);
void delete_queue(Queue *queue);

bool is_queue_empty(Queue *queue);
bool is_queue_full(Queue *queue);

void enqueue(Queue *queue, void *item);
void * dequeue(Queue *queue);

void * get_previous_item(Queue *queue);
void * get_next_item(Queue *queue);
void * get_current_item(Queue *queue);

void * get_item_at_idx(Queue *queue, int idx);

int get_queue_capacity(Queue *queue);

#endif
