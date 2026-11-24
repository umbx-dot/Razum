#include "syntax/syntax.h"

#include "common/arena.h"

Expression *new_expression(ExpressionKind kind, int line, int column) {
    Expression *expression = arena_alloc(sizeof *expression);
    expression->kind = kind;
    expression->line = line;
    expression->column = column;
    expression->type = TYPE_UNKNOWN;
    return expression;
}
