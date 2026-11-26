#include "project.h"

#include <string.h>

#include "common/arena.h"

static const char *string_field(toml_datum_t table, const char *name, bool required) {
    toml_datum_t value = rz_toml_field(table, name, TOML_STRING, required);
    return value.type == TOML_UNKNOWN ? NULL : rz_toml_string(value, name);
}

static RzKind read_kind(const char *kind) {
    if (strcmp(kind, "exe") == 0) return RZ_EXE;
    if (strcmp(kind, "dll") == 0) return RZ_DLL;
    if (strcmp(kind, "object") == 0) return RZ_OBJECT;
    if (strcmp(kind, "shared") == 0) return RZ_SHARED;
    if (strcmp(kind, "cmake") == 0) return RZ_CMAKE;
    rz_error("unknown target kind '%s'", kind);
}

static int read_platforms(toml_datum_t table, RzKind kind) {
    toml_datum_t array = rz_toml_field(table, "platforms", TOML_ARRAY, false);
    int allowed = kind == RZ_DLL ? 1 : 3;
    if (array.type == TOML_UNKNOWN) {
        return allowed;
    }
    int mask = 0;
    for (int index = 0; index < array.u.arr.size; index++) {
        const char *name = rz_toml_string(array.u.arr.elem[index], "platforms");
        int platform = strcmp(name, "windows") == 0 ? 1 : strcmp(name, "linux") == 0 ? 2 : 0;
        if ((platform & allowed) == 0 || (mask & platform) != 0) {
            rz_error("line %d: invalid or duplicate platform '%s'", array.lineno, name);
        }
        mask |= platform;
    }
    if (mask == 0) {
        rz_error("line %d: platforms cannot be empty", array.lineno);
    }
    return mask;
}

RzTarget *rz_load_target(toml_datum_t table) {
    static const char *const keys[] = {
        "name", "kind", "source", "platforms", "subsystem", "depends_on", "import_dirs", "optimize",
        "keep_asm", "cpu", "cmake_target", "artifact", "preset", NULL
    };
    if (table.type != TOML_TABLE) {
        rz_error("line %d: each target must be a table", table.lineno);
    }
    rz_toml_keys(table, keys);
    RzTarget *target = arena_alloc(sizeof *target);
    target->name = string_field(table, "name", true);
    target->kind = read_kind(string_field(table, "kind", true));
    target->source = string_field(table, "source", true);
    target->platforms = read_platforms(table, target->kind);
    target->flags = rz_read_flags(table, (RzFlags){-1, -1, 0});
    if (!rz_name(target->name) || !rz_relative(target->source)) {
        rz_error("line %d: invalid target name or source", table.lineno);
    }
    rz_toml_strings(rz_toml_field(table, "depends_on", TOML_ARRAY, false), &target->needs, "depends_on", false);
    toml_datum_t imports = rz_toml_field(table, "import_dirs", TOML_ARRAY, false);
    rz_toml_strings(imports, &target->imports, "import_dirs", true);
    const char *subsystem = string_field(table, "subsystem", false);
    if (subsystem != NULL) {
        target->subsystem = strcmp(subsystem, "gui") == 0 ? 1 : strcmp(subsystem, "console") == 0 ? -1 : 0;
        if (target->kind != RZ_EXE || target->subsystem == 0 || (target->platforms & 1) == 0) {
            rz_error("target '%s': subsystem requires a Windows executable", target->name);
        }
    }
    target->cmake_target = string_field(table, "cmake_target", false);
    target->artifact = string_field(table, "artifact", false);
    target->preset = string_field(table, "preset", false);
    if (target->kind == RZ_CMAKE) {
        if (target->artifact == NULL) {
            target->artifact = rz_format("%s%s", target->cmake_target != NULL ? target->cmake_target : target->name,
                                         rz_exe_suffix());
        }
        if (!rz_relative(target->artifact) || (target->preset != NULL && !rz_name(target->preset)) ||
            (target->cmake_target != NULL && !rz_name(target->cmake_target)) || imports.type != TOML_UNKNOWN ||
            target->flags.optimize >= 0 || target->flags.assembly >= 0 || target->flags.cpu != 0) {
            rz_error("target '%s': invalid CMake artifact, preset, target name or Razum-only setting", target->name);
        }
    } else if (target->artifact != NULL || target->preset != NULL || target->cmake_target != NULL) {
        rz_error("target '%s': artifact, preset and cmake_target need kind = \"cmake\"", target->name);
    }
    return target;
}
