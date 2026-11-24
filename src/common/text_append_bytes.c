#include "common/text_buffer.h"

#include <stdlib.h>
#include <string.h>

#include "common/diagnostics.h"

void text_append_bytes(TextBuffer *buffer, const char *bytes, size_t length) {
    if (buffer->length + length + 1 > buffer->capacity) {
        size_t capacity = buffer->capacity == 0 ? 4096 : buffer->capacity;
        while (buffer->length + length + 1 > capacity) {
            capacity *= 2;
        }
        char *data = realloc(buffer->data, capacity);
        if (data == NULL) {
            fatal_error("out of memory");
        }
        buffer->data = data;
        buffer->capacity = capacity;
    }
    memcpy(buffer->data + buffer->length, bytes, length);
    buffer->length += length;
    buffer->data[buffer->length] = '\0';
}
