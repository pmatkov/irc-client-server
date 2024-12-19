#ifdef TEST
#include "priv_poll_manager.h"
#else
#include "poll_manager.h"
#endif

#include "../../libs/src/common.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <poll.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#ifndef TEST

struct FdIdxPair {
    int fd;
    int fdIdx;
};

struct PollManager {
    struct pollfd *pfds;
    HashTable *fdsIdxMap;
    int events;
    int count;
    int capacity;
};

#endif

STATIC int find_poll_fd_idx(PollManager *pollManager, int fd);

PollManager * create_poll_manager(int capacity, int events) {

    if (capacity <= 0) {
        capacity = DEF_FDS;
    }
    else if (capacity > MAX_FDS) {
        capacity = MAX_FDS;
    }

    PollManager *pollManager = (PollManager*) malloc(sizeof(PollManager));
    if (pollManager == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    pollManager->pfds = (struct pollfd*) malloc(capacity * sizeof(struct pollfd));
    if (pollManager->pfds == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    pollManager->fdsIdxMap = create_hash_table(capacity, 0, fnv1a_hash, are_ints_equal, NULL, delete_pfd_idx_pair);

    for (int i = 0; i < capacity; i++) {
       pollManager->pfds[i].fd = UNASSIGNED;
       pollManager->pfds[i].events = events;
    }

    pollManager->events = events;
    pollManager->count = 0;
    pollManager->capacity = capacity;

    return pollManager;
}

void delete_poll_manager(PollManager *pollManager) {

   if (pollManager != NULL) {
        free(pollManager->pfds);
        delete_hash_table(pollManager->fdsIdxMap);
    }
    free(pollManager); 
}

FdIdxPair * create_pfd_idx_pair(int fd, int fdIdx) {

    FdIdxPair *fdIdxPair = (FdIdxPair*) malloc(sizeof(FdIdxPair));
    if (fdIdxPair == NULL) {
        FAILED(ALLOC_ERROR, NULL);
    }

    fdIdxPair->fd = fd;
    fdIdxPair->fdIdx = fdIdx;

    return fdIdxPair;
}

void delete_pfd_idx_pair(void *fdIdxPair) {

    free(fdIdxPair);
}

bool are_pfd_idx_equal(void *fdIdxPair1, void *fdIdxPair2) {

    bool equal = 0;

    FdIdxPair *p1 = (FdIdxPair*)fdIdxPair1;
    FdIdxPair *p2 = (FdIdxPair*)fdIdxPair2;

    if (p1 != NULL && p2 != NULL) {

        equal = p1->fd == p2->fd;
    }
    return equal;
}

STATIC int find_poll_fd_idx(PollManager *pollManager, int fd) {

    if (pollManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int index = UNASSIGNED;

    for (int i = 0; i < pollManager->capacity; i++) {

        if (pollManager->pfds[i].fd == fd) {
            index = i;
            break;
        }
    }
    return index;
}

void add_fd_idx_to_hash_table(HashTable *hashTable, int fd, int fdIdx) {

    if (hashTable == NULL) {
        FAILED(ARG_ERROR, NULL);
    }
    FdIdxPair *fdIdxPair = create_pfd_idx_pair(fd, fdIdx);
    HashItem *item = create_hash_item(&fdIdxPair->fd, fdIdxPair);

    insert_item_to_hash_table(hashTable, item);
}

void remove_fd_idx_from_hash_table(HashTable *hashTable, int fd) {

    if (hashTable == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    remove_item_from_hash_table(hashTable, &fd);
}

int find_fd_idx_in_hash_table(HashTable *hashTable, int fd) {

    if (hashTable == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int fdIdx = -1;

    HashItem *item = find_item_in_hash_table(hashTable, &fd);

    if (item != NULL) {
        FdIdxPair *fdIdxPair = (FdIdxPair*)get_value(item);
        fdIdx = fdIdxPair->fdIdx;
    }
    return fdIdx;
}

void set_poll_fd(PollManager *pollManager, int fd) {

    if (pollManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int fdIdx = find_poll_fd_idx(pollManager, UNASSIGNED);

    if (fdIdx != -1) {

        add_fd_idx_to_hash_table(pollManager->fdsIdxMap, fd, fdIdx);
        pollManager->pfds[fdIdx].fd = fd;
        pollManager->count++;
    }
}

void unset_poll_fd(PollManager *pollManager, int fd) {

    if (pollManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int fdIdx = find_fd_idx_in_hash_table(pollManager->fdsIdxMap, fd);

    if (fdIdx != -1) {

        remove_fd_idx_from_hash_table(pollManager->fdsIdxMap, fd);
        pollManager->pfds[fdIdx].fd = UNASSIGNED;
        pollManager->count--;
    }
}

int get_poll_fd_count(PollManager *pollManager) {

    if (pollManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return pollManager->count;
}

int get_poll_capacity(PollManager *pollManager) {

    if (pollManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return pollManager->capacity;
}

struct pollfd * get_poll_pfds(PollManager *pollManager) {

    if (pollManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return pollManager->pfds;
}

int get_poll_fd(PollManager *pollManager, int fd) {

    if (pollManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int pollFd = UNASSIGNED;

    int fdIdx = find_fd_idx_in_hash_table(pollManager->fdsIdxMap, fd);

    if (fdIdx != -1) {
        pollFd = pollManager->pfds[fdIdx].fd;
    }

    return pollFd;
}

int get_poll_revents(PollManager *pollManager, int fd) {

    if (pollManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int revents = UNASSIGNED;

    int fdIdx = find_fd_idx_in_hash_table(pollManager->fdsIdxMap, fd);

    if (fdIdx != -1) {
        revents = pollManager->pfds[fdIdx].revents;
    }

    return revents;
}

bool is_fd_input_event(PollManager *pollManager, int fd) {

    if (pollManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int revents = get_poll_revents(pollManager, fd);

    return revents & POLLIN;
}

bool is_fd_error_event(PollManager *pollManager, int fd) {

    if (pollManager == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int revents = get_poll_revents(pollManager, fd);

    return revents & POLLERR || revents & POLLHUP;
}