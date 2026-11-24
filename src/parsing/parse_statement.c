#include "parsing/parsing.h"

#include "common/diagnostics.h"

Statement *parse_statement(TokenStream *stream) {
    const Token *token = stream_current(stream);
    if (token->kind == TOKEN_ASM) {
        stream_advance(stream);
        Statement *statement = new_statement(STATEMENT_ASM, token->line, token->column);
        statement->asm_text = token->text;
        return statement;
    }
    if (stream_is_operator(stream, "{", 0)) {
        return parse_block(stream);
    }
    if (stream_accept_keyword(stream, "const")) {
        Statement *declaration = parse_declaration(stream, true);
        if (declaration->declaration.initial == NULL) {
            compile_error(token->line, token->column, "const '%s' needs a value, as in const int %s = 10;",
                          declaration->declaration.name, declaration->declaration.name);
        }
        declaration->declaration.is_const = true;
        return declaration;
    }
    if (starts_declaration(stream)) {
        return parse_declaration(stream, true);
    }
    Statement *control = parse_control_statement(stream);
    if (control != NULL) {
        return control;
    }
    Statement *statement = parse_simple_statement(stream);
    stream_expect_operator(stream, ";");
    return statement;
}
