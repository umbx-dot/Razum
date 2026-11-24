#include "parsing/parsing.h"

#include "common/arena.h"
#include "common/diagnostics.h"

static Field *parse_field(TokenStream *stream) {
    const Token *start = stream_current(stream);
    if (!starts_declaration(stream)) {
        compile_error(start->line, start->column, "expected a field such as int x; found %s", describe_token(start));
    }
    Field *field = arena_alloc(sizeof *field);
    Shape *base = parse_shape(stream);
    field->name = stream_expect_name(stream, "a field name")->text;
    field->shape = parse_array_suffix(stream, base);
    field->line = start->line;
    field->column = start->column;
    stream_expect_operator(stream, ";");
    return field;
}

StructType *parse_struct(TokenStream *stream) {
    const Token *start = stream_advance(stream);
    StructType *structure = arena_alloc(sizeof *structure);
    structure->name = stream_expect_name(stream, "a struct name")->text;
    structure->line = start->line;
    structure->column = start->column;
    stream_expect_operator(stream, "{");
    while (!stream_accept_operator(stream, "}")) {
        list_push(&structure->fields, parse_field(stream));
    }
    if (structure->fields.count == 0) {
        compile_error(start->line, start->column, "struct '%s' needs at least one field", structure->name);
    }
    stream_accept_operator(stream, ";");
    return structure;
}
