#include "project.h"

#include <string.h>

#include "common/files.h"

void rz_save_package_edit(RzProject *project, const char *manifest) {
    const char *basename = project->file;
    for (const char *cursor = basename; *cursor != '\0'; cursor++) {
        if (*cursor == '/' || *cursor == '\\') {
            basename = cursor + 1;
        }
    }
    if (strcmp(basename, "razum.lock") == 0) {
        rz_error("the manifest and lockfile need different paths");
    }
    TextBuffer lock = rz_lock_contents(project->packages);
    const char *stage = rz_temporary_directory(project->root, "package");
    const char *config = rz_join(stage, basename);
    const char *pin = rz_join(stage, "razum.lock");
    if (!write_file(config, manifest, strlen(manifest)) || !write_file(pin, lock.data, lock.length)) {
        remove(config);
        remove(pin);
        rz_remove_directory(stage);
        rz_error("cannot stage the project manifest and lockfile");
    }
    PointerList files = {0};
    rz_add_argument(&files, basename);
    rz_add_argument(&files, "razum.lock");
    rz_publish_outputs(stage, project->root, files);
    rz_remove_directory(stage);
}
