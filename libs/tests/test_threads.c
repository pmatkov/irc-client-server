#include "../src/priv_threads.h"
#include "../src/time_utils.h"
#include "../src/io_utils.h"
#include "../src/string_utils.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define THREAD_COUNT 10
#define ARRAY_SIZE (1000 * 100)

static int numbers[ARRAY_SIZE];
static int primeCount;
static int signal = 0; 
static Timer *timer;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 

static void fill_array(void) {

    for (int i = 0; i < ARRAY_SIZE; i++) {

        numbers[i] = rand() % ARRAY_SIZE;
    }
}

static void initialize_test_suite(void) {

    fill_array();
    timer = create_timer();
}

static void cleanup_test_suite(void) {

    delete_timer(timer);
}

static int is_prime(int n) {

    int prime = 1;

    if (n <= 1 || (n > 2 && n % 2 == 0)) {
        prime = 0;
    }

    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) {
            prime = 0;
            break;
        }
    }

    return prime;
}

static void * count_primes(void *arg) {

    ThreadData *threadData = (ThreadData*) arg;

    for (int i = threadData->startIdx; i < threadData->endIdx; i++) {
        if (is_prime(numbers[i])) {

            pthread_mutex_lock(&mutex);
            primeCount++;
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

static void * reader(void *arg) {

    pthread_mutex_lock(&mutex);
        
    while (!signal) {
        pthread_cond_wait(&cond, &mutex);
    }

    ThreadData *threadData = (ThreadData*) arg;

    char buffer[MAX_CHARS + 1] = {'\0'};
    read_message(threadData->pipeFd[READ_PIPE], buffer, sizeof(buffer));

    pthread_mutex_unlock(&mutex);

    intptr_t result = 0;

    if (strcmp(buffer, "message") == 0) {
        result = 1;
    }

    return (void*) result; 
}

START_TEST(test_thread_pool) {

    ThreadPool *threadPool = create_thread_pool(THREAD_COUNT, ARRAY_SIZE);
    ck_assert_ptr_ne(threadPool, NULL);

    start_timer(timer);
    run_multiple_threads(threadPool, count_primes);
    join_multiple_threads(threadPool, NULL);
    stop_timer(timer);

    printf("time pool: %ld\n", get_elapsed_time(timer, MICROSECONDS));
    reset_timer(timer);

    ck_assert_int_ne(primeCount, 0);

    delete_thread_pool(threadPool);
}
END_TEST

START_TEST(test_thread) {

    primeCount = 0;

    Thread *thread = create_thread();
    ck_assert_ptr_ne(thread, NULL);

    set_thread_data(thread, &(ThreadData) {0, 0, 0, ARRAY_SIZE});

    start_timer(timer);
    run_single_thread(thread, count_primes);
    join_single_thread(thread, NULL);
    stop_timer(timer);
    printf("time single: %ld\n", get_elapsed_time(timer, MICROSECONDS));

    ck_assert_int_ne(primeCount, 0);

    delete_thread(thread);
}
END_TEST

START_TEST(test_get_thread_data) {

    Thread *thread = create_thread();
    ck_assert_ptr_ne(thread, NULL);

    set_thread_data(thread, &(ThreadData) {0, 0, 1, 5});
    ThreadData *threadData = get_thread_data(thread);

    ck_assert_int_eq(threadData->running, 0);
    ck_assert_int_eq(threadData->threadId, 0);
    ck_assert_int_eq(threadData->startIdx, 1);
    ck_assert_int_eq(threadData->endIdx, 5);

    delete_thread(thread);

    ThreadPool *threadPool = create_thread_pool(THREAD_COUNT, ARRAY_SIZE);
    threadData = get_thread_data_from_pool(threadPool, 0);

    ck_assert_int_eq(threadData->threadId, 0);
    ck_assert_int_eq(threadData->startIdx, 0);
    ck_assert_int_eq(threadData->endIdx, ARRAY_SIZE/ THREAD_COUNT);
    ck_assert_int_eq(get_thread_pool_workload(threadPool), ARRAY_SIZE);

    int threadId = get_thread_id_by_range_idx(threadPool, 10000);
    ck_assert_int_eq(threadId, 1);

    delete_thread_pool(threadPool);

}
END_TEST

START_TEST(test_thread_messaging) {

    Thread *thread = create_thread();
    ck_assert_ptr_ne(thread, NULL);

    run_single_thread(thread, reader);

    pthread_mutex_lock(&mutex);

    ck_assert_ptr_ne(thread->threadData, NULL);
    write_message(thread->threadData->pipeFd[WRITE_PIPE], "message");
    signal = 1;

    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    
    void *result = NULL;
    join_single_thread(thread, &result);

    ck_assert_int_eq((intptr_t)result, 1);

    delete_thread(thread);

}
END_TEST

Suite* threads_suite(void) {
    Suite *s;
    TCase *tc_core, *tc_convert;

    s = suite_create("Threads");
    tc_core = tcase_create("Core");
    tc_convert = tcase_create("Convert");

    // Add the test case to the test suite
    tcase_add_test(tc_convert, test_thread_pool);
    tcase_add_test(tc_convert, test_thread);
    tcase_add_test(tc_convert, test_get_thread_data);
    tcase_add_test(tc_convert, test_thread_messaging);

    suite_add_tcase(s, tc_convert);
    suite_add_tcase(s, tc_core);

    return s;
}

#ifdef TEST
int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = threads_suite();
    sr = srunner_create(s);

    initialize_test_suite();

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    cleanup_test_suite();

    return (number_failed == 0) ? 0 : 1;
}

#endif
