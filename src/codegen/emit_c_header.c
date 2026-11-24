#include "codegen/codegen.h"

#include <stdio.h>
#include <string.h>

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

static void append_type(TextBuffer *output, const Shape *shape) {
    if (shape->kind == TYPE_POINTER) {
        append_type(output, shape->element);
        text_append(output, " *");
    } else if (shape->kind == TYPE_STRUCT) {
        text_append(output, shape->struct_name);
    } else {
        text_append(output, c_scalar(shape->kind));
    }
}

static void append_declaration(TextBuffer *output, const Shape *shape, const char *name) {
    const Shape *base = shape;
    while (base->kind == TYPE_ARRAY) {
        base = base->element;
    }
    append_type(output, base);
    text_append(output, " ");
    text_append(output, name);
    for (const Shape *level = shape; level->kind == TYPE_ARRAY; level = level->element) {
        char size[24];
        snprintf(size, sizeof size, "[%d]", level->count);
        text_append(output, size);
    }
}

static void append_parameter(TextBuffer *output, const Parameter *parameter) {
    if (parameter->type == TYPE_STRUCT) {
        append_type(output, parameter->shape);
        text_append(output, " *");
        text_append(output, parameter->name);
    } else if (parameter->type == TYPE_ARRAY) {
        append_type(output, parameter->shape->element);
        text_append(output, " *");
        text_append(output, parameter->name);
        text_append(output, ", int64_t ");
        text_append(output, parameter->name);
        text_append(output, "_length");
    } else {
        append_type(output, parameter->shape);
        text_append(output, " ");
        text_append(output, parameter->name);
    }
}

static const StructType *struct_in(const Shape *shape) {
    while (shape->kind == TYPE_ARRAY) {
        shape = shape->element;
    }
    return shape->kind == TYPE_STRUCT ? shape->structure : NULL;
}

static void define_struct(TextBuffer *output, const StructType *structure, PointerList *defined) {
    for (int index = 0; index < defined->count; index++) {
        if (LIST_AT(*defined, StructType, index) == structure) {
            return;
        }
    }
    list_push(defined, (void *)structure);
    for (int index = 0; index < structure->fields.count; index++) {
        const StructType *inner = struct_in(LIST_AT(structure->fields, Field, index)->shape);
        if (inner != NULL) {
            define_struct(output, inner, defined);
        }
    }
    text_line(output, "struct %s {", structure->name);
    for (int index = 0; index < structure->fields.count; index++) {
        const Field *field = LIST_AT(structure->fields, Field, index);
        text_append(output, "    ");
        append_declaration(output, field->shape, field->name);
        text_line(output, ";");
    }
    text_line(output, "};");
    text_line(output, "");
}

char *emit_c_header(const Program *program) {
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
    for (int index = 0; index < program->structs.count; index++) {
        const StructType *structure = LIST_AT(program->structs, StructType, index);
        text_line(&output, "typedef struct %s %s;", structure->name, structure->name);
    }
    if (program->structs.count > 0) {
        text_line(&output, "");
    }
    PointerList defined = {0};
    for (int index = 0; index < program->structs.count; index++) {
        define_struct(&output, LIST_AT(program->structs, StructType, index), &defined);
    }
    for (int index = 0; index < program->functions.count; index++) {
        const Function *function = LIST_AT(program->functions, Function, index);
        if (!function->is_exported) {
            continue;
        }
        text_append(&output, "__declspec(dllimport) ");
        append_type(&output, function->return_shape);
        text_append(&output, " ");
        text_append(&output, function->name);
        text_append(&output, "(");
        for (int parameter_index = 0; parameter_index < function->parameters.count; parameter_index++) {
            text_append(&output, parameter_index > 0 ? ", " : "");
            append_parameter(&output, LIST_AT(function->parameters, Parameter, parameter_index));
        }
        text_line(&output, function->parameters.count == 0 ? "void);" : ");");
    }
    text_line(&output, "");
    text_line(&output, "#ifdef __cplusplus");
    text_line(&output, "}");
    text_line(&output, "#endif");
    return output.data;
}
