#include "codegen/codegen.h"

#include <stdio.h>
#include <string.h>

static void append_format(TextBuffer *output, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    text_append_formatted(output, format, arguments);
    va_end(arguments);
}

static const char *c_scalar(ValueType type) {
    switch (type) {
        case TYPE_VOID: return "void";
        case TYPE_BOOL: return "bool";
        case TYPE_CHAR: return "char";
        case TYPE_BYTE: return "uint8_t";
        case TYPE_SBYTE: return "int8_t";
        case TYPE_SHORT: return "int16_t";
        case TYPE_USHORT: return "uint16_t";
        case TYPE_INT: return "int32_t";
        case TYPE_UINT: return "uint32_t";
        case TYPE_LONG: return "int64_t";
        case TYPE_ULONG: return "uint64_t";
        case TYPE_FLOAT: return "float";
        case TYPE_DOUBLE: return "double";
        default: return "const char *";
    }
}

static void append_declaration(TextBuffer *output, const Shape *shape, const char *name, bool elf);

static void append_parameter(TextBuffer *output, const Shape *shape, const char *name, bool elf) {
    if (shape->kind == TYPE_STRUCT || shape->kind == TYPE_ARRAY) {
        Shape pointer = {0};
        pointer.kind = TYPE_POINTER;
        pointer.element = shape->kind == TYPE_ARRAY ? shape->element : shape;
        append_declaration(output, &pointer, name, elf);
        if (shape->kind == TYPE_ARRAY && shape->count < 0) {
            append_format(output, ", int64_t %s_length", name);
        }
        return;
    }
    append_declaration(output, shape, name, elf);
}

static void append_declaration(TextBuffer *output, const Shape *shape, const char *name, bool elf) {
    TextBuffer declarator = {0};
    switch (shape->kind) {
        case TYPE_POINTER:
            append_format(&declarator, shape->element->kind == TYPE_ARRAY ? "(*%s)" : "*%s", name);
            append_declaration(output, shape->element, declarator.data, elf);
            return;
        case TYPE_ARRAY:
            append_format(&declarator, "%s[%d]", name, shape->count);
            append_declaration(output, shape->element, declarator.data, elf);
            return;
        case TYPE_FUNCTION:
            append_format(&declarator, elf ? "(__attribute__((ms_abi)) *%s)(" : "(*%s)(", name);
            if (shape->parameters.count == 0) {
                text_append(&declarator, "void");
            }
            for (int index = 0; index < shape->parameters.count; index++) {
                char argument[24];
                snprintf(argument, sizeof argument, "arg%d", index);
                text_append(&declarator, index > 0 ? ", " : "");
                append_parameter(&declarator, LIST_AT(shape->parameters, Shape, index), argument, elf);
            }
            text_append(&declarator, ")");
            append_declaration(output, shape->result, declarator.data, elf);
            return;
        default:
            append_format(output, "%s %s", shape->kind == TYPE_STRUCT ? shape->struct_name : c_scalar(shape->kind), name);
    }
}

static const StructType *struct_in(const Shape *shape) {
    while (shape->kind == TYPE_ARRAY) {
        shape = shape->element;
    }
    return shape->kind == TYPE_STRUCT ? shape->structure : NULL;
}

static void collect_structs(PointerList *structures, const Shape *shape) {
    if (shape->kind == TYPE_ARRAY || shape->kind == TYPE_POINTER) {
        collect_structs(structures, shape->element);
    } else if (shape->kind == TYPE_FUNCTION) {
        collect_structs(structures, shape->result);
        for (int index = 0; index < shape->parameters.count; index++) {
            collect_structs(structures, LIST_AT(shape->parameters, Shape, index));
        }
    } else if (shape->kind == TYPE_STRUCT) {
        const StructType *structure = shape->structure;
        for (int index = 0; index < structures->count; index++) {
            if (LIST_AT(*structures, StructType, index) == structure) {
                return;
            }
        }
        list_push(structures, (void *)structure);
        for (int index = 0; index < structure->fields.count; index++) {
            collect_structs(structures, LIST_AT(structure->fields, Field, index)->shape);
        }
    }
}

