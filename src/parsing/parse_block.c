#include "parsing/parsing.h"

#include "common/diagnostics.h"

Statement *parse_block(TokenStream *stream) {
    const Token *start = stream_expect_operator(stream, "{");
    Statement *block = new_statement(STATEMENT_BLOCK, start->line, start->column);
    while (!stream_accept_operator(stream, "}")) {
        const Token *token = stream_current(stream);
        if (token->kind == TOKEN_END) {
            compile_error(token->line, token->column, "the block opened at line %d is never closed with }", start->line);
        }
        list_push(&block->block, parse_statement(stream));
    }
    return block;
}
