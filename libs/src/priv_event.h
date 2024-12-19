/* --INTERNAL HEADER--
   used for testing */
#ifndef EVENT_H
#define EVENT_H

#include "data_type.h"
#include "priv_queue.h"

#include <pthread.h>
#include <stdbool.h>

typedef enum {
    UI_EVENT,
    NETWORK_EVENT,
    SYSTEM_EVENT,
    UNKNOWN_EVENT_TYPE,
    EVENT_TYPE_COUNT
} EventType;

typedef enum {
    UI_KEY,
    UI_WIN_RESIZE,
    UNKNOWN_UI_EVENT_TYPE,
    UI_EVENT_TYPE_COUNT
} UIEventType;

typedef enum {
    NE_CLIENT_CONNECT,
    NE_CLIENT_DISCONNECT,
    NE_ADD_POLL_FD,
    NE_REMOVE_POLL_FD,
    NE_PEER_CLOSE,
    NE_SERVER_MSG,
    NE_CLIENT_MSG,
    NE_SEND_IMMEDIATE_MSG,
    UNKNOWN_NETWORK_EVENT_TYPE,
    NETWORK_EVENT_TYPE_COUNT
} NetworkEventType;

typedef enum {
    SE_TIMER,
    SE_EXIT,
    UNKNOWN_SYSTEM_EVENT_TYPE,
    SYSTEM_EVENT_TYPE_COUNT
} SystemEventType;

typedef struct {
    EventType eventType;
    int subEventType;
    DataType dataType;
    DataItem dataItem;
} Event;

typedef void (*EvHandlerFunc)(Event *event);
typedef void (*EvQueueOverflowFunc)(void);

typedef struct {
    EvHandlerFunc baseHandlers[EVENT_TYPE_COUNT];
    EvHandlerFunc uiHandlers[UI_EVENT_TYPE_COUNT];
    EvHandlerFunc networkHandlers[NETWORK_EVENT_TYPE_COUNT];
    EvHandlerFunc systemHandlers[SYSTEM_EVENT_TYPE_COUNT];;
    Queue *eventQueue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;     
    int droppedEvents;
    bool stopProcessing;
} EventManager;

EventManager * create_event_manager(int capacity);
void delete_event_manager(EventManager *eventManager);

Event * create_event(EventType type, int subEventType, DataItem dataItem, DataType dataType);
void delete_event(Event *event);

void register_base_event_handler(EventManager *manager, EventType type, EvHandlerFunc handler);
void register_ui_event_handler(EventManager *manager, UIEventType type, EvHandlerFunc handler);
void register_network_event_handler(EventManager *manager, NetworkEventType type, EvHandlerFunc handler);
void register_system_event_handler(EventManager *manager, SystemEventType type, EvHandlerFunc handler);

void dispatch_base_event(EventManager *manager, Event *event);
void dispatch_ui_event(EventManager *manager, Event *event);
void dispatch_system_event(EventManager *manager, Event *event);
void dispatch_network_event(EventManager *manager, Event *event);

void push_event_to_queue(EventManager *manager, Event *event);
Event * pop_event_from_queue(EventManager *manager);

EventType get_event_type(Event *event);
DataItem * get_event_data_item(Event *event);

const char ** get_event_type_strings(void);

#ifdef TEST

void reset_event_handlers(EventManager *eventManager);

#endif

#endif
