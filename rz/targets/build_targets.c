#include "project.h"

#include <stdlib.h>
#include <string.h>

#include "common/files.h"
#include "platform/filesystem.h"

static const char *unfinished_stage;
static PointerList unfinished_files;

static void remove_stage(void) {
    if (unfinished_stage == NULL) {
        return;
    }
    for (int index = 0; index < unfinished_files.count; index++) {
        remove(rz_join(unfinished_stage, LIST_AT(unfinished_files, const char, index)));
    }
    PointerList assembly = {0};
    list_files(unfinished_stage, ".asm", false, &assembly);
    for (int index = 0; index < assembly.count; index++) {
        remove(LIST_AT(assembly, const char, index));
    }
    rz_remove_directory(unfinished_stage);
}

static void compile_target(const RzProject *project, const RzTarget *target, const char *compiler,
                           const char *stage, PointerList imports) {
    PointerList arguments = {0};
    rz_add_argument(&arguments, compiler);
    rz_add_argument(&arguments, rz_join(project->root, target->source));
    rz_add_argument(&arguments, "-o");
    rz_add_argument(&arguments, rz_join(stage, rz_target_filename(target)));
    if (target->kind == RZ_DLL || target->kind == RZ_OBJECT || target->kind == RZ_SHARED) {
        const char *mode = target->kind == RZ_DLL ? "--dll" : target->kind == RZ_OBJECT ? "--object" : "--shared";
        rz_add_argument(&arguments, mode);
    }
    rz_compiler_args(project, target, &arguments);
    for (int index = 0; index < imports.count; index++) {
        rz_add_argument(&arguments, "-I");
        rz_add_argument(&arguments, LIST_AT(imports, const char, index));
    }
    rz_run(arguments, project->root);
}

static void add_output(PointerList *outputs, const char *entry) {
    for (int index = 0; index < outputs->count; index++) {
        const char *prior = LIST_AT(*outputs, const char, index);
#ifdef _WIN32
        bool equal = _stricmp(prior, entry) == 0;
#else
        bool equal = strcmp(prior, entry) == 0;
#endif
        if (equal) {
            rz_error("multiple targets write %s", entry);
        }
    }
    rz_add_argument(outputs, entry);
}

static PointerList validate_targets(const RzProject *project, PointerList order) {
    PointerList outputs = {0};
    for (int index = 0; index < order.count; index++) {
        const RzTarget *target = LIST_AT(order, RzTarget, index);
        PathKind source = path_kind(rz_join(project->root, target->source));
        if (source != (target->kind == RZ_CMAKE ? PATH_DIRECTORY : PATH_FILE)) {
            rz_error("target '%s': source not found: %s", target->name, target->source);
        }
        add_output(&outputs, rz_target_filename(target));
        if (target->kind != RZ_CMAKE) {
            PointerList flags = {0};
            rz_compiler_args(project, target, &flags);
            for (int flag = 0; flag < flags.count; flag++) {
                if (strcmp(LIST_AT(flags, const char, flag), "--asm") == 0) {
                    add_output(&outputs, rz_format("%s.asm", target->name));
                }
            }
        }
        if (target->kind == RZ_DLL || target->kind == RZ_OBJECT || target->kind == RZ_SHARED) {
            add_output(&outputs, rz_format("%s.h", target->name));
        }
        if (target->kind == RZ_DLL || (target->kind == RZ_SHARED && RZ_HOST == 1)) {
            add_output(&outputs, rz_format("%s.def", target->name));
        }
    }
    return outputs;
}

static bool up_to_date(const char *output, const char *stamp, const char *fingerprint, PointerList paths) {
    size_t size = 0;
    const char *prior = read_file(stamp, &size);
    if (prior == NULL || size != strlen(fingerprint) || memcmp(prior, fingerprint, size) != 0) {
        return false;
    }
    for (int index = 0; index < paths.count; index++) {
        if (path_kind(rz_join(output, LIST_AT(paths, const char, index))) != PATH_FILE) {
            return false;
        }
    }
    return true;
}

void rz_build(const RzProject *project, const RzOptions *options, PointerList imports) {
    double started = rz_milliseconds();
    PointerList order = rz_build_order(project, options->target);
    PointerList files = validate_targets(project, order);
    const char *compiler = rz_tool("rzc", "RZC");
    bool native = false;
    const char *linker = NULL;
    for (int index = 0; index < order.count; index++) {
        RzKind kind = LIST_AT(order, RzTarget, index)->kind;
        native |= kind == RZ_CMAKE;
        if (RZ_HOST == 2 && kind == RZ_SHARED && linker == NULL) {
            linker = rz_tool("cc", "RZ_CC");
        }
    }
    const char *output = rz_join(project->root, rz_format("build/%s/%s", rz_platform(), options->profile));
    rz_make_directories(output);
    uintptr_t lock = rz_lock_file(rz_join(output, ".build-lock"));
    if (lock == 0) {
        rz_error("cannot lock %s; another process may be building this profile", output);
    }
    const char *scope = options->target == NULL ? ".all" : rz_format(".target-%s", options->target);
    const char *stamp = rz_join(output, scope);
    const char *fingerprint = native ? NULL : rz_build_fingerprint(project, options, compiler, linker, imports);
    if (fingerprint != NULL && up_to_date(output, stamp, fingerprint, files)) {
        printf("Up to date (%s, %.1f ms)\n", options->profile, rz_milliseconds() - started);
        rz_unlock_file(lock);
        return;
    }
    const char *stage = rz_temporary_directory(output, "build");
    unfinished_stage = stage;
    unfinished_files = files;
    atexit(remove_stage);
    for (int index = 0; index < order.count; index++) {
        const RzTarget *target = LIST_AT(order, RzTarget, index);
        double before = rz_milliseconds();
        printf("%s %s\n", target->kind == RZ_CMAKE ? "Building with CMake" : "Compiling", target->name);
        fflush(stdout);
        if (target->kind == RZ_CMAKE) {
            rz_build_cmake(project, target, options, stage);
        } else {
            compile_target(project, target, compiler, stage, imports);
        }
        printf("  %.1f ms\n", rz_milliseconds() - before);
    }
    rz_publish_outputs(stage, output, files);
    if (fingerprint != NULL) {
        rz_save_file(stamp, fingerprint, strlen(fingerprint));
    }
    printf("Built %s in %.1f ms\n", output, rz_milliseconds() - started);
    rz_unlock_file(lock);
}
