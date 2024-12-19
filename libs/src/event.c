#ifdef TEST
#include "priv_event.h"
#else
#include "event.h"
#include "queue.h"
#endif

#include "event.h"
#include "common.h"
#include "enum_utils.h"
#include "error_control.h"
#include "logger.h"

#include <stdlib.h>
#include <unistd.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define DEF_QUEUE_CAPACITY 20

#ifndef TEST

struct EventManager {
    EvHandlerFunc baseHandlers[EVENT_TYPE_COUNT];
    EvHandlerFunc uiHandlers[UI_EVENT_TYPE_COUNT];
    EvHandlerFunc networkHandlers[NETWORK_EVENT_TYPE_COUNT];
    EvHandlerFunc systemHandlers[SYSTEM_EVENT_TYPE_COUNT];
    Queue *eventQueue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;    
    int droppedEvents;
    bool stopProcessing;
};

#endif

static const char *EVENT_TYPE_STRINGS[] = {
    "UI event",
    "Network event",
    "System event",
    "Unknown"
};


STATIC void reset_event_handlers(EventManager *eventManager);

EventManager * create_event_manager(int capacity) {

    if (capacity <= 0) {
        capacity = DEF_QUEUE_CAPACITY;
    }

    EventManager *eventManager = (EventManager *) malloc(sizeof(EventManager));
    if (eventManager == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    reset_event_handlers(eventManager);

    eventManager->eventQueue = create_queue(capacity, sizeof(Event));

    pthread_mutex_init(&eventManager->mutex, NULL);
    pthread_cond_init(&eventManager->cond, NULL);

    eventManager->droppedEvents = 0;
    eventManager->stopProcessing = 0;

    return eventManager;

}

void delete_event_manager(EventManager *eventManager) {

    if (eventManager != NULL) {

        delete_queue(eventManager->eventQueue);

        pthread_mutex_destroy(&eventManager->mutex);
        pthread_cond_destroy(&eventManager->cond);
    }
    free(eventManager); 
}

Event * create_event(EventType eventType, int subEventType, DataItem dataItem, DataType dataType) {

    if (!is_valid_enum_type(eventType, EVENT_TYPE_COUNT) || !is_valid_enum_type(dataType, DATA_TYPE_COUNT)) {
        FAILED(ARG_ERROR, NULL);
    }

    Event *event = (Event*) malloc(sizeof(Event));
    if (event == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    event->eventType = eventType;
    event->subEventType = subEventType;
    event->dataItem = dataItem;
    event->dataType = dataType;

    return event;
}

void delete_event(Event *event) {

    free(event);
}

STATIC void reset_event_handlers(EventManager *eventManager) {

    if (eventManager == NULL) {
        FAILED(ARG_ERROR, NO_ERRCODE);
    }

    for (int i = 0; i < EVENT_TYPE_COUNT; i++) {
        eventManager->baseHandlers[i] = NULL;
    }
    for (int i = 0; i < UI_EVENT_TYPE_COUNT; i++) {
        eventManager->uiHandlers[i] = NULL;
    }
    for (int i = 0; i < NETWORK_EVENT_TYPE_COUNT; i++) {
        eventManager->networkHandlers[i] = NULL;
    }
    for (int i = 0; i < SYSTEM_EVENT_TYPE_COUNT; i++) {
        eventManager->systemHandlers[i] = NULL;
    }
}

void register_base_event_handler(EventManager *eventManager, EventType type, EvHandlerFunc handler) {

    if (eventManager == NULL || !is_valid_enum_type(type, EVENT_TYPE_COUNT)) {
        FAILED(ARG_ERROR, NO_ERRCODE);
    }

    eventManager->baseHandlers[type] = handler;
}

void register_ui_event_handler(EventManager *eventManager, UIEventType type, EvHandlerFunc handler) {

    if (eventManager == NULL || !is_valid_enum_type(type, UI_EVENT_TYPE_COUNT))
    {
        FAILED(ARG_ERROR, NO_ERRCODE);
    }
    eventManager->uiHandlers[type] = handler;
}

void register_network_event_handler(EventManager *eventManager, NetworkEventType type, EvHandlerFunc handler) {

    if (eventManager == NULL || !is_valid_enum_type(type, NETWORK_EVENT_TYPE_COUNT))
    {
        FAILED(ARG_ERROR, NO_ERRCODE);
    }
    eventManager->networkHandlers[type] = handler;
}

void register_system_event_handler(EventManager *eventManager, SystemEventType type, EvHandlerFunc handler) {

    if (eventManager == NULL || !is_valid_enum_type(type, SYSTEM_EVENT_TYPE_COUNT)) {
        FAILED(ARG_ERROR, NO_ERRCODE);
    }

    eventManager->systemHandlers[type] = handler;
}

void dispatch_base_event(EventManager *eventManager, Event *event) {

    if (eventManager == NULL || event == NULL) {
        FAILED(ARG_ERROR, NO_ERRCODE);
    }

    if (is_valid_enum_type(event->eventType, EVENT_TYPE_COUNT) && \
        eventManager->baseHandlers[event->eventType] != NULL) {
        eventManager->baseHandlers[event->eventType](event);
    }
}

void dispatch_ui_event(EventManager *eventManager, Event *event) {

    if (eventManager == NULL || event == NULL) {
        FAILED(ARG_ERROR, NO_ERRCODE);
    }

    if (event->eventType == UI_EVENT && \
        eventManager->uiHandlers[event->subEventType] != NULL) {
        eventManager->uiHandlers[event->subEventType](event);
    }
}

void dispatch_network_event(EventManager *eventManager, Event *event) {

    if (eventManager == NULL || event == NULL) {
        FAILED(ARG_ERROR, NO_ERRCODE);
    }

    if (event->eventType == NETWORK_EVENT && \
        eventManager->networkHandlers[event->subEventType] != NULL) {
        eventManager->networkHandlers[event->subEventType](event);
    }
}

void dispatch_system_event(EventManager *eventManager, Event *event) {

    if (eventManager == NULL || event == NULL) {
        FAILED(ARG_ERROR, NO_ERRCODE);
    }

    if (event->eventType == SYSTEM_EVENT && \
        eventManager->systemHandlers[event->subEventType] != NULL) {
        eventManager->systemHandlers[event->subEventType](event);
    }
}

void push_event_to_queue(EventManager *eventManager, Event *event) {

    if (eventManager == NULL || event == NULL) {
        FAILED(ARG_ERROR, NO_ERRCODE);
    }

    enqueue(eventManager->eventQueue, event);
}

Event * pop_event_from_queue(EventManager *eventManager) {

    if (eventManager == NULL) {
        FAILED(ARG_ERROR, NO_ERRCODE);
    }

    return dequeue(eventManager->eventQueue);
}

EventType get_event_type(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NO_ERRCODE);
    }

    return event->eventType;
}

DataItem * get_event_data_item(Event *event) {

    if (event == NULL) {
        FAILED(ARG_ERROR, NO_ERRCODE);
    }

    return &event->dataItem;
}

const char ** get_event_type_strings(void) {

    return EVENT_TYPE_STRINGS;
}