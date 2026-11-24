#include "platform/filesystem.h"

#include <sys/stat.h>

PathKind path_kind(const char *path) {
    struct stat information;
    if (stat(path, &information) != 0) {
        return PATH_MISSING;
    }
    return S_ISDIR(information.st_mode) ? PATH_DIRECTORY : PATH_FILE;
}
