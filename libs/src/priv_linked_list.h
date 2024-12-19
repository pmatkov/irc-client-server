/* --INTERNAL HEADER--
   used for testing */
#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdbool.h>

typedef bool (*ComparatorFunc)(void *data1, void *data2);
typedef void (*DeleteDataFunc)(void *data);
typedef void (*LinkedListFunc)(void *data, void *arg);

typedef struct Node {
    void *data;
    struct Node *next;
} Node;

typedef struct LinkedList {
    Node *head;
    Node *iterator;
    int count;
    ComparatorFunc comparatorFunc;
    DeleteDataFunc deleteDataFunc;
} LinkedList;

LinkedList * create_linked_list(ComparatorFunc comparatorFunc, DeleteDataFunc deleteDataFunc);
void delete_linked_list(LinkedList *linkedList);

void reset_linked_list(LinkedList *linkedList);

Node * create_node(void *data);

void append_node(LinkedList *linkedList, Node *node);
int remove_node(LinkedList *linkedList, void *data);
Node * find_node(LinkedList *linkedList, void *data);

void reset_iterator(LinkedList *linkedList);
Node * iterator_next(LinkedList *linkedList);
void iterate_list(LinkedList *linkedList, LinkedListFunc iteratorFunc, void *arg);

void * get_data(Node *node);
void set_data(Node *node, void *data);
Node * get_next(Node *node);

int get_list_count(LinkedList *linkedList);

#ifdef TEST

void delete_node(Node *node, DeleteDataFunc deleteDataFunc);
void delete_all_nodes(LinkedList *linkedList);

#endif

#endif