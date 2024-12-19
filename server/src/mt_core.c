#define _XOPEN_SOURCE 700

#ifdef TEST
#else
#include "mt_core.h"
#endif

#include "tcp_server.h"

#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <signal.h>
#include <errno.h>

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

static void * reader_thread(void *arg);
static void * writer_thread(void *arg);
static void handle_client_connection(TCPServer *tcpServer, int threadId);
static void set_blocked_signals(void);

static pthread_mutex_t writeMutex = PTHREAD_MUTEX_INITIALIZER;


/* multithreaded server uses several threads:
    1. main thread - handles initialization,
        executes commands, sends messages and 
        handles signals,
    2. reader threads - read  parse client
        messages and execute commands
        from a subset of all sockets
    3. writer thread - sends messages and writes
        log file */
static void run_concurrent_server(AppContext *appContext) {

    ThreadPool *threadPool = create_thread_pool(get_fds_capacity(get_tcp_server_context())/ DEF_WORKLOAD, get_fds_capacity(get_tcp_server_context()));
    Thread *thread = create_thread();

    set_thread_pool_context(threadPool);
    set_thread_context(thread);

    run_multiple_threads(get_thread_pool_context(), reader_thread);
    run_single_thread(get_thread_context(), writer_thread);

    /* enable thread notifications */
    set_thread_pool(get_thread_pool_context());
    set_thread_pool_callback(notify_thread_pool);

    set_thread(get_thread_context());
    set_thread_callback(notify_single_thread);

    set_log_thread(get_thread_context());
    set_log_thread_callback(notify_single_thread);
 
    join_single_thread(get_thread_context(), NULL);
    join_multiple_threads(get_thread_pool_context(), NULL);

    delete_thread(get_thread_context());
    delete_thread_pool(get_thread_pool_context());
    
}

/* reader threads are responsible for reading incoming data
    from the clients, parsing messages and executing commands.
    the workload is divided between the threads, so that each
    thread works on the subset of clients */
static void * reader_thread(void *arg) {

    set_blocked_signals();

    ThreadData *threadData = (ThreadData*)arg;
    
    /* the startIdx and endIdx define the range of 
        sockets on which the thread will operate */
    int startIdx = get_start_idx(threadData);
    int endIdx = get_end_idx(threadData);

    /* pipes are used for inter-thread communication */
    const int FIRST_THREAD = get_thread_id(threadData) == 0 ? 1 : 0;
    const int PIPE_IDX = FIRST_THREAD ? LISTEN_FD_IDX + 1: startIdx;

    TCPServer *tcpServer = get_tcp_server_context();

    /* add pipe to the set of fd's monitored by poll() */
    assign_fd(get_tcp_server_context(), PIPE_IDX, get_thread_pipe_fd(threadData, READ_PIPE));

    CommandTokens *cmdTokens = create_command_tokens(1);

    int terminated = 0;

    while (!terminated) {

        int fdsReady = poll(&get_fds(tcpServer)[startIdx], endIdx - startIdx, -1);

        if (fdsReady < 0) {

            if (errno == EINTR) {
                continue;
            }
            else {
                FAILED(NO_ERRCODE, "Error polling descriptors");  
            }
        }

        while (fdsReady && startIdx < endIdx) {

            if (is_fd_ready(tcpServer, startIdx)) {

                /* first reader thread is responsible for 
                    handling connection requests */
                if (startIdx == LISTEN_FD_IDX) {

                    handle_client_connection();
                }  

                /* reader thread may receive sginal related 
                    message or a message from another thread.
                    pipes are used as a form of inter-thread 
                    communication */
                else if (startIdx == PIPE_IDX) {

                    struct {
                        int *terminated;
                        int *message;
                        int *log;
                    } data = {&terminated};

                    read_pipe_messages(&data);
                }       
                else {
                    read_socket_messages(tcpServer, cmdTokens, startIdx);

                    pthread_mutex_lock(&writeMutex);
                        
                    if (!messagePending) {

                        ThreadData *td = get_thread_data(get_thread_context());
                        write_message(get_thread_pipe_fd(td, WRITE_PIPE), "message");
                        messagePending = 1;
                    }
                    pthread_mutex_unlock(&writeMutex);
                }
                fdsReady--;
            }
            startIdx++;
        }
    }

    delete_command_tokens(get_cmd_tokens_context());

    return NULL;
}



/* writer thread handles sending messages to
    clients and logging to a file */
static void * writer_thread(void *arg) {

    set_blocked_signals();

    ThreadData *threadData = (ThreadData*)arg;

    struct pollfd pfd = {.events = POLL_IN, .fd = get_thread_pipe_fd(threadData, READ_PIPE)};

    int terminated = 0;

    while (!terminated) {

        int fdsReady = poll(&pfd, 1, -1);
        
        if (fdsReady < 0) {

            if (errno == EINTR) {
                continue;
            }
            else {
                FAILED(NO_ERRCODE, "Error polling descriptors");  
            }
        }
        else {

            int message = 0, log = 0;

            struct {
                int *terminated;
                int *message;
                int *log;
            } data = {&terminated, &message, &log};

            read_pipe_messages(&data);

            if (message) {

                /* send messages to clients */
                pthread_mutex_lock(&writeMutex);
                send_socket_messages(get_tcp_server_context());       
                messagePending = 0;
                pthread_mutex_unlock(&writeMutex);
            }
            else if (log) {

                write_log_to_file();
                set_log_pending(0);
            }
        }
    }
    return NULL;
}

static void set_blocked_signals(void) {

    /* thread blocks delivery of below signals 
    because these are handled by the main 
    thread */
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
}

static void handle_client_connection(TCPServer *tcpServer, int threadId) {

    int connectFd = accept_connection(tcpServer);
    int fdIndex = find_fd_index(tcpServer, -1);
    register_connection(tcpServer, fdIndex, connectFd);

    ThreadPool *threadPool = get_thread_pool_context();

    int id = get_thread_id_by_range_idx(threadPool, fdIndex);

    /* notify thread of new client connection */
    if (id != threadId) {

        ThreadData *threadData = get_thread_data_from_pool(threadPool, threadId);
        write_message(get_thread_pipe_fd(threadData, WRITE_PIPE), "client");
    }
}