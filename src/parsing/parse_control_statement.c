#include "parsing/parsing.h"

#include "common/arena.h"
#include "common/diagnostics.h"

static Expression *parse_condition(TokenStream *stream) {
    stream_expect_operator(stream, "(");
    Expression *condition = parse_expression(stream);
    stream_expect_operator(stream, ")");
    return condition;
}

static Statement *parse_if(TokenStream *stream, const Token *start) {
    Statement *statement = new_statement(STATEMENT_IF, start->line, start->column);
    statement->if_statement.condition = parse_condition(stream);
    statement->if_statement.then_branch = parse_statement(stream);
    if (stream_accept_keyword(stream, "else")) {
        statement->if_statement.else_branch = parse_statement(stream);
    }
    return statement;
}

static Statement *parse_while(TokenStream *stream, const Token *start) {
    Statement *statement = new_statement(STATEMENT_WHILE, start->line, start->column);
    statement->while_statement.condition = parse_condition(stream);
    statement->while_statement.body = parse_statement(stream);
    return statement;
}

static Statement *parse_for(TokenStream *stream, const Token *start) {
    Statement *statement = new_statement(STATEMENT_FOR, start->line, start->column);
    stream_expect_operator(stream, "(");
    if (!stream_is_operator(stream, ";", 0)) {
        statement->for_statement.initial = starts_declaration(stream)
            ? parse_declaration(stream, false)
            : parse_simple_statement(stream);
    }
    stream_expect_operator(stream, ";");
    if (!stream_is_operator(stream, ";", 0)) {
        statement->for_statement.condition = parse_expression(stream);
    }
    stream_expect_operator(stream, ";");
    if (!stream_is_operator(stream, ")", 0)) {
        statement->for_statement.step = parse_simple_statement(stream);
    }
    stream_expect_operator(stream, ")");
    statement->for_statement.body = parse_statement(stream);
    return statement;
}

static Statement *parse_do(TokenStream *stream, const Token *start) {
    Statement *statement = new_statement(STATEMENT_DO, start->line, start->column);
    statement->while_statement.body = parse_statement(stream);
    const Token *found = stream_current(stream);
    if (!stream_accept_keyword(stream, "while")) {
        compile_error(found->line, found->column, "a do loop ends with while (condition);, found %s", describe_token(found));
    }
    statement->while_statement.condition = parse_condition(stream);
    stream_expect_operator(stream, ";");
    return statement;
}

static SwitchCase *parse_case(TokenStream *stream) {
    const Token *start = stream_current(stream);
    SwitchCase *switch_case = arena_alloc(sizeof *switch_case);
    switch_case->line = start->line;
    switch_case->column = start->column;
    if (stream_accept_keyword(stream, "default")) {
        switch_case->is_default = true;
    } else if (stream_accept_keyword(stream, "case")) {
        do {
            list_push(&switch_case->values, parse_expression(stream));
        } while (stream_accept_operator(stream, ","));
    } else {
        compile_error(start->line, start->column, "expected 'case' or 'default' in a switch, found %s", describe_token(start));
    }
    stream_expect_operator(stream, ":");
    Statement *body = new_statement(STATEMENT_BLOCK, start->line, start->column);
    while (!stream_is_keyword(stream, "case") && !stream_is_keyword(stream, "default") && !stream_is_operator(stream, "}", 0) &&
           stream_current(stream)->kind != TOKEN_END) {
        list_push(&body->block, parse_statement(stream));
    }
    switch_case->body = body;
    return switch_case;
}

static Statement *parse_switch(TokenStream *stream, const Token *start) {
    Statement *statement = new_statement(STATEMENT_SWITCH, start->line, start->column);
    statement->switch_statement.value = parse_condition(stream);
    stream_expect_operator(stream, "{");
    while (!stream_accept_operator(stream, "}")) {
        list_push(&statement->switch_statement.cases, parse_case(stream));
    }
    return statement;
}

static Statement *parse_return(TokenStream *stream, const Token *start) {
    Statement *statement = new_statement(STATEMENT_RETURN, start->line, start->column);
    if (!stream_is_operator(stream, ";", 0)) {
        statement->return_value = parse_expression(stream);
    }
    stream_expect_operator(stream, ";");
    return statement;
}

static Statement *parse_jump(TokenStream *stream, const Token *start, StatementKind kind) {
    stream_expect_operator(stream, ";");
    return new_statement(kind, start->line, start->column);
}

Statement *parse_control_statement(TokenStream *stream) {
    const Token *start = stream_current(stream);
    if (stream_accept_keyword(stream, "if")) {
        return parse_if(stream, start);
    }
    if (stream_accept_keyword(stream, "while")) {
        return parse_while(stream, start);
    }
    if (stream_accept_keyword(stream, "for")) {
        return parse_for(stream, start);
    }
    if (stream_accept_keyword(stream, "do")) {
        return parse_do(stream, start);
    }
    if (stream_accept_keyword(stream, "switch")) {
        return parse_switch(stream, start);
    }
    if (stream_accept_keyword(stream, "return")) {
        return parse_return(stream, start);
    }
    if (stream_accept_keyword(stream, "break")) {
        return parse_jump(stream, start, STATEMENT_BREAK);
    }
    if (stream_accept_keyword(stream, "continue")) {
        return parse_jump(stream, start, STATEMENT_CONTINUE);
    }
    return NULL;
}
