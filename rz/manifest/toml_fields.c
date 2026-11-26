#include "project.h"

#include <limits.h>
#include <string.h>

#include "common/arena.h"
#include "common/files.h"

toml_result_t rz_read_toml(const char *file, const char **source) {
    size_t length = 0;
    const char *text = read_file(file, &length);
    if (text == NULL || length > INT_MAX) {
        rz_error("cannot read %s", file);
    }
    toml_result_t result = toml_parse_named(text, (int)length, file);
    if (!result.ok) {
        rz_error("%s: %s", file, result.errmsg);
    }
    if (source != NULL) {
        *source = text;
    }
    return result;
}

const char *rz_toml_string(toml_datum_t value, const char *field) {
    if (value.type != TOML_STRING || value.u.str.len <= 0 ||
        (size_t)value.u.str.len != strlen(value.u.s)) {
        rz_error("line %d: %s must be a nonempty string", value.lineno, field);
    }
    return arena_copy_text(value.u.s, (size_t)value.u.str.len);
}

toml_datum_t rz_toml_field(toml_datum_t table, const char *name, toml_type_t type, bool required) {
    toml_datum_t value = table.type == TOML_TABLE ? toml_get(table, name) : (toml_datum_t){0};
    if (value.type == TOML_UNKNOWN ? required : value.type != type) {
        rz_error("line %d: missing or invalid '%s'", value.lineno != 0 ? value.lineno : table.lineno, name);
    }
    return value;
}

void rz_toml_keys(toml_datum_t table, const char *const *keys) {
    if (table.type == TOML_UNKNOWN) {
        return;
    }
    for (int index = 0; index < table.u.tab.size; index++) {
        const char *key = table.u.tab.key[index];
        int allowed = 0;
        while (keys[allowed] != NULL && strcmp(keys[allowed], key) != 0) {
            allowed++;
        }
        if (keys[allowed] == NULL) {
            rz_error("line %d: unknown setting '%s'", table.u.tab.value[index].lineno, key);
        }
    }
}

void rz_toml_strings(toml_datum_t array, PointerList *result, const char *field, bool paths) {
    if (array.type == TOML_UNKNOWN) {
        return;
    }
    for (int index = 0; index < array.u.arr.size; index++) {
        const char *value = rz_toml_string(array.u.arr.elem[index], field);
        if (!(paths ? rz_relative(value) : rz_name(value))) {
            rz_error("line %d: invalid %s entry '%s'", array.u.arr.elem[index].lineno, field, value);
        }
        list_push(result, (void *)value);
    }
}
