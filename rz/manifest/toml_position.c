#include "project.h"

#include <string.h>

const char *rz_toml_position(const char *source, toml_datum_t value) {
    if (value.lineno <= 0 || value.colno <= 0) {
        rz_error("missing source position for manifest value");
    }
    const char *line = source;
    for (int index = 1; index < value.lineno; index++) {
        line = strchr(line, '\n');
        if (line == NULL) {
            rz_error("invalid line for manifest value");
        }
        line++;
    }
    if (line == source && strncmp(line, "\xef\xbb\xbf", 3) == 0) {
        line += 3;
    }
    if ((size_t)value.colno > strcspn(line, "\r\n")) {
        rz_error("invalid column for manifest value");
    }
    return line + value.colno - 1;
}
