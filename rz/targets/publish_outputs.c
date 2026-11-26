#include "project.h"

#include "common/arena.h"
#include "platform/filesystem.h"

typedef struct {
    const char *from;
    const char *to;
    const char *backup;
    bool saved;
    bool published;
} Output;

void rz_publish_outputs(const char *stage, const char *output, PointerList files) {
    Output *entries = arena_alloc((size_t)files.count * sizeof *entries);
    for (int index = 0; index < files.count; index++) {
        const char *entry = LIST_AT(files, const char, index);
        Output *file = &entries[index];
        file->from = rz_join(stage, entry);
        file->to = rz_join(output, entry);
        file->backup = rz_join(stage, rz_format(".previous-%d", index));
        if (path_kind(file->from) != PATH_FILE || path_kind(file->to) == PATH_DIRECTORY) {
            rz_error("cannot publish %s", file->to);
        }
    }
    for (int index = 0; index < files.count; index++) {
        Output *file = &entries[index];
        bool okay = true;
        if (path_kind(file->to) == PATH_FILE) {
            okay = file->saved = rz_replace(file->to, file->backup);
        }
        if (okay && rz_replace(file->from, file->to)) {
            file->published = true;
            continue;
        }
        bool restored = true;
        for (int rollback = index; rollback >= 0; rollback--) {
            Output *undo = &entries[rollback];
            if (undo->saved) {
                restored &= rz_replace(undo->backup, undo->to);
            } else if (undo->published) {
                restored &= remove(undo->to) == 0;
            }
        }
        if (!restored) {
            rz_error("cannot publish %s; previous files are in %s", file->to, stage);
        }
        rz_error("cannot publish %s; previous files were restored", file->to);
    }
    for (int index = 0; index < files.count; index++) {
        if (entries[index].saved) {
            remove(entries[index].backup);
        }
    }
}
