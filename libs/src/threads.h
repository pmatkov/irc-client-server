#ifndef THREADS_H
#define THREADS_H

#define READ_PIPE 0
#define WRITE_PIPE 1
#define PIPE_FD_COUNT 2
#define DEF_WORKLOAD 100

/* contains thread status, thread id,
    pipe fd's and, for threads in the pool,
    range indexes on which the thread will
    operate */
typedef struct ThreadData ThreadData;

/* contains poll of threads with thread 
    data */
typedef struct ThreadPool ThreadPool;

/* contains thread with thread data */
typedef struct Thread Thread;

typedef void * (*ThreadFunc)(void *arg);

/* notify thread. arg is either thread or thread 
    pool */
typedef void (*NotifyThreadFunc)(void *arg, const char *string);

/* create a thread pool */
ThreadPool * create_thread_pool(int count, int totalWorkload);
void delete_thread_pool(ThreadPool *threadPool);

/* create a single thread */
Thread * create_thread(void);
void delete_thread(Thread *thread);

/* run and join threads */
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

/* send a message to thread or thread pool */
void notify_single_thread(void *thread, const char *message);
void notify_thread_pool(void *threadPool, const char *message);

#endif