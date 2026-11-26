#pragma once

#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

enum {
    HTTP_BACKEND_FAILURE = -31001, HTTP_TIMEOUT = -31002, HTTP_LIMIT = -31003,
    HTTP_TLS = -31004, HTTP_TRANSFER = -31005, HTTP_PROTOCOL = -31006, HTTP_UNSUPPORTED = -31007,
    HTTP_MAX_BODY = 16777216, HTTP_MAX_HEADERS = 65536
};

bool http_transport_read(void *destination, size_t size);
bool http_transport_write(const void *source, size_t size);
int32_t http_transport_code(CURLcode code);
bool http_transport_tls(CURL *curl, const char *url, const char *protocols, const char *ca, long timeout);
int http_backend_call(void);
int http_backend_websocket(void);
