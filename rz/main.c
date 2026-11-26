#include "project.h"

#include <stdio.h>
#include <string.h>

#include "platform/filesystem.h"

int main(int argc, char **argv) {
    RzOptions options = rz_options(argc, argv);
    const char *manifest = rz_manifest_path(options.manifest);
    RzProject project = rz_load_project(manifest, options.profile);
    bool add = strcmp(options.command, "add") == 0;
    bool update = strcmp(options.command, "update") == 0;
    uintptr_t lock = 0;
    if (add || update || project.packages.count > 0 || path_kind(rz_join(project.root, "razum.lock")) != PATH_MISSING) {
        const char *state = rz_join(project.root, "build");
        rz_make_directories(state);
        lock = rz_lock_file(rz_join(state, ".package-lock"));
        if (lock == 0) {
            rz_error("another command may be using package metadata under %s", state);
        }
        project = rz_load_project(manifest, options.profile);
    }
    if (add || update) {
        if (add) {
            rz_add_package(&project, &options);
        } else {
            rz_update_package(&project, &options);
        }
        rz_unlock_file(lock);
    } else if (strcmp(options.command, "compiler-args") == 0) {
        RzTarget *target = rz_find_target(&project, options.target);
        if (target == NULL || target->kind == RZ_CMAKE || (target->platforms & RZ_HOST) == 0) {
            rz_error("unavailable Razum target '%s'", options.target);
        }
        PointerList arguments = {0};
        rz_compiler_args(&project, target, &arguments);
        PointerList imports = rz_packages(&project, &options);
        rz_unlock_file(lock);
        for (int index = 0; index < arguments.count; index++) {
            puts(LIST_AT(arguments, const char, index));
        }
        for (int index = 0; index < imports.count; index++) {
            puts("-I");
            puts(LIST_AT(imports, const char, index));
        }
    } else {
        double started = rz_milliseconds();
        bool building = strcmp(options.command, "build") == 0;
        if (building) {
            rz_build_order(&project, options.target);
        }
        PointerList imports = rz_packages(&project, &options);
        rz_unlock_file(lock);
        printf("Dependencies: %.1f ms\n", rz_milliseconds() - started);
        if (building) {
            rz_build(&project, &options, imports);
        }
    }
    return 0;
}
