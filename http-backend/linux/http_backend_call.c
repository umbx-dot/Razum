#include "transport.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t used;
    size_t limit;
    bool exceeded;
} Buffer;

static size_t collect(char *data, size_t size, size_t count, void *context) {
    Buffer *buffer = context;
    if (size != 0 && count > (buffer->limit - buffer->used) / size) {
        buffer->exceeded = true;
        return 0;
    }
    size_t length = size * count;
    memcpy(buffer->data + buffer->used, data, length);
    buffer->used += length;
    return length;
}

static size_t collect_header(char *data, size_t size, size_t count, void *context) {
    Buffer *buffer = context;
    if (size == 1 && count >= 5 && memcmp(data, "HTTP/", 5) == 0) {
        buffer->used = 0;
    }
    return collect(data, size, count, context);
}

static bool load_headers(char *headers, struct curl_slist **result) {
    for (char *start = headers; *start != '\0';) {
        char *end = strstr(start, "\r\n");
        if (end == NULL) {
            return false;
        }
        *end = '\0';
        struct curl_slist *added = curl_slist_append(*result, start);
        if (added == NULL) {
            return false;
        }
        *result = added;
        start = end + 2;
    }
    return true;
}

static int32_t perform_call(const int32_t *size, char *url, char *method, char *headers, char *body, char *ca,
                            Buffer *out, Buffer *received_headers) {
    struct curl_slist *list = NULL;
    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return HTTP_BACKEND_FAILURE;
    }
    bool configured = load_headers(headers, &list) && http_transport_tls(curl, url, "https", ca, size[5]) &&
                      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, collect) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_WRITEDATA, out) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, collect_header) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_HEADERDATA, received_headers) == CURLE_OK;
    if (configured) {
        configured = strcmp(method, "HEAD") == 0
            ? curl_easy_setopt(curl, CURLOPT_NOBODY, 1L) == CURLE_OK
            : curl_easy_setopt(curl, CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)size[6]) == CURLE_OK;
    }
    if (configured && size[3] > 0) {
        configured = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body) == CURLE_OK &&
                     curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)size[3]) == CURLE_OK;
    }
    CURLcode code = configured ? curl_easy_perform(curl) : CURLE_FAILED_INIT;
    long status = 0;
    bool got_status = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status) == CURLE_OK &&
                      status >= 100 && status <= 599;
    curl_easy_cleanup(curl);
    curl_slist_free_all(list);
    if (out->exceeded || received_headers->exceeded || code == CURLE_FILESIZE_EXCEEDED) {
        return HTTP_LIMIT;
    }
    return code == CURLE_OK ? (got_status ? (int32_t)status : HTTP_PROTOCOL) : http_transport_code(code);
}

int http_backend_call(void) {
    int32_t size[7];
    static const int32_t limits[] = {4096, 16, 8192, 67108864, 4096};
    if (!http_transport_read(size, sizeof size) || size[5] <= 0 || size[6] < 0 || size[6] > HTTP_MAX_BODY) {
        return 1;
    }
    size_t total = 0;
    for (size_t index = 0; index < sizeof limits / sizeof limits[0]; index++) {
        if (size[index] < 0 || size[index] > limits[index]) {
            return 1;
        }
        total += (size_t)size[index] + 1;
    }
    char *input = malloc(total);
    Buffer body = {malloc((size_t)size[6] + 1), 0, (size_t)size[6], false};
    Buffer headers = {malloc(HTTP_MAX_HEADERS), 0, HTTP_MAX_HEADERS, false};
    char *parts[5];
    bool ready = input != NULL && body.data != NULL && headers.data != NULL;
    size_t offset = 0;
    for (size_t index = 0; index < sizeof parts / sizeof parts[0] && ready; index++) {
        parts[index] = input + offset;
        ready = http_transport_read(parts[index], (size_t)size[index]);
        parts[index][size[index]] = '\0';
        offset += (size_t)size[index] + 1;
    }
    int32_t status = HTTP_BACKEND_FAILURE;
    if (ready && strncmp(parts[0], "https://", 8) == 0 && curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK) {
        status = perform_call(size, parts[0], parts[1], parts[2], parts[3], parts[4], &body, &headers);
        curl_global_cleanup();
    }
    int32_t reply[] = {status, status > 0 ? (int32_t)body.used : 0, status > 0 ? (int32_t)headers.used : 0};
    bool sent = http_transport_write(reply, sizeof reply) &&
                http_transport_write(body.data, (size_t)reply[1]) &&
                http_transport_write(headers.data, (size_t)reply[2]) && fflush(stdout) == 0;
    free(input);
    free(body.data);
    free(headers.data);
    return sent ? 0 : 1;
}
