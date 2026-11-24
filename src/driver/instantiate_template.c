#include "driver/driver.h"

#include <ctype.h>
#include <string.h>

#include "common/text_buffer.h"

const TemplateType TEMPLATE_TYPES[] = {
    {"char", "8", "0", "long", true},
    {"byte", "8", "0", "long", true},
    {"sbyte", "8", "128", "long", true},
    {"short", "16", "32768", "long", true},
    {"ushort", "16", "0", "long", true},
    {"int", "32", "2147483648", "long", true},
    {"uint", "32", "0", "long", true},
    {"long", "64", "9223372036854775808", "long", true},
    {"ulong", "64", "0", "ulong", true},
    {"float", "32", "0", "double", false},
    {"double", "64", "0", "double", false},
};
const int TEMPLATE_TYPE_COUNT = sizeof TEMPLATE_TYPES / sizeof TEMPLATE_TYPES[0];

static bool is_name_character(char character) {
    return isalnum((unsigned char)character) || character == '_';
}

static const char *replacement(const char *name, size_t length, const TemplateType *type) {
    const char *const placeholders[] = {"T", "T_BITS", "T_FLIP", "T_SUM"};
    const char *const values[] = {type->name, type->bits, type->flip, type->sum};
    for (size_t index = 0; index < sizeof placeholders / sizeof placeholders[0]; index++) {
        if (strlen(placeholders[index]) == length && strncmp(name, placeholders[index], length) == 0) {
            return values[index];
        }
    }
    return NULL;
}

static const char *skip_string(const char *cursor) {
    cursor++;
    while (*cursor != '\0' && *cursor != '"') {
        cursor += cursor[0] == '\\' && cursor[1] != '\0' ? 2 : 1;
    }
    return *cursor == '"' ? cursor + 1 : cursor;
}

char *instantiate_template(const char *text, const TemplateType *type) {
    TextBuffer output = {0};
    const char *cursor = text;
    while (*cursor != '\0') {
        const char *start = cursor;
        if (*cursor == '"') {
            cursor = skip_string(cursor);
        } else if (is_name_character(*cursor)) {
            while (is_name_character(*cursor)) {
                cursor++;
            }
        } else {
            cursor++;
        }
        size_t length = (size_t)(cursor - start);
        bool is_name = isalpha((unsigned char)*start) || *start == '_';
        const char *value = is_name ? replacement(start, length, type) : NULL;
        if (value != NULL) {
            text_append(&output, value);
        } else if (is_name && length > 2 && start[length - 2] == '_' && start[length - 1] == 'T') {
            text_append_bytes(&output, start, length - 1);
            text_append(&output, type->name);
        } else {
            text_append_bytes(&output, start, length);
        }
    }
    text_append(&output, "");
    return output.data;
}
