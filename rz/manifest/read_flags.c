#include "project.h"

#include <string.h>

RzFlags rz_read_flags(toml_datum_t table, RzFlags flags) {
    toml_datum_t optimize = rz_toml_field(table, "optimize", TOML_BOOLEAN, false);
    toml_datum_t assembly = rz_toml_field(table, "keep_asm", TOML_BOOLEAN, false);
    toml_datum_t cpu = rz_toml_field(table, "cpu", TOML_STRING, false);
    if (optimize.type != TOML_UNKNOWN) {
        flags.optimize = optimize.u.boolean;
    }
    if (assembly.type != TOML_UNKNOWN) {
        flags.assembly = assembly.u.boolean;
    }
    if (cpu.type != TOML_UNKNOWN) {
        const char *value = rz_toml_string(cpu, "cpu");
        if (strcmp(value, "portable") == 0) {
            flags.cpu = 1;
        } else if (strcmp(value, "znver3") == 0) {
            flags.cpu = 2;
            if (optimize.type == TOML_UNKNOWN) {
                flags.optimize = 1;
            }
        } else {
            rz_error("line %d: unknown CPU '%s'", cpu.lineno, value);
        }
    }
    return flags;
}
