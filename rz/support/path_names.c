#include "project.h"

#include <string.h>

#include "common/arena.h"
#include "platform/filesystem.h"

bool rz_name(const char *name) {
    size_t length = strlen(name);
    if (length == 0 || length > 64 || name[0] == '-') {
        return false;
    }
    if (strcmp(name, "con") == 0 || strcmp(name, "prn") == 0 || strcmp(name, "aux") == 0 ||
        strcmp(name, "nul") == 0 || (length == 4 && (strncmp(name, "com", 3) == 0 ||
        strncmp(name, "lpt", 3) == 0) && name[3] >= '1' && name[3] <= '9')) {
        return false;
    }
    for (const char *ch = name; *ch != '\0'; ch++) {
        if (!(*ch >= 'a' && *ch <= 'z') && !(*ch >= '0' && *ch <= '9') && *ch != '-' && *ch != '_') {
            return false;
        }
    }
    return true;
}

bool rz_relative(const char *path) {
    if (*path == '\0' || *path == '/' || *path == '\\' || strlen(path) > 2048) {
        return false;
    }
    const char *part = path;
    for (const char *ch = path;; ch++) {
        if (*ch == '\0' || *ch == '/' || *ch == '\\') {
            if (ch == part || (ch[-1] == '.' && !(ch - part == 1 && part[0] == '.')) || ch[-1] == ' ') {
                return false;
            }
            if (ch - part == 2 && part[0] == '.' && part[1] == '.') {
                return false;
            }
            if (*ch == '\0') {
                return true;
            }
            part = ch + 1;
        } else if ((unsigned char)*ch < 32 || strchr(":;<>|\"?*", *ch) != NULL) {
            return false;
        }
    }
}

const char *rz_join(const char *directory, const char *name) {
    return rz_format("%s/%s", directory, name);
}

const char *rz_parent(const char *path) {
    const char *end = path + strlen(path);
    while (end > path + 1 && end[-1] != '/' && end[-1] != '\\') {
        end--;
    }
    if (end == path + 1 && path[0] != '/') {
        return ".";
    }
    return arena_copy_text(path, (size_t)(end - path > 1 && end[-2] != ':' ? end - path - 1 : end - path));
}

const char *rz_manifest_path(const char *given) {
    if (given != NULL) {
        const char *path = rz_absolute(given);
        if (path_kind(path) != PATH_FILE) {
            rz_error("manifest not found: %s", path);
        }
        return path;
    }
    const char *folder = rz_absolute(".");
    for (;;) {
        const char *manifest = rz_join(folder, "razum.toml");
        if (path_kind(manifest) == PATH_FILE) {
            return manifest;
        }
        const char *parent = rz_parent(folder);
        if (strcmp(folder, parent) == 0 || strcmp(parent, ".") == 0) {
            rz_error("no razum.toml found; pass --manifest FILE");
        }
        folder = parent;
    }
}

const char *rz_platform(void) {
    return RZ_HOST == 1 ? "windows-x64" : "linux-x64";
}

const char *rz_exe_suffix(void) {
    return RZ_HOST == 1 ? ".exe" : "";
}
