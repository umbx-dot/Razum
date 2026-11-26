#include "project.h"

#include <stdlib.h>
#include <string.h>

#include "common/arena.h"
#include "platform/filesystem.h"
#include "platform/process.h"

const char *rz_tool(const char *name, const char *override) {
    const char *configured = override != NULL ? getenv(override) : NULL;
    if (configured != NULL && *configured != '\0') {
        const char *path = rz_absolute(configured);
        if (path_kind(path) != PATH_FILE) {
            rz_error("%s does not point to a file: %s", override, path);
        }
        return path;
    }
    if (strcmp(name, "rzc") == 0) {
        const char *local = rz_sibling(rz_format("rzc%s", rz_exe_suffix()));
        if (local != NULL && path_kind(local) == PATH_FILE) {
            return local;
        }
    }
    char path[4096];
    if (!find_program(name, path, sizeof path)) {
        rz_error("%s was not found on PATH", name);
    }
    return rz_absolute(path);
}

void rz_add_argument(PointerList *arguments, const char *value) {
    list_push(arguments, (void *)value);
}

void rz_run(PointerList arguments, const char *directory) {
    const char **vector = arena_alloc(((size_t)arguments.count + 1) * sizeof *vector);
    for (int index = 0; index < arguments.count; index++) {
        vector[index] = LIST_AT(arguments, const char, index);
    }
    vector[arguments.count] = NULL;
    int result = run_process(vector, directory);
    if (result != 0) {
        rz_error("%s exited with status %d", vector[0], result);
    }
}
