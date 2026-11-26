#define _POSIX_C_SOURCE 200809L
#include "transport.h"

#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { WS_CLOSED = -31100 };

#if LIBCURL_VERSION_NUM >= 0x080300
static int64_t now_ms(void) {
    struct timespec current;
    return clock_gettime(CLOCK_MONOTONIC, &current) == 0
        ? (int64_t)current.tv_sec * 1000 + current.tv_nsec / 1000000 : 0;
}

typedef struct {
    CURL *curl;
    curl_socket_t socket;
    unsigned char *data;
    size_t used;
    size_t capacity;
    bool binary;
} WebSocket;

static int32_t wait_socket(WebSocket *socket, short events, int64_t deadline) {
    for (;;) {
        int64_t remaining = deadline - now_ms();
        if (remaining <= 0) {
            return HTTP_TIMEOUT;
        }
        struct pollfd descriptor = {socket->socket, events, 0};
        int result = poll(&descriptor, 1, (int)remaining);
        if (result >= 0) {
            return result == 0 ? HTTP_TIMEOUT : 0;
        }
        if (errno != EINTR) {
            return HTTP_TRANSFER;
        }
    }
}

static int32_t send_message(WebSocket *socket, int action, size_t length, int64_t deadline) {
    unsigned char *data = malloc(length + 1);
    if (data == NULL) {
        return HTTP_BACKEND_FAILURE;
    }
    int32_t result = HTTP_TRANSFER;
    if (http_transport_read(data, length)) {
        size_t used = 0;
        unsigned flags = (action == 2 ? CURLWS_TEXT : CURLWS_BINARY) | CURLWS_OFFSET;
        curl_off_t frame_size = (curl_off_t)length;
        do {
            size_t sent = 0;
            CURLcode code = curl_ws_send(socket->curl, data + used, length - used, &sent, frame_size, flags);
            used += sent;
            if (sent != 0 || code == CURLE_OK) {
                frame_size = 0;
                flags = CURLWS_OFFSET;
            }
            result = http_transport_code(code);
            if (code == CURLE_AGAIN || (code == CURLE_OK && used < length)) {
                result = wait_socket(socket, POLLOUT, deadline);
            }
        } while (result == 0 && used < length);
    }
    free(data);
    return result;
}

static int32_t receive_message(WebSocket *socket, int64_t deadline, int32_t *detail) {
    for (;;) {
        unsigned char part[8192];
        size_t got = 0;
        const struct curl_ws_frame *meta = NULL;
        CURLcode code = curl_ws_recv(socket->curl, part, sizeof part, &got, &meta);
        if (code == CURLE_AGAIN) {
            int32_t wait = wait_socket(socket, POLLIN, deadline);
            if (wait != 0) {
                return wait;
            }
            continue;
        }
        if (code != CURLE_OK || meta == NULL) {
            return code == CURLE_OK ? HTTP_PROTOCOL : http_transport_code(code);
        }
        if ((meta->flags & CURLWS_CLOSE) != 0) {
            *detail = got >= 2 ? (int32_t)(part[0] * 256u + part[1]) : 1005;
            return WS_CLOSED;
        }
        if ((meta->flags & (CURLWS_PING | CURLWS_PONG)) != 0) {
            if (now_ms() >= deadline) {
                return HTTP_TIMEOUT;
            }
            continue;
        }
        if (socket->used == 0) {
            socket->binary = (meta->flags & CURLWS_BINARY) != 0;
        }
        if (got > socket->capacity - socket->used) {
            return HTTP_LIMIT;
        }
        memcpy(socket->data + socket->used, part, got);
        socket->used += got;
        if (meta->bytesleft == 0 && (meta->flags & CURLWS_CONT) == 0) {
            *detail = socket->binary ? 1 : 0;
            return (int32_t)socket->used;
        }
        if (now_ms() >= deadline) {
            return HTTP_TIMEOUT;
        }
    }
}

static int serve_websocket(WebSocket *socket) {
    int32_t command[3];
    while (http_transport_read(command, sizeof command)) {
        int32_t action = command[0], length = command[1], timeout = command[2];
        if (action < 1 || action > 3 || length < 0 || length > HTTP_MAX_BODY || timeout <= 0) {
            return 1;
        }
        int64_t deadline = now_ms() + timeout;
        int32_t reply[2] = {0, 0};
        if (action == 1) {
            reply[0] = receive_message(socket, deadline, &reply[1]);
        } else {
            reply[0] = send_message(socket, action, (size_t)length, deadline);
        }
        bool received = action == 1 && reply[0] >= 0;
        if (!http_transport_write(reply, sizeof reply) ||
            (received && !http_transport_write(socket->data, (size_t)reply[0])) || fflush(stdout) != 0) {
            return 1;
        }
        if (received) {
            socket->used = 0;
        } else if (reply[0] < 0 && (action != 1 || reply[0] != HTTP_TIMEOUT)) {
            break;
        }
    }
    return 0;
}
#endif

int http_backend_websocket(void) {
    int32_t size[4];
    char url[4097], ca[4097];
    if (!http_transport_read(size, sizeof size) ||
        size[0] < 6 || size[0] > 4096 || size[1] < 0 || size[1] > 4096 ||
        size[2] <= 0 || size[3] <= 0 || size[3] > HTTP_MAX_BODY ||
        !http_transport_read(url, (size_t)size[0]) || !http_transport_read(ca, (size_t)size[1])) {
        return 1;
    }
    url[size[0]] = '\0';
    ca[size[1]] = '\0';
    int32_t status = HTTP_UNSUPPORTED;
#if LIBCURL_VERSION_NUM >= 0x080300
    if (strncmp(url, "wss://", 6) == 0 && curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK) {
        WebSocket socket = {curl_easy_init(), CURL_SOCKET_BAD, malloc((size_t)size[3]), 0, (size_t)size[3], false};
        bool ready = socket.curl != NULL && socket.data != NULL &&
                     http_transport_tls(socket.curl, url, "wss", ca, size[2]) &&
                     curl_easy_setopt(socket.curl, CURLOPT_CONNECT_ONLY, 2L) == CURLE_OK;
        status = ready ? http_transport_code(curl_easy_perform(socket.curl)) : HTTP_BACKEND_FAILURE;
        if (status == 0 && (curl_easy_getinfo(socket.curl, CURLINFO_ACTIVESOCKET, &socket.socket) != CURLE_OK ||
                            socket.socket == CURL_SOCKET_BAD)) {
            status = HTTP_BACKEND_FAILURE;
        }
        bool sent = http_transport_write(&status, sizeof status) && fflush(stdout) == 0;
        int result = !sent ? 1 : status == 0 ? serve_websocket(&socket) : 0;
        curl_easy_cleanup(socket.curl);
        free(socket.data);
        curl_global_cleanup();
        return result;
    }
#endif
    return http_transport_write(&status, sizeof status) && fflush(stdout) == 0 ? 0 : 1;
}
