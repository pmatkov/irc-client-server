/* --INTERNAL HEADER--
   used for testing */
#ifndef THREADS_H
#define THREADS_H

#include "../../libs/src/priv_io_utils.h"

#include <pthread.h>

#define DEF_WORKLOAD 100

typedef struct {
    int running;
    int threadId;
    int startIdx;
    int endIdx;
    StreamPipe *streamPipe;
} ThreadData;

typedef struct {
    pthread_t *threads;
    ThreadData *threadData;
    int count;
    int workload;
} ThreadPool;

typedef struct {
    pthread_t *thread;
    ThreadData *threadData;
} Thread;

typedef void * (*ThreadFunc)(void *arg);
typedef void (*NotifyThreadFunc)(void *arg, const char *string);

ThreadPool * create_thread_pool(int count, int totalWorkload);
void delete_thread_pool(ThreadPool *threadPool);

Thread * create_thread(void);
void delete_thread(Thread *thread);

void run_single_thread(Thread *thread, ThreadFunc threadFunc);
void run_multiple_threads(ThreadPool *threadPool, ThreadFunc threadFunc);
void join_single_thread(Thread *thread, void **buffer);
void join_multiple_threads(ThreadPool *threadPool, void **buffer);

ThreadData * get_current_thread_data_from_pool(ThreadPool *threadPool);
ThreadData * get_thread_data_from_pool(ThreadPool *threadPool, int index);
ThreadData * get_thread_data(Thread *thread);
void set_thread_data(Thread *thread, ThreadData *threadData);

int get_thread_pool_workload(ThreadPool *threadPool);
int get_thread_count(ThreadPool *threadPool);
int get_thread_id(ThreadData *threadData);
int get_start_idx(ThreadData *threadData);
int get_end_idx(ThreadData *threadData);

int get_thread_id_by_range_idx(ThreadPool *threadPool, int rangeIdx);
int get_thread_idx_by_range_idx(ThreadPool *threadPool, int rangeIdx);

int get_thread_pipe_fd(ThreadData *threadData, int pipeIdx);

void notify_single_thread(void *thread, const char *message);
void notify_thread_pool(void *threadPool, const char *message);

#endif