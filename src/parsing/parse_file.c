#include "parsing/parsing.h"

#include <stdlib.h>
#include <string.h>

#include "common/arena.h"
#include "common/diagnostics.h"
#include "common/text_buffer.h"

static Import *parse_import(TokenStream *stream, const char *source_path) {
    const Token *start = stream_advance(stream);
    TextBuffer name = {0};
    text_append(&name, stream_expect_name(stream, "a library or module name")->text);
    while (stream_accept_operator(stream, ".")) {
        text_append(&name, ".");
        text_append(&name, stream_expect_name(stream, "a folder name after '.'")->text);
    }
    stream_expect_operator(stream, ";");
    Import *import = arena_alloc(sizeof *import);
    import->name = arena_copy_text(name.data, name.length);
    import->line = start->line;
    import->column = start->column;
    import->source_path = source_path;
    free(name.data);
    return import;
}

static Parameter *parse_parameter(TokenStream *stream, bool allow_sized) {
    const Token *start = stream_current(stream);
    if (!starts_declaration(stream)) {
        compile_error(start->line, start->column, "expected a parameter type such as int, found %s", describe_token(start));
    }
    Parameter *parameter = arena_alloc(sizeof *parameter);
    Shape *base = parse_shape(stream);
    parameter->name = stream_expect_name(stream, "a parameter name")->text;
    parameter->shape = parse_array_suffix(stream, base);
    parameter->type = parameter->shape->kind;
    parameter->line = start->line;
    parameter->column = start->column;
    if (parameter->type == TYPE_ARRAY && parameter->shape->count > 0 && !allow_sized) {
        compile_error(start->line, start->column, "write an array parameter without its size, as in %s %s[]; its .length comes with it",
                      describe_shape(parameter->shape->element), parameter->name);
    }
    return parameter;
}

static Function *parse_function(TokenStream *stream, const Token *start, const Shape *shape, const Token *name, const char *extern_dll) {
    Function *function = arena_alloc(sizeof *function);
    function->return_type = shape->kind;
    function->return_shape = (Shape *)shape;
    function->name = name->text;
    function->line = start->line;
    function->column = start->column;
    stream_expect_operator(stream, "(");
    if (!stream_is_operator(stream, ")", 0)) {
        do {
            list_push(&function->parameters, parse_parameter(stream, extern_dll != NULL));
        } while (stream_accept_operator(stream, ","));
    }
    stream_expect_operator(stream, ")");
    function->extern_dll = extern_dll;
    if (extern_dll != NULL) {
        stream_expect_operator(stream, ";");
    } else if (stream_accept_operator(stream, ";")) {
        function->is_prototype = true;
    } else {
        function->body = parse_block(stream);
        function->end_line = stream_peek(stream, -1)->line;
    }
    return function;
}

static bool ends_with_dll(const char *text, size_t length) {
    static const char SUFFIX[] = ".dll";
    if (length <= 4) {
        return false;
    }
    for (size_t index = 0; index < 4; index++) {
        char character = text[length - 4 + index];
        char lower = character >= 'A' && character <= 'Z' ? (char)(character - 'A' + 'a') : character;
        if (lower != SUFFIX[index]) {
            return false;
        }
    }
    return true;
}

