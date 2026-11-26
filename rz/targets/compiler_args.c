#include "project.h"

#include <string.h>

#include "platform/filesystem.h"

const char *rz_target_filename(const RzTarget *target) {
    if (target->kind == RZ_CMAKE) {
        const char *name = target->artifact;
        for (const char *at = name; *at != '\0'; at++) {
            if (*at == '/' || *at == '\\') name = at + 1;
        }
        return name;
    }
    const char *suffix = rz_exe_suffix();
    if (target->kind == RZ_DLL || target->kind == RZ_SHARED) suffix = RZ_HOST == 1 ? ".dll" : ".so";
    if (target->kind == RZ_OBJECT) suffix = RZ_HOST == 1 ? ".obj" : ".o";
    return rz_format("%s%s", target->name, suffix);
}

void rz_compiler_args(const RzProject *project, const RzTarget *target, PointerList *arguments) {
    RzFlags flags = project->flags;
    if (target->flags.optimize >= 0) flags.optimize = target->flags.optimize;
    if (target->flags.assembly >= 0) flags.assembly = target->flags.assembly;
    if (target->flags.cpu != 0) flags.cpu = target->flags.cpu;
    if (flags.cpu == 2 && !flags.optimize) {
        rz_error("target '%s': znver3 requires optimize = true", target->name);
    }
    if (flags.cpu == 2) {
        rz_add_argument(arguments, "-march=znver3");
    } else if (flags.optimize) {
        rz_add_argument(arguments, "-O");
    }
    if (flags.assembly) {
        rz_add_argument(arguments, "--asm");
    }
    if (target->subsystem != 0 && RZ_HOST == 1) {
        rz_add_argument(arguments, target->subsystem > 0 ? "--gui" : "--console");
    }
    for (int index = 0; index < target->imports.count; index++) {
        const char *path = rz_join(project->root, LIST_AT(target->imports, const char, index));
        if (path_kind(path) != PATH_DIRECTORY) {
            rz_error("target '%s': import directory does not exist: %s", target->name, path);
        }
        rz_add_argument(arguments, "-I");
        rz_add_argument(arguments, path);
    }
}
