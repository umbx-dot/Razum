#include "transport.h"

bool http_transport_read(void *destination, size_t size) {
    return size == 0 || fread(destination, 1, size, stdin) == size;
}

bool http_transport_write(const void *source, size_t size) {
    return size == 0 || fwrite(source, 1, size, stdout) == size;
}

int32_t http_transport_code(CURLcode code) {
    switch (code) {
        case CURLE_OK: return 0;
        case CURLE_OPERATION_TIMEDOUT: return HTTP_TIMEOUT;
        case CURLE_SSL_CONNECT_ERROR:
        case CURLE_PEER_FAILED_VERIFICATION:
        case CURLE_SSL_CERTPROBLEM:
        case CURLE_SSL_CACERT_BADFILE: return HTTP_TLS;
        case CURLE_UNSUPPORTED_PROTOCOL: return HTTP_UNSUPPORTED;
        case CURLE_OUT_OF_MEMORY:
        case CURLE_FAILED_INIT: return HTTP_BACKEND_FAILURE;
        default: return HTTP_TRANSFER;
    }
}

bool http_transport_tls(CURL *curl, const char *url, const char *protocols, const char *ca, long timeout) {
    return curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK &&
           curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, protocols) == CURLE_OK &&
           curl_easy_setopt(curl, CURLOPT_DISALLOW_USERNAME_IN_URL, 1L) == CURLE_OK &&
           curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L) == CURLE_OK &&
           curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK &&
           curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L) == CURLE_OK &&
           curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2) == CURLE_OK &&
           curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L) == CURLE_OK &&
           curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout) == CURLE_OK &&
           (*ca == '\0' || curl_easy_setopt(curl, CURLOPT_CAINFO, ca) == CURLE_OK);
}
