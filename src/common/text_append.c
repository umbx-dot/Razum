#include "common/text_buffer.h"

#include <string.h>

void text_append(TextBuffer *buffer, const char *text) {
    text_append_bytes(buffer, text, strlen(text));
}
