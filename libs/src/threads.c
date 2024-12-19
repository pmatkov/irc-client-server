#ifdef TEST
#include "priv_threads.h"
#else
#include "threads.h"
#include "../../libs/src/io_utils.h"
#endif

#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

#define MAX_THREADS 10

#ifndef TEST

struct ThreadData {
    int running;
    int threadId;
    int startIdx;
    int endIdx;
    StreamPipe *streamPipe;
};

struct ThreadPool {
    pthread_t *threads;
    ThreadData *threadData;
    int count;
    int workload;
};

struct Thread {
    pthread_t *thread;
    ThreadData *threadData;
};

#endif

ThreadPool * create_thread_pool(int count, int totalWorkload) {

    if (count <= 0) {
        count = 1;
    }
    else if (count > MAX_THREADS) {
        count = MAX_THREADS;
    }

    int threadWorkload = totalWorkload/ count;
    int remainingWorkload = totalWorkload% count;

    ThreadPool *threadPool = (ThreadPool*) malloc(sizeof(ThreadPool));
    if (threadPool == NULL) {
        FAILED(ALLOC_ERROR, NULL);  
    }

    threadPool->threads = (pthread_t*) malloc(count * sizeof(pthread_t));
    if (threadPool->threads == NULL) {
        FAILED(ALLOC_ERROR, NULL);  
    }

    threadPool->threadData = (ThreadData*) malloc(count * sizeof(ThreadData));
    if (threadPool->threadData == NULL) {
        FAILED(ALLOC_ERROR, NULL);  
    }

    for (int i = 0; i < count; i++) {
        threadPool->threadData[i].running = 0;
        threadPool->threadData[i].threadId = i;
        threadPool->threadData[i].startIdx = threadPool->threadData[i].threadId * threadWorkload;

        if (i < count - 1) {
            threadPool->threadData[i].endIdx = threadPool->threadData[i].startIdx + threadWorkload;
        }
        else {
            threadPool->threadData[i].endIdx = threadPool->threadData[i].startIdx + threadWorkload + remainingWorkload;
        }

        threadPool->threadData[i].streamPipe = create_pipe();   
    }

    threadPool->count = count;
    threadPool->workload = totalWorkload;

    return threadPool;
}

void delete_thread_pool(ThreadPool *threadPool) {

    if (threadPool != NULL) {

        for (int i = 0; i < threadPool->count; i++) {
            delete_pipe(threadPool->threadData[i].streamPipe);
        }
        free(threadPool->threadData);
        free(threadPool->threads);
    }

    free(threadPool);
}

Thread * create_thread(void) {

    Thread *thread = (Thread*) malloc(sizeof(Thread));
    if (thread == NULL) {
        FAILED(ALLOC_ERROR, NULL);  
    }

    thread->thread = (pthread_t*) malloc(sizeof(pthread_t));
    if (thread->thread == NULL) {
        FAILED(ALLOC_ERROR, NULL);  
    }

    thread->threadData = (ThreadData*) malloc(sizeof(ThreadData));
    if (thread->threadData == NULL) {
        FAILED(ALLOC_ERROR, NULL);  
    }

    thread->threadData->running = 0;
    thread->threadData->threadId = 0;
    thread->threadData->startIdx = 0;
    thread->threadData->endIdx = 0;

    thread->threadData->streamPipe = create_pipe();

    return thread;
}

void delete_thread(Thread *thread) {

    if (thread != NULL) {

        delete_pipe(thread->threadData->streamPipe);

        free(thread->threadData);
        free(thread->thread);
    }

    free(thread);
}

