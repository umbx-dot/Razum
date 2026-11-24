#include "common/text_buffer.h"

void text_line(TextBuffer *buffer, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    text_append_formatted(buffer, format, arguments);
    va_end(arguments);
    text_append_bytes(buffer, "\n", 1);
}
