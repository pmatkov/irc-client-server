#define _XOPEN_SOURCE 700

#include "main.h"
#include "tcp_server.h"
#include "user.h"
#include "channel.h"
#include "command_handler.h"
#include "../../libs/src/threads.h"
#include "../../libs/src/settings.h"
#include "../../libs/src/command.h"
#include "../../libs/src/message.h"
#include "../../libs/src/io_utils.h"
#include "../../libs/src/network_utils.h"
#include "../../libs/src/signal_handler.h"
#include "../../libs/src/error_control.h"
#include "../../libs/src/logger.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_COMMANDS MAX_FDS/ 2

/* holds references to dynamically allocated
    data for cleanup */
typedef struct {
    Logger *logger;
    Settings *settings;
    TCPServer *tcpServer;
    CommandTokens *cmdTokens;
    ThreadPool *readerThreadPool;
    Thread *writerThread;
} AppContext;

#ifndef TEST

static AppContext appContext = {NULL};

static int messagePending = 0;
static pthread_mutex_t writeMutex = PTHREAD_MUTEX_INITIALIZER;

/* enables signal handling with poll() - 
   self pipe trick */
static int serverPipeFd[PIPE_FD_COUNT];

static void run_standard_server(AppContext *appContext);
static void run_concurrent_server(AppContext *appContext);

static void * reader_thread(void *arg);
static void * writer_thread(void *arg);

static void read_socket_messages(TCPServer *tcpServer, CommandTokens *cmdTokens, int fdIndex);
static void read_pipe_messages(void *arg);
static void send_socket_messages(TCPServer *tcpServer);

static void process_pipe_message(const char *message, void *arg);
static void process_irc_message(const char *command, void *arg);

static void daemonize(void);

static void get_command_line_args(int argc, char **argv);
static void cleanup(void);

int main(int argc, char **argv)
{
    /* register cleanup function */
    atexit(cleanup);

    /* create and initialize settings */
    appContext.settings = create_settings(SERVER_OT_COUNT);
    initialize_server_settings();

    /* parse command line arguments */
    get_command_line_args(argc, argv);

    /* create logger */
    appContext.logger = create_logger(NULL, LOG_FILE(server), get_int_option_value(OT_SERVER_LOG_LEVEL));

    LOG(INFO, "Server started");

    if (get_int_option_value(OT_DAEMON)) {
        daemonize();
        LOG(INFO, "Started daemon process with PID: %d", getpid());
    }
    if (get_int_option_value(OT_ECHO)) {
        LOG(INFO, "Echo server enabled");
    }
    if (get_int_option_value(OT_THREADS)) {
        LOG(INFO, "Multithreading enabled");
    }
    /* provides networking functionality for 
        the app */
    appContext.tcpServer = create_server(get_int_option_value(OT_MAX_FDS), get_char_option_value(OT_SERVER_NAME));
    int listenFd = init_server(NULL, get_int_option_value(OT_PORT));

    /* set fd for the listening socket */
    assign_fd(appContext.tcpServer, LISTEN_FD_IDX, listenFd);

    /*  a pipe is used to handle registered signals 
        with poll(). signals interrupt poll() and
        transfer control to the signal handler. inside
        a registered handler, a message is written to 
        the pipe. this message will then be detected 
        with poll() as an input event on the pipe */
    if (!get_int_option_value(OT_THREADS)) {
        create_pipe(serverPipeFd);
        set_server_pipe(serverPipeFd[WRITE_PIPE]);
        assign_fd(appContext.tcpServer, PIPE_FD_IDX, serverPipeFd[READ_PIPE]);
    }

    /* set signal handlers */
    set_sigaction(handle_server_sigint, SIGINT, (int[]){SIGINT, SIGTERM, SIGHUP, SIGQUIT, 0});
    set_sigaction(handle_server_sigint, SIGTERM, (int[]){SIGINT, SIGTERM, SIGHUP, SIGQUIT, 0});
    set_sigaction(SIG_IGN, SIGPIPE, NULL);

    /* the server can be run in single-threaded or 
        multithreaded mode. performance improvements 
        are expected in multithreaded mode with a 
        large number of clients and high traffic */
    if (!get_int_option_value(OT_THREADS)) {
        run_standard_server(&appContext);  
    }
    else {
        run_concurrent_server(&appContext);  
    }
    return 0;
}

