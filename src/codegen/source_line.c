#include "codegen/codegen.h"

#include <string.h>

#include "common/arena.h"

static LineIndex *index_text(ProgramContext *program, const char *text) {
    for (int index = 0; index < program->line_indexes.count; index++) {
        LineIndex *existing = LIST_AT(program->line_indexes, LineIndex, index);
        if (existing->text == text) {
            return existing;
        }
    }
    LineIndex *line_index = arena_alloc(sizeof *line_index);
    line_index->text = text;
    list_push(&line_index->starts, (void *)text);
    for (const char *newline = strchr(text, '\n'); newline != NULL; newline = strchr(newline + 1, '\n')) {
        list_push(&line_index->starts, (void *)(newline + 1));
    }
    list_push(&program->line_indexes, line_index);
    return line_index;
}

const char *source_line(ProgramContext *program, const char *text, int line, size_t *length) {
    LineIndex *line_index = index_text(program, text);
    const char *start = LIST_AT(line_index->starts, const char, line - 1);
    while (*start == ' ' || *start == '\t') {
        start++;
    }
    *length = strcspn(start, "\r\n");
    return start;
}
