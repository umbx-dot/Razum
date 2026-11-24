#include "parsing/parsing.h"

#include "common/diagnostics.h"

typedef struct {
    const char *text;
    Operator operator;
} AssignmentOperator;

static const AssignmentOperator ASSIGNMENTS[] = {
    {"=", OP_ASSIGN}, {"+=", OP_ADD}, {"-=", OP_SUBTRACT},
    {"*=", OP_MULTIPLY}, {"/=", OP_DIVIDE}, {"%=", OP_REMAINDER}, {"&=", OP_BIT_AND},
    {"|=", OP_BIT_OR}, {"^=", OP_BIT_XOR}, {"<<=", OP_SHIFT_LEFT}, {">>=", OP_SHIFT_RIGHT},
};

static Expression *parse_target(TokenStream *stream, const Token *name) {
    Expression *target = new_expression(EXPRESSION_NAME, name->line, name->column);
    target->name.name = name->text;
    return parse_postfix(stream, target);
}

static Statement *increment(const Token *at, Expression *target, bool is_increment) {
    Statement *statement = new_statement(STATEMENT_INCREMENT, at->line, at->column);
    statement->increment.target = target;
    statement->increment.is_increment = is_increment;
    return statement;
}

static Statement *prefix_increment(TokenStream *stream) {
    const Token *operator = stream_advance(stream);
    const Token *name = stream_expect_name(stream, "a variable name after ++ or --");
    return increment(operator, parse_target(stream, name), operator->text[0] == '+');
}

static Expression *parse_deref_target(TokenStream *stream) {
    const Token *star = stream_advance(stream);
    Expression *deref = new_expression(EXPRESSION_DEREF, star->line, star->column);
    if (stream_is_operator(stream, "*", 0)) {
        deref->unary.operand = parse_deref_target(stream);
    } else {
        const Token *name = stream_expect_name(stream, "a pointer after *");
        deref->unary.operand = parse_target(stream, name);
    }
    return deref;
}

static Statement *finish_target(TokenStream *stream, const Token *at, Expression *target) {
    if (target->kind == EXPRESSION_CALL) {
        Statement *statement = new_statement(STATEMENT_CALL, at->line, at->column);
        statement->call = target;
        return statement;
    }
    if (stream_is_operator(stream, "++", 0) || stream_is_operator(stream, "--", 0)) {
        return increment(at, target, stream_advance(stream)->text[0] == '+');
    }
    for (size_t index = 0; index < sizeof ASSIGNMENTS / sizeof ASSIGNMENTS[0]; index++) {
        if (stream_accept_operator(stream, ASSIGNMENTS[index].text)) {
            Statement *statement = new_statement(STATEMENT_ASSIGNMENT, at->line, at->column);
            statement->assignment.target = target;
            statement->assignment.operator = ASSIGNMENTS[index].operator;
            statement->assignment.value = parse_expression(stream);
            return statement;
        }
    }
    const Token *found = stream_current(stream);
    compile_error(found->line, found->column, "expected '=', '++' or '--' after the target, found %s", describe_token(found));
}

Statement *parse_simple_statement(TokenStream *stream) {
    if (stream_is_operator(stream, "++", 0) || stream_is_operator(stream, "--", 0)) {
        return prefix_increment(stream);
    }
    if (stream_is_operator(stream, "*", 0)) {
        const Token *at = stream_current(stream);
        return finish_target(stream, at, parse_deref_target(stream));
    }
    const Token *name = stream_expect_name(stream, "a statement");
    if (stream_is_operator(stream, "(", 0)) {
        Statement *statement = new_statement(STATEMENT_CALL, name->line, name->column);
        statement->call = new_expression(EXPRESSION_CALL, name->line, name->column);
        statement->call->call.name = name->text;
        statement->call->call.arguments = parse_arguments(stream);
        return statement;
    }
    return finish_target(stream, name, parse_target(stream, name));
}
