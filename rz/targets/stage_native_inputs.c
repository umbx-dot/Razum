#include "project.h"

#include <string.h>

#include "common/files.h"
#include "platform/filesystem.h"

static void copy_if_changed(const char *stage, const char *destination, const char *name) {
    const char *source = rz_join(stage, name);
    const char *output = rz_join(destination, name);
    size_t size = 0, previous_size = 0;
    const char *contents = read_file(source, &size);
    if (contents == NULL) {
        rz_error("missing native build input %s", source);
    }
    const char *prior = read_file(output, &previous_size);
    if (prior == NULL || size != previous_size || memcmp(contents, prior, size) != 0) {
        rz_save_file(output, contents, size);
    }
}

static void collect_inputs(const RzProject *project, const RzTarget *target, PointerList *files) {
    for (int index = 0; index < target->needs.count; index++) {
        const RzTarget *dependency = rz_find_target(project, LIST_AT(target->needs, const char, index));
        bool seen = false;
        for (int added = 0; added < files->count; added++) {
            if (LIST_AT(*files, RzTarget, added) == dependency) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            list_push(files, (void *)dependency);
            collect_inputs(project, dependency, files);
        }
    }
}

void rz_stage_native_inputs(const RzProject *project, const RzTarget *target, const char *stage, const char *dir) {
    PointerList dependencies = {0};
    collect_inputs(project, target, &dependencies);
    rz_make_directories(dir);
    for (int index = 0; index < dependencies.count; index++) {
        const RzTarget *dependency = LIST_AT(dependencies, RzTarget, index);
        const char *filename = rz_target_filename(dependency);
        copy_if_changed(stage, dir, filename);
        if ((dependency->kind == RZ_EXE || dependency->kind == RZ_CMAKE) && !make_executable(rz_join(dir, filename))) {
            rz_error("cannot mark native input %s as executable", filename);
        }
        if (dependency->kind == RZ_DLL || dependency->kind == RZ_OBJECT || dependency->kind == RZ_SHARED) {
            copy_if_changed(stage, dir, rz_format("%s.h", dependency->name));
        }
        if (dependency->kind == RZ_DLL || (dependency->kind == RZ_SHARED && RZ_HOST == 1)) {
            copy_if_changed(stage, dir, rz_format("%s.def", dependency->name));
        }
    }
}
