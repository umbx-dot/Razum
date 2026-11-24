#include "parsing/parsing.h"

#include "common/arena.h"
#include "common/diagnostics.h"

static EnumMember *parse_member(TokenStream *stream, int64_t *next) {
    const Token *name = stream_expect_name(stream, "an enum member name");
    EnumMember *member = arena_alloc(sizeof *member);
    member->name = name->text;
    member->line = name->line;
    member->column = name->column;
    if (stream_accept_operator(stream, "=")) {
        bool negative = stream_accept_operator(stream, "-");
        const Token *value = stream_current(stream);
        if (value->kind != TOKEN_INT || value->int_value > 0x80000000u) {
            compile_error(value->line, value->column, "an enum value must be a whole number that fits an int, such as 4 or -1, found %s",
                          describe_token(value));
        }
        stream_advance(stream);
        *next = negative ? -(int64_t)value->int_value : (int64_t)value->int_value;
    }
    member->value = *next;
    *next += 1;
    return member;
}

EnumType *parse_enum(TokenStream *stream) {
    const Token *start = stream_advance(stream);
    EnumType *enumeration = arena_alloc(sizeof *enumeration);
    enumeration->name = stream_expect_name(stream, "an enum name")->text;
    enumeration->line = start->line;
    enumeration->column = start->column;
    stream_expect_operator(stream, "{");
    int64_t next = 0;
    while (!stream_accept_operator(stream, "}")) {
        list_push(&enumeration->members, parse_member(stream, &next));
        if (!stream_accept_operator(stream, ",")) {
            stream_expect_operator(stream, "}");
            break;
        }
    }
    if (enumeration->members.count == 0) {
        compile_error(start->line, start->column, "enum '%s' needs at least one member", enumeration->name);
    }
    stream_accept_operator(stream, ";");
    return enumeration;
}