/* a standard server runs on a single thread */
static void run_standard_server(AppContext *appContext) {

  /* the main program flow consists of the following actions:

        1.a keep accepting connection requests from the clients 
            as they arrive,
        1.b read and parse clients messages,
        2. if a parsed message is a valid command, execute it,
        3. send responses to the clients or forward messages
            from the clients to other clients */

    /* command tokens store parsed commands */
    appContext->cmdTokens = create_command_tokens(1);

    while (1) {

        /*  poll() is used to monitor input events on the pipe and
            sockets. two kinds of network events are monitored, 
            connection requests and incoming data */
        int fdsReady = poll(get_fds(appContext->tcpServer), get_fds_capacity(appContext->tcpServer), -1);

        if (fdsReady < 0) {

            if (errno == EINTR) {
                continue;
            }
            else {
                FAILED(NO_ERRCODE, "Error polling descriptors");  
            }
        }
        /* a client may become unresponsive in certain situations but is still connected to the server. these situations could pose a problem for the server if not handled. therefore, server will remove inactive clients after a certain period of inactivity */

        // remove_unregistered_clients(appContext.tcpServer, appContext.clArgs->waitTime);
        
        int startIdx = 0;

        while (fdsReady && startIdx < get_fds_capacity(appContext->tcpServer)) {

            int ready = is_fd_ready(appContext->tcpServer, startIdx);

            if (ready) {

                if (startIdx == LISTEN_FD_IDX) {
                    add_client(appContext->tcpServer);
                }      
                else if (startIdx == PIPE_FD_IDX) {
                    read_pipe_messages(NULL);
                }        
                else {
                    read_socket_messages(appContext->tcpServer, appContext->cmdTokens, startIdx);
                }
                fdsReady--;
            }
            startIdx++;
        }
        send_socket_messages(appContext->tcpServer);
    }
}

/* the concurrent server uses multithreading to 
    improve performance in environment with many
    clients and high traffic. the workload is 
    divided between several threads:
    1. main thread - calls startup routines and
        performs initialization, executes IRC 
        commands, sends ougoing messages and 
        handles signals,
    2. a pool of reader threads - read incoming 
        data from sockets and parse messages (in 
        order to minimize access to shared resources,
        each thread is responsible for a specific 
        subset of sockets),
    3. logger thread - writes to the log file */
static void run_concurrent_server(AppContext *appContext) {

    appContext->readerThreadPool = create_thread_pool(get_fds_capacity(appContext->tcpServer)/ DEFAULT_WORKLOAD, get_fds_capacity(appContext->tcpServer));
    appContext->writerThread = create_thread();

    run_multiple_threads(appContext->readerThreadPool, reader_thread);
    run_single_thread(appContext->writerThread, writer_thread);

    /* enable thread notifications */
    set_thread_pool(appContext->readerThreadPool);
    set_thread_pool_callback(notify_thread_pool);

    set_thread(appContext->writerThread);
    set_thread_callback(notify_single_thread);

    set_log_thread(appContext->writerThread);
    set_log_thread_callback(notify_single_thread);
 
    join_single_thread(appContext->writerThread, NULL);
    join_multiple_threads(appContext->readerThreadPool, NULL);

    delete_thread(appContext->writerThread);
    delete_thread_pool(appContext->readerThreadPool);
    
}

/* reader threads are responsible for reading incoming data
    from the clients, parsing messages and executing commands.
    the workload is divided between the threads, so that each
    thread works on the subset of clients */
