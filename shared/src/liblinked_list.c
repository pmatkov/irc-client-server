#ifdef TEST
#include "priv_linked_list.h"
#else
#include "linked_list.h"
#endif

#include "error_control.h"
#include "logger.h"

#include <stdlib.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

struct Node {
    void *data;
    Node *next;
};

struct LinkedList {
    Node *head;
    int count;
    ComparatorFunc comparatorFunc;
    DeleteDataFunc deleteDataFunc;
};

#endif

STATIC void delete_node(Node *node, DeleteDataFunc deleteDataFunc);
STATIC void delete_all_nodes(LinkedList *linkedList);

LinkedList * create_linked_list(ComparatorFunc comparatorFunc, DeleteDataFunc deleteDataFunc) {

    LinkedList *linkedList = (LinkedList*) malloc(sizeof(LinkedList));
    if (linkedList == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    linkedList->head = NULL;
    linkedList->count = 0;
    linkedList->comparatorFunc = comparatorFunc;
    linkedList->deleteDataFunc = deleteDataFunc;

    return linkedList;
}

void delete_linked_list(LinkedList *linkedList) {

    if (linkedList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    delete_all_nodes(linkedList);

    free(linkedList);
}

void reset_linked_list(LinkedList *linkedList) {

    if (linkedList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    delete_all_nodes(linkedList);

    linkedList->count = 0;
}

Node * create_node(void *data) {

    if (data == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    Node *node = (Node*) malloc(sizeof(Node));
    if (node == NULL) {
        FAILED("Error allocating memory", NO_ERRCODE);
    }

    node->data = data;
    node->next = NULL;

    return node;
}

STATIC void delete_node(Node *node, DeleteDataFunc deleteDataFunc) {

    if (node != NULL && deleteDataFunc != NULL) {
        deleteDataFunc(node->data);
    }

    free(node);
}

STATIC void delete_all_nodes(LinkedList *linkedList) {

    if (linkedList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    Node *current = linkedList->head;
    Node *previous = NULL;

    while (current != NULL) {

        previous = current;
        current = current->next;

        delete_node(previous, linkedList->deleteDataFunc);
    }

}

void append_node(LinkedList *linkedList, Node *node) {

    if (linkedList == NULL || node == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    if (linkedList->head == NULL) {
        linkedList->head = node;
    }
    else {

        Node *current = linkedList->head;
        Node *previous = NULL;

        while (current != NULL) {

            previous = current;
            current = current->next;
        }

        previous->next = node;
    }
    linkedList->count++;
}

int remove_node(LinkedList *linkedList, void *data) {

    if (linkedList == NULL || data == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    int removed = 0;

    Node *current = linkedList->head;
    Node *previous = NULL;

    while (current != NULL && !linkedList->comparatorFunc(current->data, data)) {

        previous = current;
        current = current->next;
    }

    if (current != NULL) {

        if (previous == NULL) {

            linkedList->head = linkedList->head->next;
        }
        else {
            previous->next = current->next;
        }
        delete_node(current, linkedList->deleteDataFunc);
        linkedList->count--;
        removed = 1;
    }
    return removed; 
}

Node * find_node(LinkedList *linkedList, void *data) {

    if (linkedList == NULL || data == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    
    Node *current = linkedList->head;

    while (current != NULL && !linkedList->comparatorFunc(current->data, data)) {

        current = current->next;
    }

    return current;
}

void iterate_list(LinkedList *linkedList, IteratorFunc iteratorFunction, void *arg) {

    if (linkedList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    Node *current = linkedList->head;

    while (current != NULL) {

        iteratorFunction(current->data, arg);
        current = current->next;
    }
}

void * get_data(Node *node) {

    void *data = NULL;

    if (node != NULL) {
        data = node->data;
    }

    return data;
}

void set_data(Node *node, void *data) {

    if (node == NULL) {
        FAILED(NULL, ARG_ERROR);
    }
    node->data = data;

}

Node * get_next(Node *node) {

    Node *next = NULL;

    if (node != NULL) {
        next = node->next;
    }

    return next;
}

int get_list_count(LinkedList *linkedList) {

    if (linkedList == NULL) {
        FAILED(NULL, ARG_ERROR);
    }

    return linkedList->count;
}
