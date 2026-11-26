#include <curl/curl.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transport.h"

typedef struct {
    size_t remaining;
    bool limited;
} Output;

static bool read_line(char *target, int capacity) {
    if (fgets(target, capacity, stdin) == NULL) {
        return false;
    }
    char *newline = strchr(target, '\n');
    if (newline == NULL) {
        return false;
    }
    *newline = '\0';
    return true;
}

static bool read_number(long *value, long highest) {
    char line[48];
    if (!read_line(line, sizeof line) || line[0] < '0' || line[0] > '9') {
        return false;
    }
    char *end = NULL;
    *value = strtol(line, &end, 10);
    return *end == '\0' && *value >= 0 && *value <= highest;
}

static size_t write_body(char *data, size_t size, size_t count, void *context) {
    Output *output = context;
    if (size > 0 && count > output->remaining / size) {
        output->limited = true;
        return 0;
    }
    size_t bytes = size * count;
    output->remaining -= bytes;
    return fwrite(data, 1, bytes, stdout);
}

int main(int argc, char **argv) {
    if (argc == 2) {
        if (strcmp(argv[1], "--call") == 0) {
            return http_backend_call();
        }
        if (strcmp(argv[1], "--websocket") == 0) {
            return http_backend_websocket();
        }
        return CURLE_URL_MALFORMAT;
    }
    long timeout = 0;
    long maximum = 0;
    char url[4098];
    char ca[4098];
    if (!read_number(&timeout, 2147483647L) || timeout == 0 ||
        !read_number(&maximum, 16777216L) ||
        !read_line(url, sizeof url) || strncmp(url, "https://", 8) != 0 ||
        !read_line(ca, sizeof ca)) {
        return CURLE_URL_MALFORMAT;
    }
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        return CURLE_FAILED_INIT;
    }
    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        curl_global_cleanup();
        return CURLE_FAILED_INIT;
    }
    Output output = {(size_t)maximum, false};
    bool configured = curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https") == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_DISALLOW_USERNAME_IN_URL, 1L) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)maximum) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_body) == CURLE_OK &&
                      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output) == CURLE_OK &&
                      (ca[0] == '\0' || curl_easy_setopt(curl, CURLOPT_CAINFO, ca) == CURLE_OK);
    CURLcode result = configured ? curl_easy_perform(curl) : CURLE_FAILED_INIT;
    if (result == CURLE_OK) {
        long status = 0;
        if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status) != CURLE_OK ||
            status < 100 || status > 599 || fprintf(stdout, "%03ld", status) != 3 || fflush(stdout) != 0) {
            result = CURLE_WRITE_ERROR;
        }
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return output.limited ? CURLE_FILESIZE_EXCEEDED : result;
}