static void * reader_thread(void *arg) {

    /* threads block delivery of below signals because
        these are handled by the main thread */
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    ThreadData *threadData = (ThreadData*)arg;
    
    /* the startIdx and endIdx define the range of 
        sockets on which the thread will operate */
    int threadId = get_thread_id(threadData);
    int startIdx = get_start_idx(threadData);
    int endIdx = get_end_idx(threadData);

    /* pipes are used for inter-thread communication */
    const int FIRST_THREAD = threadId == 0 ? 1 : 0;
    const int PIPE_IDX = FIRST_THREAD ? LISTEN_FD_IDX + 1: startIdx;

    /* add pipe to the set of fd's monitored by poll() */
    assign_fd(appContext.tcpServer, PIPE_IDX, get_thread_pipe_fd(threadData, READ_PIPE));

    CommandTokens *cmdTokens = create_command_tokens(1);

    int terminated = 0;

    while (!terminated) {

        int fdsReady = poll(&get_fds(appContext.tcpServer)[startIdx], endIdx - startIdx, -1);

        if (fdsReady < 0) {

            if (errno == EINTR) {
                continue;
            }
            else {
                FAILED(NO_ERRCODE, "Error polling descriptors");  
            }
        }

        while (fdsReady && startIdx < endIdx) {

            if (is_fd_ready(appContext.tcpServer, startIdx)) {

                /* the first reader thread is responsible for 
                    accepting connection requests */
                if (startIdx == LISTEN_FD_IDX) {

                    int connectFd = accept_connection(appContext.tcpServer);
                    int fdIndex = find_fd_index(appContext.tcpServer, -1);
                    register_connection(appContext.tcpServer, fdIndex, connectFd);

                    int id = get_thread_id_by_range_idx(appContext.readerThreadPool, fdIndex);

                    /* if fd that is assigned to the new connection 
                        belongs to the range of another thread, this 
                        thread will notify it */
                    if (id != threadId) {

                        ThreadData *threadData = get_thread_data_from_pool(appContext.readerThreadPool, threadId);
                        write_message(get_thread_pipe_fd(threadData, WRITE_PIPE), "client");
                    }
                }  

                /* a reader thread may receive a signal message or 
                    a "client" message from the pipe. a "client" 
                    message is used to restart poll() with new set
                    of fd's after client connection (sendMessage 
                    and writeLog fields are ignored here) */
                else if (startIdx == PIPE_IDX) {

                    struct {
                        int *terminated;
                        int *sendMessage;
                        int *writeLog;
                    } data = {&terminated};

                    read_pipe_messages(&data);
                }       
                else {
                    read_socket_messages(appContext.tcpServer, cmdTokens, startIdx);

                    pthread_mutex_lock(&writeMutex);
                        
                    if (!messagePending) {

                        ThreadData *td = get_thread_data(appContext.writerThread);
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

    delete_command_tokens(appContext.cmdTokens);

    return NULL;
}

/* writer thread handles sending messages to
    clients and logging to a file */
static void * writer_thread(void *arg) {

    /* thread blocks delivery of below signals 
        because these are handled by the main 
        thread */
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

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
                send_socket_messages(appContext.tcpServer);       
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

static void read_pipe_messages(void *arg) {

    char buffer[MAX_CHARS + 1] = {'\0'};

    read_string(serverPipeFd[READ_PIPE], buffer, sizeof(buffer));
    process_messages(buffer, CRLF, process_pipe_message, arg);
}

static void process_pipe_message(const char *message, void *arg) {

    if (message == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct {
        int *terminated;
        int *message;
        int *log;
    } *data = arg;

    if (strcmp(message, "sigint") == 0) {

        /* terminate single threaded server */
        if (!get_int_option_value(OT_THREADS)) {

            exit(EXIT_SUCCESS);
        }
        /* terminate multithreaded server */
        else {
            if (data != NULL) {
                *data->terminated = 1;
            }
        }
    }
    else if (strcmp(message, "message") == 0) {

        if (data != NULL) {
            *data->message = 1;
        }
    }
    else if (strcmp(message, "log") == 0) {

        if (data != NULL) {
            *data->log = 1;
        }
    }
}

static void read_socket_messages(TCPServer *tcpServer, CommandTokens *cmdTokens, int fdIndex) {

    if (tcpServer == NULL || cmdTokens == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    /* read data from the socket */
    int readStatus = server_read(tcpServer, fdIndex);

    if (readStatus) {

        /* extract individual messages from the buffer. 
            partial messages are left in the buffer until
            the full message is received */
        Client *client = get_client(tcpServer, fdIndex);
        char *buffer = get_client_inbuffer(client);

        struct {
            TCPServer *tcpServer;
            Client *client;
            CommandTokens *cmdTokens;
            int fdIndex;
        } data = {tcpServer, client, cmdTokens, fdIndex};

        process_messages(buffer, CRLF, process_irc_message, &data);
    }
}

static void process_irc_message(const char *message, void *arg) {

    if (message == NULL || arg == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    struct {
        TCPServer *tcpServer;
        Client *client;
        CommandTokens *cmdTokens;
        int fdIndex;
    } *data = arg;

    /* send message back to the client */
    if (get_int_option_value(OT_ECHO)) {
        server_write(data->tcpServer, data->fdIndex, message);
    }
    /* parse messages and execute commands */
    else {
        LOG(DEBUG, "-> extracted message \"%s\"", message);

        parse_message(data->tcpServer, data->client, data->cmdTokens);
        const char *commandString = get_command(data->cmdTokens);

        if (commandString != NULL) {

            CommandFunc commandFunc = get_command_function(string_to_command_type(commandString));

            commandFunc(data->tcpServer, data->client, data->cmdTokens);
            reset_command_tokens(data->cmdTokens);
        } 
    }
}

static void send_socket_messages(TCPServer *tcpServer) {

    if (tcpServer == NULL) {
        FAILED(ARG_ERROR, NULL);
    }

    Session *session = get_session(tcpServer);

    int fdIndex = -1;

    /* send messages from the server message queue (messages
        intended for unregistered clients) */
    while (!is_queue_empty(get_server_out_queue(tcpServer))) {

        ExtMessage *message = dequeue_from_server_queue(tcpServer);
        char *recipient = get_ext_message_recipient(message);
        char *content = get_ext_message_content(message);

        int fd = str_to_uint(recipient);
        fdIndex = find_fd_index(tcpServer, fd);

        server_write(tcpServer, fdIndex, content);
    }

    struct {
        TCPServer *tcpServer;
        int fdIndex;
        const char *message;
    } data = {tcpServer, fdIndex, NULL};

    /* send messages from users' and channels' queues ( 
        users and channels have dedicated queues) */
    // iterate_list(get_ready_users(get_ready_list(session)), send_user_queue_messages, NULL);
    iterate_list(get_ready_users(get_ready_list(session)), send_user_queue_messages, &data);

    iterate_list(get_ready_channels(get_ready_list(session)), send_channel_queue_messages, &data);
    // iterate_list(get_ready_channels(get_ready_list(session)), send_channel_queue_messages, get_session(tcpServer));

    reset_linked_list(get_ready_users(get_ready_list(session)));
    reset_linked_list(get_ready_channels(get_ready_list(session)));
}


/* create a deamon process that will run in the background
    detached from the terminal */
static void daemonize(void) {

    pid_t pid = fork();

    if (pid < 0) {
        FAILED(NO_ERRCODE, "Error creating daemon process");
    }
    else if (pid) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        FAILED(NO_ERRCODE, "Error creating daemon process");
    }

    set_sigaction(SIG_IGN, SIGHUP, NULL);

    pid = fork();

    if (pid < 0) {
        FAILED(NO_ERRCODE, "Error creating daemon process");
    }
    else if (pid) {
        exit(EXIT_SUCCESS);
    }

    enable_stderr_logging(0);
    enable_stdout_logging(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);

}

/* parse command line arguments */
static void get_command_line_args(int argc, char **argv) {

    int opt;

    while ((opt = getopt(argc, argv, "deflpstw")) != -1) {

        switch (opt) {
            case 'd': {
                set_option_value(OT_DAEMON, &(int){1});
                break;
            }
            case 'e': {
                set_option_value(OT_ECHO, &(int){1});
                break;
            }
            case 'f': {
                int maxFds = str_to_uint(argv[3]);
                if (maxFds != -1) {
                    set_option_value(OT_MAX_FDS, &maxFds);
                }
                break;
            }
            case 'l': {
                LogLevel logLevel = string_to_log_level(argv[4]);
                if (is_valid_log_level(logLevel)) {
                    set_option_value(OT_SERVER_LOG_LEVEL, &logLevel);
                }
                break;
            }
            case 'p': {
                int port = str_to_uint(argv[5]);
                if (is_valid_port(port)) {
                    set_option_value(OT_PORT, &port);
                }
                break;
            }
            case 's': {
                set_option_value(OT_SERVER_NAME, argv[6]);
                break;
            }
            case 't': {
                set_option_value(OT_THREADS, &(int){str_to_uint(argv[7])});
                break;
            }
            case 'w': {
                int waitTime = str_to_uint(argv[8]);
                if (waitTime != -1) {
                    set_option_value(OT_WAIT_TIME, &waitTime);
                }
                break;
            }
            default:
                printf("Usage: %s [-d <daemon>] [-e <echo>] [-f <max fds>] [-l <loglevel>] [-p <port>] [-s <servername>] [-t <threads>] [-w <waittime>]\n", argv[0]);
                printf("\tOptions:\n");
                printf("\t  -d : Run as a daemon\n");
                printf("\t  -e : Enable echo mode\n");
                printf("\t  -f : Set max file descriptors\n");
                printf("\t  -l : Set the logging level\n");
                printf("\t  -p : Specify the port number\n");
                printf("\t  -s : Specify the server name\n");
                printf("\t  -t : Use multithreading\n");
                printf("\t  -w : Set wait time\n");
                exit(EXIT_FAILURE);
        }
    }
}

/* perform cleanup */
static void cleanup(void) {

    LOG(INFO, "Shutdown initiated");

    if (!get_int_option_value(OT_THREADS)) {
        close(serverPipeFd[READ_PIPE]);
        close(serverPipeFd[WRITE_PIPE]);
    }

    delete_command_tokens(appContext.cmdTokens);
    delete_server(appContext.tcpServer);
    delete_logger(appContext.logger);
    delete_settings(appContext.settings);
}

#endif

void initialize_server_settings(void) {

    register_option(INT_TYPE, OT_DAEMON, "daemon", &(int){0});
    register_option(INT_TYPE, OT_ECHO, "echo", &(int){0});
    register_option(INT_TYPE, OT_MAX_FDS, "maxfds", &(int){1024});
    register_option(INT_TYPE, OT_SERVER_LOG_LEVEL, "loglevel", &(int){DEBUG});
    register_option(INT_TYPE, OT_PORT, "port",  &(int){50100});
    register_option(CHAR_TYPE, OT_SERVER_NAME, "servername", "irc.server.com");
    register_option(INT_TYPE, OT_THREADS, "threads", &(int){0});
    register_option(INT_TYPE, OT_WAIT_TIME, "waittime", &(int){60});
}