static const char *parse_dll_name(TokenStream *stream) {
    const Token *token = stream_current(stream);
    if (token->kind != TOKEN_STRING || token->string_length == 0) {
        compile_error(token->line, token->column, "extern needs a DLL name in quotes, as in extern \"user32\" int MessageBoxA(...);");
    }
    stream_advance(stream);
    size_t length = token->string_length;
    if (ends_with_dll(token->string_value, length)) {
        length -= 4;
    }
    char *dll = arena_copy_text(token->string_value, length);
    for (char *cursor = dll; *cursor != '\0'; cursor++) {
        char character = *cursor;
        bool allowed = (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
                       (character >= '0' && character <= '9') || character == '_';
        if (!allowed) {
            compile_error(token->line, token->column, "a DLL name may only hold letters, digits and _, as in \"ws2_32\"");
        }
        if (character >= 'A' && character <= 'Z') {
            *cursor = (char)(character - 'A' + 'a');
        }
    }
    return dll;
}

static GlobalVariable *parse_global(TokenStream *stream, const Token *start, Shape *base, const Token *name) {
    GlobalVariable *variable = arena_alloc(sizeof *variable);
    variable->shape = parse_array_suffix(stream, base);
    variable->type = variable->shape->kind;
    variable->name = name->text;
    variable->line = start->line;
    variable->column = start->column;
    if (stream_accept_operator(stream, "=")) {
        variable->initial = parse_initializer(stream);
    }
    stream_expect_operator(stream, ";");
    return variable;
}

typedef struct {
    TokenStream *stream;
    Program *program;
    const char *source_path;
    const char *source_text;
    bool is_library;
} ItemContext;

static void parse_item(void *data) {
    ItemContext *context = data;
    TokenStream *stream = context->stream;
    Program *program = context->program;
    const char *source_path = context->source_path;
    const char *source_text = context->source_text;
    bool is_library = context->is_library;
    const Token *start = stream_current(stream);
    bool declares_program = start->kind == TOKEN_NAME && strcmp(start->text, "program") == 0 &&
                            stream_peek(stream, 1)->kind == TOKEN_NAME && stream_is_operator(stream, ";", 2);
    if (declares_program) {
        const Token *kind = stream_peek(stream, 1);
        if (strcmp(kind->text, "gui") != 0 && strcmp(kind->text, "console") != 0) {
            compile_error(kind->line, kind->column, "a program is 'gui' (a window, no console) or 'console', as in program gui;");
        }
        program->is_window_program = strcmp(kind->text, "gui") == 0;
        stream_advance(stream);
        stream_advance(stream);
        stream_advance(stream);
    } else if (stream_is_keyword(stream, "import")) {
        list_push(&program->imports, parse_import(stream, source_path));
    } else if (stream_is_keyword(stream, "struct")) {
        StructType *structure = parse_struct(stream);
        structure->source_path = source_path;
        list_push(&program->structs, structure);
    } else if (stream_is_keyword(stream, "enum")) {
        EnumType *enumeration = parse_enum(stream);
        enumeration->source_path = source_path;
        list_push(&program->enums, enumeration);
    } else if (stream_accept_keyword(stream, "const")) {
        Shape *base = parse_shape(stream);
        const Token *name = stream_expect_name(stream, "a constant name");
        GlobalVariable *variable = parse_global(stream, start, base, name);
        if (variable->initial == NULL) {
            compile_error(start->line, start->column, "const '%s' needs a value, as in const int %s = 10;", name->text, name->text);
        }
        variable->is_const = true;
        variable->source_path = source_path;
        variable->is_library = is_library;
        list_push(&program->globals, variable);
    } else if (stream_accept_keyword(stream, "extern")) {
        const char *dll = parse_dll_name(stream);
        Shape *base = parse_shape(stream);
        const Token *name = stream_expect_name(stream, "a function name");
        if (!stream_is_operator(stream, "(", 0)) {
            compile_error(name->line, name->column, "extern declares a function, as in extern \"%s\" int %s(...);", dll, name->text);
        }
        Function *function = parse_function(stream, start, base, name, dll);
        function->source_path = source_path;
        function->source_text = source_text;
        function->is_library = is_library;
        list_push(&program->functions, function);
    } else if (stream_is_keyword(stream, "export") || starts_declaration(stream)) {
        bool is_exported = stream_accept_keyword(stream, "export");
        Shape *base = parse_shape(stream);
        const Token *name = stream_expect_name(stream, "a function or variable name");
        if (stream_is_operator(stream, "(", 0)) {
            Function *function = parse_function(stream, start, base, name, NULL);
            function->is_exported = is_exported;
            function->source_path = source_path;
            function->source_text = source_text;
            function->is_library = is_library;
            list_push(&program->functions, function);
        } else {
            GlobalVariable *variable = parse_global(stream, start, base, name);
            variable->is_exported = is_exported;
            variable->source_path = source_path;
            variable->is_library = is_library;
            list_push(&program->globals, variable);
        }
    } else if (start->kind == TOKEN_ASM) {
        compile_error(start->line, start->column, "asm blocks belong inside a function");
    } else {
        compile_error(start->line, start->column, "expected 'import', 'struct', 'enum', 'const', 'extern', a function or a global variable, found %s", describe_token(start));
    }
}

static bool is_operator(const Token *token, char text) {
    return token->kind == TOKEN_OPERATOR && token->text[0] == text && token->text[1] == '\0';
}

static bool skip_balanced_item(TokenStream *stream, int item_start) {
    int depth = 0;
    for (int index = item_start; index < stream->tokens.count; index++) {
        const Token *token = &stream->tokens.items[index];
        if (token->kind == TOKEN_END) {
            return false;
        }
        if (is_operator(token, '{')) {
            depth++;
        } else if (is_operator(token, '}')) {
            depth--;
            if (depth == 0) {
                stream->index = index + 1;
                return true;
            }
        } else if (depth == 0 && is_operator(token, ';') && index >= stream->index) {
            stream->index = index + 1;
            return true;
        }
    }
    return false;
}

static void skip_to_next_item(TokenStream *stream, int item_start) {
    if (skip_balanced_item(stream, item_start) && stream->index > item_start) {
        return;
    }
    stream_advance(stream);
    while (stream_current(stream)->kind != TOKEN_END) {
        const Token *token = stream_current(stream);
        bool starts_line_at_margin = token->column == 1 && !is_operator(token, '}');
        if (starts_line_at_margin) {
            return;
        }
        stream_advance(stream);
    }
}

void parse_file(TokenArray tokens, Program *program, const char *source_path, const char *source_text, bool is_library) {
    TokenStream stream = {tokens, 0};
    ItemContext context = {&stream, program, source_path, source_text, is_library};
    while (stream_current(&stream)->kind != TOKEN_END) {
        int item_start = stream.index;
        if (!diagnostic_try(parse_item, &context)) {
            skip_to_next_item(&stream, item_start);
        }
    }
}
