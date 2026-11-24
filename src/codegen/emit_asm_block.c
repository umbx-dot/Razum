#include "codegen/codegen.h"

#include <string.h>

static bool is_blank(const char *line, size_t length) {
    for (size_t index = 0; index < length; index++) {
        if (line[index] != ' ' && line[index] != '\t' && line[index] != '\r') {
            return false;
        }
    }
    return true;
}

static size_t indentation(const char *line, size_t length) {
    size_t count = 0;
    while (count < length && (line[count] == ' ' || line[count] == '\t')) {
        count++;
    }
    return count;
}

static bool is_label_line(const char *line, size_t length) {
    size_t index = 0;
    while (index < length && line[index] != ':' && line[index] != ' ' && line[index] != '\t' && line[index] != ';') {
        index++;
    }
    return index > 0 && index < length && line[index] == ':';
}

static size_t common_indentation(const char *text) {
    size_t smallest = (size_t)-1;
    for (const char *line = text; *line != '\0';) {
        size_t length = strcspn(line, "\n");
        if (!is_blank(line, length)) {
            size_t indent = indentation(line, length);
            smallest = indent < smallest ? indent : smallest;
        }
        line += length + (line[length] == '\n');
    }
    return smallest == (size_t)-1 ? 0 : smallest;
}

void emit_asm_block(FunctionContext *context, const char *text) {
    size_t drop = common_indentation(text);
    for (const char *line = text; *line != '\0';) {
        size_t length = strcspn(line, "\n");
        if (!is_blank(line, length)) {
            const char *start = line + drop;
            size_t visible = length - drop;
            while (visible > 0 && start[visible - 1] == '\r') {
                visible--;
            }
            const char *indent = is_label_line(start, visible) ? "" : "    ";
            text_line(&context->body, "%s%.*s", indent, (int)visible, start);
        }
        line += length + (line[length] == '\n');
    }
}
