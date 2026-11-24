#pragma once

#include "parsing/token_stream.h"
#include "syntax/syntax.h"

void parse_file(TokenArray tokens, Program *program, const char *source_path, const char *source_text, bool is_library);

Expression *parse_expression(TokenStream *stream);
PointerList parse_arguments(TokenStream *stream);
Statement *parse_statement(TokenStream *stream);
Statement *parse_block(TokenStream *stream);
Statement *parse_declaration(TokenStream *stream, bool require_semicolon);
Statement *parse_simple_statement(TokenStream *stream);
Statement *parse_control_statement(TokenStream *stream);
ValueType parse_type(TokenStream *stream);
bool starts_declaration(const TokenStream *stream);
Shape *parse_shape(TokenStream *stream);
Shape *parse_array_suffix(TokenStream *stream, Shape *element);
Expression *parse_initializer(TokenStream *stream);
Expression *parse_postfix(TokenStream *stream, Expression *base);
StructType *parse_struct(TokenStream *stream);
EnumType *parse_enum(TokenStream *stream);
