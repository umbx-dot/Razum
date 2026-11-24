#include "common/text_buffer.h"

#include <stdio.h>
#include <stdlib.h>

#include "common/diagnostics.h"

void text_append_formatted(TextBuffer *buffer, const char *format, va_list arguments) {
    va_list measuring;
    va_copy(measuring, arguments);
    int length = vsnprintf(NULL, 0, format, measuring);
    va_end(measuring);
    if (length < 0) {
        fatal_error("invalid output format");
    }
    char *formatted = malloc((size_t)length + 1);
    if (formatted == NULL) {
        fatal_error("out of memory");
    }
    vsnprintf(formatted, (size_t)length + 1, format, arguments);
    text_append_bytes(buffer, formatted, (size_t)length);
    free(formatted);
}
