#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int socket;
    int outstanding;
    char tail[16];
    size_t tail_length;
} Connection;

typedef struct {
    int connections;
    int pipeline;
    int port;
    double seconds;
    atomic_long *responses;
    atomic_long *failures;
} Work;

static const char REQUEST[] = "GET / HTTP/1.1\r\nHost: bench\r\n\r\n";
static const char MARKER[] = "HTTP/1.1 200";

static double now_seconds(void) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (double)time.tv_sec + (double)time.tv_nsec / 1e9;
}

static int open_connection(int port) {
    int handle = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {.sin_family = AF_INET, .sin_port = htons((unsigned short)port)};
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int yes = 1;
    setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof yes);
    if (connect(handle, (struct sockaddr *)&address, sizeof address) != 0) {
        close(handle);
        return -1;
    }
    return handle;
}

static int send_batch(Connection *connection, int pipeline) {
    char batch[sizeof REQUEST * 64];
    size_t size = 0;
    for (int index = 0; index < pipeline; index++) {
        memcpy(&batch[size], REQUEST, sizeof REQUEST - 1);
        size += sizeof REQUEST - 1;
    }
    if (write(connection->socket, batch, size) != (ssize_t)size) {
        return -1;
    }
    connection->outstanding = pipeline;
    return 0;
}

static long count_markers(Connection *connection, const char *data, size_t length) {
    char joined[sizeof connection->tail + 65536];
    memcpy(joined, connection->tail, connection->tail_length);
    memcpy(&joined[connection->tail_length], data, length);
    size_t total = connection->tail_length + length;
    long found = 0;
    for (size_t at = 0; at + sizeof MARKER - 1 <= total; at++) {
        if (memcmp(&joined[at], MARKER, sizeof MARKER - 1) == 0) {
            found++;
            at += sizeof MARKER - 2;
        }
    }
    size_t keep = total < sizeof MARKER - 2 ? total : sizeof MARKER - 2;
    memcpy(connection->tail, &joined[total - keep], keep);
    connection->tail_length = keep;
    return found;
}

static void *run(void *argument) {
    Work *work = argument;
    int events = epoll_create1(0);
    Connection *connections = calloc((size_t)work->connections, sizeof *connections);
    for (int index = 0; index < work->connections; index++) {
        connections[index].socket = open_connection(work->port);
        if (connections[index].socket < 0 || send_batch(&connections[index], work->pipeline) != 0) {
            atomic_fetch_add(work->failures, 1);
            continue;
        }
        struct epoll_event event = {.events = EPOLLIN, .data.ptr = &connections[index]};
        epoll_ctl(events, EPOLL_CTL_ADD, connections[index].socket, &event);
    }
    double end = now_seconds() + work->seconds;
    struct epoll_event ready[64];
    char buffer[65536];
    while (now_seconds() < end) {
        int count = epoll_wait(events, ready, 64, 100);
        for (int index = 0; index < count; index++) {
            Connection *connection = ready[index].data.ptr;
            ssize_t got = read(connection->socket, buffer, sizeof buffer);
            if (got <= 0) {
                atomic_fetch_add(work->failures, 1);
                epoll_ctl(events, EPOLL_CTL_DEL, connection->socket, NULL);
                continue;
            }
            long done = count_markers(connection, buffer, (size_t)got);
            atomic_fetch_add(work->responses, done);
            connection->outstanding -= (int)done;
            if (connection->outstanding <= 0 && send_batch(connection, work->pipeline) != 0) {
                atomic_fetch_add(work->failures, 1);
            }
        }
    }
    for (int index = 0; index < work->connections; index++) {
        if (connections[index].socket >= 0) {
            close(connections[index].socket);
        }
    }
    free(connections);
    close(events);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 6) {
        fprintf(stderr, "usage: http_load PORT CONNECTIONS PIPELINE SECONDS THREADS\n");
        return 2;
    }
    int port = atoi(argv[1]);
    int connections = atoi(argv[2]);
    int pipeline = atoi(argv[3]);
    double seconds = atof(argv[4]);
    int threads = atoi(argv[5]);
    if (port <= 0 || connections <= 0 || pipeline <= 0 || pipeline > 64 || seconds <= 0 || threads <= 0) {
        fprintf(stderr, "http_load: invalid arguments\n");
        return 2;
    }
    atomic_long responses = 0;
    atomic_long failures = 0;
    pthread_t *workers = calloc((size_t)threads, sizeof *workers);
    Work *work = calloc((size_t)threads, sizeof *work);
    for (int index = 0; index < threads; index++) {
        work[index] = (Work){connections / threads + (index < connections % threads), pipeline, port, seconds, &responses, &failures};
        pthread_create(&workers[index], NULL, run, &work[index]);
    }
    for (int index = 0; index < threads; index++) {
        pthread_join(workers[index], NULL);
    }
    printf("%.0f requests/s, %ld failures\n", (double)atomic_load(&responses) / seconds, (long)atomic_load(&failures));
    free(workers);
    free(work);
    return 0;
}
