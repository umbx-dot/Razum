#include "codegen/codegen.h"

#include <stdio.h>
#include <string.h>

#include "common/arena.h"

void emit(FunctionContext *context, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    text_append(&context->body, "    ");
    text_append_formatted(&context->body, format, arguments);
    text_append(&context->body, "\n");
    va_end(arguments);
}

void emit_label(FunctionContext *context, const char *label) {
    text_line(&context->body, "%s:", label);
}

void emit_source_comment(FunctionContext *context, int line) {
    if (line == context->last_comment_line) {
        return;
    }
    context->last_comment_line = line;
    size_t length = 0;
    const char *start = source_line(context->program, context->function->source_text, line, &length);
    text_line(&context->body, "    ; %d: %.*s", line, (int)length, start);
}

const char *new_label(FunctionContext *context, const char *kind) {
    size_t size = strlen(kind) + 24;
    char *label = arena_alloc(size);
    snprintf(label, size, ".@%s%d", kind, ++context->program->label_count);
    return label;
}

Operand acquire_temp(FunctionContext *context) {
    Operand operand = {0};
    snprintf(operand.text, sizeof operand.text, "qword [rsp + .@t%d]", context->temps_in_use);
    context->temps_in_use++;
    if (context->temps_in_use > context->temp_count) {
        context->temp_count = context->temps_in_use;
    }
    return operand;
}

void release_temp(FunctionContext *context) {
    context->temps_in_use--;
}
