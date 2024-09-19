#ifndef LINKED_LIST_H
#define LINKED_LIST_H

typedef int (*ComparatorFunc)(void *data1, void *data2);
typedef void (*DeleteDataFunc)(void *data);
typedef void (*IteratorFunction)(void *data, void *arg);

typedef struct Node {
    void *data;
    struct Node *next;
} Node;

typedef struct LinkedList {
    Node *head;
    int count;
    ComparatorFunc comparatorFunc;
    DeleteDataFunc deleteDataFunc;
} LinkedList;


LinkedList * create_linked_list(ComparatorFunc comparatorFunc, DeleteDataFunc deleteDataFunc);
void delete_linked_list(LinkedList *linkedList);

Node * create_node(void *data);

void append_node(LinkedList *linkedList, Node *node);
int remove_node(LinkedList *linkedList, void *data);
Node * find_node(LinkedList *linkedList, void *data);

void iterate_list(LinkedList *linkedList, IteratorFunction iteratorFunction, void *arg);

void * get_data(Node *node);
void set_data(Node *node, void *data);
Node * get_next(Node *node);

#ifdef TEST

void delete_node(Node *node, DeleteDataFunc deleteDataFunc);

#endif

#endif