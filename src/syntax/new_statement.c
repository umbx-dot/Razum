#include "syntax/syntax.h"

#include "common/arena.h"

Statement *new_statement(StatementKind kind, int line, int column) {
    Statement *statement = arena_alloc(sizeof *statement);
    statement->kind = kind;
    statement->line = line;
    statement->column = column;
    return statement;
}
