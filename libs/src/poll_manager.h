#ifndef POLL_MANAGER_H
#define POLL_MANAGER_H

#include "../../libs/src/hash_table.h"

#include <stdbool.h>

#define RESERVED_FDS 128
#define DEF_FDS 512 + RESERVED_FDS
#define MAX_FDS 1024 + RESERVED_FDS

typedef struct FdIdxPair FdIdxPair;
typedef struct PollManager PollManager;

PollManager * create_poll_manager(int capacity, int events);
void delete_poll_manager(PollManager *pollManager);

FdIdxPair * create_pfd_idx_pair(int fd, int fdIdx);
void delete_pfd_idx_pair(void *fdIdxPair);

bool are_pfd_idx_equal(void *pfdIdxPair1, void *pfdIdxPair2);

void add_fd_idx_to_hash_table(HashTable *hashTable, int fd, int fdIdx);
void remove_fd_idx_from_hash_table(HashTable *hashTable, int fd);
int find_fd_idx_in_hash_table(HashTable *hashTable, int fd);

void set_poll_fd(PollManager *pollManager, int fd);
void unset_poll_fd(PollManager *pollManager, int fd);

struct pollfd * get_poll_pfds(PollManager *pollManager);

int get_poll_fd_count(PollManager *pollManager);
int get_poll_capacity(PollManager *pollManager);

int get_poll_fd(PollManager *pollManager, int fd);
int get_poll_revents(PollManager *pollManager, int fd);

bool is_fd_input_event(PollManager *pollManager, int fd);
bool is_fd_error_event(PollManager *pollManager, int fd);

#endif