#include "codegen/codegen.h"

#include <stdio.h>

static const char *named_escape(char character) {
    switch (character) {
        case '\n': return "\\n";
        case '\t': return "\\t";
        case '\r': return "\\r";
        case '\\': return "\\\\";
        case '`': return "\\`";
        default: return NULL;
    }
}

void append_nasm_string(TextBuffer *buffer, const char *bytes, size_t length) {
    if (length == 0) {
        text_append(buffer, "0");
        return;
    }
    text_append(buffer, "`");
    for (size_t index = 0; index < length; index++) {
        unsigned char byte = (unsigned char)bytes[index];
        const char *escape = named_escape((char)byte);
        if (escape != NULL) {
            text_append(buffer, escape);
        } else if (byte >= 0x20 && byte < 0x7F) {
            text_append_bytes(buffer, (const char *)&byte, 1);
        } else {
            char hex[8];
            snprintf(hex, sizeof hex, "\\x%02X", byte);
            text_append(buffer, hex);
        }
    }
    text_append(buffer, "`, 0");
}
