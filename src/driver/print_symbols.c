#include "driver/driver.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#include "checking/checking.h"

static void print_symbol(const char *kind, const char *name, const char *path, int line, int column, int scope_start,
                         int scope_end, const char *detail_format, ...) {
    printf("%s\t%s\t", kind, name);
    va_list arguments;
    va_start(arguments, detail_format);
    vprintf(detail_format, arguments);
    va_end(arguments);
    printf("\t%s\t%d\t%d\t%d\t%d\n", path, line, column, scope_start, scope_end);
}

static void print_signature(const Function *function) {
    printf("%s %s(", describe_shape(function->return_shape), function->name);
    for (int index = 0; index < function->parameters.count; index++) {
        const Parameter *parameter = LIST_AT(function->parameters, Parameter, index);
        printf("%s%s %s", index > 0 ? ", " : "", describe_shape(parameter->shape), parameter->name);
    }
    printf(")");
}

static void print_constant_value(const Expression *value) {
    if (value == NULL) {
        return;
    }
    if (value->kind == EXPRESSION_INT && value->type == TYPE_BOOL) {
        printf(" = %s", value->int_literal.value != 0 ? "true" : "false");
    } else if (value->kind == EXPRESSION_INT && value->type == TYPE_CHAR && value->int_literal.value >= 32 && value->int_literal.value < 127) {
        printf(" = '%c'", (char)value->int_literal.value);
    } else if (value->kind == EXPRESSION_INT) {
        printf(" = %" PRId64, value->int_literal.value);
    } else if (value->kind == EXPRESSION_DOUBLE) {
        printf(" = %.15g", value->double_literal);
    } else if (value->kind == EXPRESSION_STRING) {
        printf(" = \"%.*s\"", (int)value->string.length, value->string.bytes);
    }
}

static void print_function(const Function *function) {
    printf("function\t%s\t", function->name);
    if (function->extern_dll != NULL) {
        printf("extern \"%s\" ", function->extern_dll);
    } else if (function->is_exported) {
        printf("export ");
    }
    print_signature(function);
    printf("\t%s\t%d\t%d\t0\t0\n", function->source_path, function->line, function->column);
    for (int index = 0; index < function->declared.count; index++) {
        const Symbol *symbol = LIST_AT(function->declared, Symbol, index);
        const char *kind = symbol->kind == SYMBOL_PARAMETER ? "parameter" : symbol->is_const ? "const" : "local";
        printf("%s\t%s\t%s%s %s", kind, symbol->name, symbol->is_const ? "const " : "", describe_shape(symbol->shape), symbol->name);
        if (symbol->is_const) {
            print_constant_value(symbol->constant);
        }
        printf("\t%s\t%d\t%d\t%d\t%d\n", function->source_path, symbol->line, symbol->column, symbol->line, function->end_line);
    }
}

static void print_global(const GlobalVariable *variable) {
    printf("%s\t%s\t%s%s %s", variable->is_const ? "const" : "global", variable->name, variable->is_const ? "const " : "",
           describe_shape(variable->shape), variable->name);
    if (variable->is_const && variable->symbol != NULL) {
        print_constant_value(variable->symbol->constant);
    }
    printf("\t%s\t%d\t%d\t0\t0\n", variable->source_path, variable->line, variable->column);
}

static void print_struct(const StructType *structure) {
    printf("struct\t%s\tstruct %s {", structure->name, structure->name);
    for (int index = 0; index < structure->fields.count; index++) {
        const Field *field = LIST_AT(structure->fields, Field, index);
        printf(" %s %s;", describe_shape(field->shape), field->name);
    }
    printf(" }\t%s\t%d\t%d\t0\t0\n", structure->source_path, structure->line, structure->column);
    for (int index = 0; index < structure->fields.count; index++) {
        const Field *field = LIST_AT(structure->fields, Field, index);
        print_symbol("field", field->name, structure->source_path, field->line, field->column, 0, 0, "%s %s.%s",
                     describe_shape(field->shape), structure->name, field->name);
    }
}

static void print_enum(const EnumType *enumeration) {
    printf("enum\t%s\tenum %s {", enumeration->name, enumeration->name);
    for (int index = 0; index < enumeration->members.count; index++) {
        const EnumMember *member = LIST_AT(enumeration->members, EnumMember, index);
        printf("%s %s = %" PRId64, index > 0 ? "," : "", member->name, member->value);
    }
    printf(" }\t%s\t%d\t%d\t0\t0\n", enumeration->source_path, enumeration->line, enumeration->column);
    for (int index = 0; index < enumeration->members.count; index++) {
        const EnumMember *member = LIST_AT(enumeration->members, EnumMember, index);
        print_symbol("member", member->name, enumeration->source_path, member->line, member->column, 0, 0, "%s.%s = %" PRId64,
                     enumeration->name, member->name, member->value);
    }
}

void print_symbols(const Program *program) {
    puts("builtin\tprint\tvoid print(any value): writes the value and a line end; needs import stdio\t<built in>\t0\t0\t0\t0");
    if (find_function(program, "printf") == NULL) {
        puts("builtin\tprintf\tvoid printf(any value): writes without a line end; %n adds one in text; needs import stdio\t<built in>\t0\t0\t0\t0");
    }
    puts("builtin\tfree\tvoid free(T *pointer): gives memory from new back\t<built in>\t0\t0\t0\t0");
    for (int index = 0; index < program->functions.count; index++) {
        print_function(LIST_AT(program->functions, Function, index));
    }
    for (int index = 0; index < program->globals.count; index++) {
        print_global(LIST_AT(program->globals, GlobalVariable, index));
    }
    for (int index = 0; index < program->constants.count; index++) {
        print_global(LIST_AT(program->constants, GlobalVariable, index));
    }
    for (int index = 0; index < program->structs.count; index++) {
        print_struct(LIST_AT(program->structs, StructType, index));
    }
    for (int index = 0; index < program->enums.count; index++) {
        print_enum(LIST_AT(program->enums, EnumType, index));
    }
}
