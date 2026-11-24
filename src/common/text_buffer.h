#pragma once

#include <stdarg.h>
#include <stddef.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} TextBuffer;

void text_append_bytes(TextBuffer *buffer, const char *bytes, size_t length);
void text_append(TextBuffer *buffer, const char *text);
void text_append_formatted(TextBuffer *buffer, const char *format, va_list arguments);
void text_line(TextBuffer *buffer, const char *format, ...);
