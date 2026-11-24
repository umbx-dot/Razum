#include "linking/linking.h"

#include <string.h>

static bool is_space(char character) {
    return character == ' ' || character == '\t';
}

static bool defines_here(const char *line, const char *label, size_t length) {
    if (strncmp(line, label, length) != 0) {
        return false;
    }
    const char *rest = line + length;
    if (*rest == ':') {
        return true;
    }
    if (!is_space(*rest)) {
        return false;
    }
    while (is_space(*rest)) {
        rest++;
    }
    return strncmp(rest, "equ", 3) == 0 && is_space(rest[3]);
}

bool unit_defines(const AsmUnit *unit, const char *label) {
    size_t length = strlen(label);
    const char *line = unit->text;
    while (line != NULL && *line != '\0') {
        if (defines_here(line, label, length)) {
            return true;
        }
        line = strchr(line, '\n');
        if (line != NULL) {
            line++;
        }
    }
    return false;
}