void run_single_thread(Thread *thread, ThreadFunc threadFunc) {

    if (thread == NULL || threadFunc == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (thread->threadData->running) {
        FAILED(NO_ERRCODE, "The thread is already running");
    }

    thread->threadData->running = 1;
    pthread_create(thread->thread, NULL, threadFunc, thread->threadData);
}

void run_multiple_threads(ThreadPool *threadPool, ThreadFunc threadFunc) {

    if (threadPool == NULL || threadFunc == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    for (int i = 0; i < threadPool->count; i++) {

        if (threadPool->threadData[i].running) {
            FAILED(NO_ERRCODE, "The thread is already running");
        }
        else {

            threadPool->threadData[i].running = 1;
            pthread_create(&threadPool->threads[i], NULL, threadFunc, &threadPool->threadData[i]);
        }
    }
}

void join_single_thread(Thread *thread, void **buffer) {

    if (thread == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    void **result = NULL;

    if (buffer != NULL) {
        result = buffer;
    }
    pthread_join(*thread->thread, result);
    thread->threadData->running = 0;
}

void join_multiple_threads(ThreadPool *threadPool, void **buffer) {

    if (threadPool == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    void **result = NULL;

    for (int i = 0; i < threadPool->count; i++) {


        if (buffer != NULL) {
            result = buffer[i];
        }
        pthread_join(threadPool->threads[i], result);
        threadPool->threadData[i].running = 0;
    }
}

ThreadData * get_current_thread_data_from_pool(ThreadPool *threadPool) {

    if (threadPool == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    pthread_t self = pthread_self();

    ThreadData *threadData = NULL;
    
    for (int i = 0; i < threadPool->count; i++) {

        if (pthread_equal(self, threadPool->threads[i])) {
            threadData = &threadPool->threadData[i];
            break;
        }
    }
    return threadData; 
}

ThreadData * get_thread_data_from_pool(ThreadPool *threadPool, int index) {

    if (threadPool == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    if (index < 0 || index >= threadPool->count) {
        FAILED(RANGE_ERROR, NULL);
    }

    return &threadPool->threadData[index];
}

ThreadData * get_thread_data(Thread *thread) {

    if (thread == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return thread->threadData;
}

void set_thread_data(Thread *thread, ThreadData *threadData) {

    if (thread == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    thread->threadData->running = threadData->running;
    thread->threadData->threadId = threadData->threadId;
    thread->threadData->startIdx = threadData->startIdx;
    thread->threadData->endIdx = threadData->endIdx;

}

int get_thread_pool_workload(ThreadPool *threadPool) {

    if (threadPool == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return threadPool->workload;
}

int get_thread_count(ThreadPool *threadPool) {

    if (threadPool == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return threadPool->count;
}

int get_thread_id(ThreadData *threadData) {

    if (threadData == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return threadData->threadId;
}

int get_start_idx(ThreadData *threadData) {

    if (threadData == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return threadData->startIdx;
}

int get_end_idx(ThreadData *threadData) {

    if (threadData == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    return threadData->endIdx;
}

int get_thread_id_by_range_idx(ThreadPool *threadPool, int rangeIdx) {

    if (threadPool == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int threadId = -1;

    for (int i = 0; i < threadPool->count; i++) {

        if (rangeIdx >= threadPool->threadData[i].startIdx && rangeIdx < threadPool->threadData[i].endIdx) {
            threadId = threadPool->threadData[i].threadId;
            break;
        }
    }
    return threadId;
}

int get_thread_idx_by_range_idx(ThreadPool *threadPool, int rangeIdx) {

    if (threadPool == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int threadIdx = -1;

    for (int i = 0; i < threadPool->count; i++) {

        if (rangeIdx >= threadPool->threadData[i].startIdx && rangeIdx < threadPool->threadData[i].endIdx) {
            threadIdx = i;
            break;
        }
    }
    return threadIdx;
}

int get_thread_pipe_fd(ThreadData *threadData, int pipeIdx) {

    if (threadData == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    int pipeFd = -1;

    if (pipeIdx >= 0 && pipeIdx < PIPE_FD_COUNT) {
        pipeFd = get_pipe_fd(threadData->streamPipe, pipeIdx);
    }

    return pipeFd;
}

void notify_single_thread(void *thread, const char *message) {

    if (thread != NULL && message != NULL) {

        write(get_thread_pipe_fd(((Thread*)thread)->threadData, WRITE_PIPE), message, strlen(message));
    }
}

void notify_thread_pool(void *threadPool, const char *message) {

    if (threadPool != NULL && message != NULL) {

        for (int i = 0; i < ((ThreadPool*)threadPool)->count; i++) {

            ThreadData *threadData = get_thread_data_from_pool((ThreadPool*)threadPool, i);
            write(get_thread_pipe_fd(threadData, WRITE_PIPE), message, strlen(message));
        }
    }
}