#include "project.h"

#include <string.h>

#include "platform/filesystem.h"
#include "platform/process.h"

void rz_make_directories(const char *path) {
    PathKind kind = path_kind(path);
    if (kind == PATH_DIRECTORY) {
        return;
    }
    if (kind != PATH_MISSING) {
        rz_error("expected a directory at %s", path);
    }
    const char *parent = rz_parent(path);
    if (strcmp(parent, path) == 0) {
        rz_error("cannot create directory %s", path);
    }
    rz_make_directories(parent);
    if (!rz_make_directory(path) && path_kind(path) != PATH_DIRECTORY) {
        rz_error("cannot create directory %s", path);
    }
}

const char *rz_temporary_directory(const char *parent, const char *prefix) {
    static unsigned serial;
    for (int attempt = 0; attempt < 50; attempt++) {
        const char *path = rz_join(parent, rz_format(".%s-%ld-%u", prefix, process_id(), serial++));
        if (path_kind(path) == PATH_MISSING && rz_make_directory(path)) {
            return path;
        }
    }
    rz_error("cannot create staging directory under %s", parent);
}