static void define_struct(TextBuffer *output, const StructType *structure, PointerList *defined, bool elf) {
    for (int index = 0; index < defined->count; index++) {
        if (LIST_AT(*defined, StructType, index) == structure) {
            return;
        }
    }
    list_push(defined, (void *)structure);
    for (int index = 0; index < structure->fields.count; index++) {
        const StructType *inner = struct_in(LIST_AT(structure->fields, Field, index)->shape);
        if (inner != NULL) {
            define_struct(output, inner, defined, elf);
        }
    }
    text_line(output, "struct %s {", structure->name);
    for (int index = 0; index < structure->fields.count; index++) {
        const Field *field = LIST_AT(structure->fields, Field, index);
        text_append(output, "    ");
        append_declaration(output, field->shape, field->name, elf);
        text_line(output, ";");
    }
    text_line(output, "};");
    text_line(output, "");
}

static PointerList exported_structs(const Program *program) {
    PointerList structures = {0};
    for (int index = 0; index < program->functions.count; index++) {
        const Function *function = LIST_AT(program->functions, Function, index);
        if (!function->is_exported) {
            continue;
        }
        collect_structs(&structures, function->return_shape);
        for (int parameter = 0; parameter < function->parameters.count; parameter++) {
            collect_structs(&structures, LIST_AT(function->parameters, Parameter, parameter)->shape);
        }
    }
    for (int index = 0; index < program->globals.count; index++) {
        const GlobalVariable *variable = LIST_AT(program->globals, GlobalVariable, index);
        if (variable->is_exported) {
            collect_structs(&structures, variable->shape);
        }
    }
    return structures;
}

char *emit_c_header(const Program *program, bool dll) {
    bool elf = program->target == TARGET_LINUX;
    TextBuffer output = {0};
    text_line(&output, "#pragma once");
    text_line(&output, "");
    text_line(&output, "#include <stdbool.h>");
    text_line(&output, "#include <stdint.h>");
    text_line(&output, "");
    text_line(&output, "#ifdef __cplusplus");
    text_line(&output, "extern \"C\" {");
    text_line(&output, "#endif");
    text_line(&output, "");
    PointerList structures = exported_structs(program);
    for (int index = 0; index < structures.count; index++) {
        const StructType *structure = LIST_AT(structures, StructType, index);
        text_line(&output, "typedef struct %s %s;", structure->name, structure->name);
    }
    if (structures.count > 0) {
        text_line(&output, "");
    }
    PointerList defined = {0};
    for (int index = 0; index < structures.count; index++) {
        define_struct(&output, LIST_AT(structures, StructType, index), &defined, elf);
    }
    for (int index = 0; index < program->functions.count; index++) {
        const Function *function = LIST_AT(program->functions, Function, index);
        if (!function->is_exported) {
            continue;
        }
        TextBuffer signature = {0};
        append_format(&signature, "%s(", function->name);
        for (int parameter_index = 0; parameter_index < function->parameters.count; parameter_index++) {
            const Parameter *parameter = LIST_AT(function->parameters, Parameter, parameter_index);
            text_append(&signature, parameter_index > 0 ? ", " : "");
            append_parameter(&signature, parameter->shape, parameter->name, elf);
        }
        text_append(&signature, function->parameters.count == 0 ? "void)" : ")");
        if (dll) {
            text_append(&output, "__declspec(dllimport) ");
        }
        append_declaration(&output, function->return_shape, signature.data, elf);
        text_line(&output, ";");
    }
    for (int index = 0; index < program->globals.count; index++) {
        const GlobalVariable *variable = LIST_AT(program->globals, GlobalVariable, index);
        if (variable->is_exported) {
            text_append(&output, dll ? "extern __declspec(dllimport) " : "extern ");
            append_declaration(&output, variable->shape, variable->name, elf);
            text_line(&output, ";");
        }
    }
    text_line(&output, "");
    text_line(&output, "#ifdef __cplusplus");
    text_line(&output, "}");
    text_line(&output, "#endif");
    return output.data;
}
