#include "syntax/syntax.h"

#include <string.h>

static const TypeInfo TYPES[TYPE_COUNT] = {
    [TYPE_UNKNOWN] = {"unknown", "an unknown value", 0, KIND_NONE, false},
    [TYPE_VOID] = {"void", "void", 0, KIND_NONE, false},
    [TYPE_BOOL] = {"bool", "a bool", 1, KIND_BOOL, false},
    [TYPE_CHAR] = {"char", "a char", 1, KIND_INTEGER, false},
    [TYPE_BYTE] = {"byte", "a byte", 1, KIND_INTEGER, false},
    [TYPE_SBYTE] = {"sbyte", "an sbyte", 1, KIND_INTEGER, true},
    [TYPE_SHORT] = {"short", "a short", 2, KIND_INTEGER, true},
    [TYPE_USHORT] = {"ushort", "a ushort", 2, KIND_INTEGER, false},
    [TYPE_INT] = {"int", "an int", 4, KIND_INTEGER, true},
    [TYPE_UINT] = {"uint", "a uint", 4, KIND_INTEGER, false},
    [TYPE_LONG] = {"long", "a long", 8, KIND_INTEGER, true},
    [TYPE_ULONG] = {"ulong", "a ulong", 8, KIND_INTEGER, false},
    [TYPE_FLOAT] = {"float", "a float", 4, KIND_FLOATING, true},
    [TYPE_DOUBLE] = {"double", "a double", 8, KIND_FLOATING, true},
    [TYPE_STRING] = {"string", "a string", 8, KIND_STRING, false},
    [TYPE_POINTER] = {"pointer", "a pointer", 8, KIND_POINTER, false},
    [TYPE_FUNCTION] = {"function", "a function", 8, KIND_POINTER, false},
    [TYPE_ARRAY] = {"array", "an array", 0, KIND_AGGREGATE, false},
    [TYPE_STRUCT] = {"struct", "a struct", 0, KIND_AGGREGATE, false},
};

const TypeInfo *type_info(ValueType type) {
    return &TYPES[type];
}

ValueType type_from_name(const char *name) {
    for (int type = TYPE_VOID; type <= TYPE_STRING; type++) {
        if (strcmp(TYPES[type].name, name) == 0) {
            return (ValueType)type;
        }
    }
    return TYPE_UNKNOWN;
}

const char *type_name(ValueType type) {
    return TYPES[type].name;
}

const char *type_with_article(ValueType type) {
    return TYPES[type].with_article;
}
