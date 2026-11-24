#include "checking/checking.h"

#include <string.h>

#include "common/diagnostics.h"

static StructType *find_struct(const Program *program, const char *name) {
    for (int index = 0; index < program->structs.count; index++) {
        StructType *structure = LIST_AT(program->structs, StructType, index);
        if (strcmp(structure->name, name) == 0) {
            return structure;
        }
    }
    return NULL;
}

static bool resolve_enum(const Program *program, Shape *shape) {
    const EnumType *enumeration = find_enum(program, shape->struct_name);
    if (enumeration == NULL) {
        return false;
    }
    shape->kind = TYPE_INT;
    shape->enumeration = enumeration;
    return true;
}

static void resolve_count_here(const Program *program, Shape *shape) {
    if (shape->count_expression == NULL) {
        return;
    }
    Checker checker = {0};
    checker.program = (Program *)program;
    checker.scopes.program = program;
    resolve_count(&checker, shape);
}

static long long round_up(long long value, int alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

void layout_struct(const Program *program, StructType *structure) {
    if (structure->layout == LAYOUT_DONE) {
        return;
    }
    const char *previous_path = diagnostic_source_path;
    diagnostic_source_path = structure->source_path;
    if (structure->layout == LAYOUT_RUNNING) {
        compile_error(structure->line, structure->column, "struct '%s' contains itself, so it would never end", structure->name);
    }
    structure->layout = LAYOUT_RUNNING;
    long long offset = 0;
    int alignment = 1;
    for (int index = 0; index < structure->fields.count; index++) {
        Field *field = LIST_AT(structure->fields, Field, index);
        for (int earlier = 0; earlier < index; earlier++) {
            if (strcmp(LIST_AT(structure->fields, Field, earlier)->name, field->name) == 0) {
                compile_error(field->line, field->column, "struct '%s' has two fields named '%s'", structure->name, field->name);
            }
        }
        resolve_shape(program, field->shape);
        if (field->shape->kind == TYPE_VOID) {
            compile_error(field->line, field->column, "a field cannot be void");
        }
        if (field->shape->kind == TYPE_ARRAY && field->shape->count < 0) {
            compile_error(field->line, field->column, "an array field needs a size, as in char name[32];");
        }
        int field_alignment = shape_alignment(field->shape);
        offset = round_up(offset, field_alignment);
        offset += shape_size(field->shape);
        if (round_up(offset, field_alignment) > 0x7FFFFFFF) {
            compile_error(field->line, field->column, "struct '%s' is larger than 2 GiB", structure->name);
        }
        field->offset = (int)(offset - shape_size(field->shape));
        alignment = field_alignment > alignment ? field_alignment : alignment;
    }
    long long size = round_up(offset, alignment);
    if (size > 0x7FFFFFFF) {
        compile_error(structure->line, structure->column, "struct '%s' is larger than 2 GiB", structure->name);
    }
    structure->size = (int)size;
    structure->alignment = alignment;
    structure->layout = LAYOUT_DONE;
    diagnostic_source_path = previous_path;
}

static void resolve_signature(const Program *program, Shape *shape);

static void resolve_pointee(const Program *program, Shape *shape) {
    if (shape->kind == TYPE_FUNCTION) {
        resolve_signature(program, shape);
        return;
    }
    if (shape->kind == TYPE_ARRAY || shape->kind == TYPE_POINTER) {
        resolve_count_here(program, shape);
        resolve_pointee(program, (Shape *)shape->element);
        return;
    }
    if (shape->kind == TYPE_STRUCT && shape->structure == NULL && !resolve_enum(program, shape)) {
        shape->structure = find_struct(program, shape->struct_name);
        if (shape->structure == NULL) {
            compile_error(shape->line, shape->column, "unknown type '%s'; declare it with struct %s { ... } or enum %s { ... }", shape->struct_name, shape->struct_name, shape->struct_name);
        }
    }
}

static void resolve_signature(const Program *program, Shape *shape) {
    for (int index = 0; index < shape->parameters.count; index++) {
        Shape *parameter = LIST_AT(shape->parameters, Shape, index);
        if (parameter->kind == TYPE_ARRAY && parameter->count >= 0) {
            compile_error(parameter->line, parameter->column, "an array parameter in a function type has no size; write %s[]",
                          describe_shape(parameter->element));
        }
        resolve_pointee(program, parameter);
    }
    resolve_pointee(program, (Shape *)shape->result);
}

void resolve_shape(const Program *program, Shape *shape) {
    if (shape->kind == TYPE_FUNCTION) {
        resolve_signature(program, shape);
        return;
    }
    if (shape->kind == TYPE_POINTER) {
        resolve_pointee(program, (Shape *)shape->element);
        return;
    }
    if (shape->kind == TYPE_ARRAY) {
        resolve_count_here(program, shape);
        resolve_shape(program, (Shape *)shape->element);
        if (shape->count > 0 && (long long)shape->count * shape_size(shape->element) > 0x7FFFFFFF) {
            compile_error(shape->line, shape->column, "%s is larger than 2 GiB", describe_shape(shape));
        }
        return;
    }
    if (shape->kind != TYPE_STRUCT || shape->structure != NULL || resolve_enum(program, shape)) {
        return;
    }
    shape->structure = find_struct(program, shape->struct_name);
    if (shape->structure == NULL) {
        compile_error(shape->line, shape->column, "unknown type '%s'; declare it with struct %s { ... } or enum %s { ... }", shape->struct_name, shape->struct_name, shape->struct_name);
    }
    layout_struct(program, shape->structure);
}
