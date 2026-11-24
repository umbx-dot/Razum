#include "platform/filesystem.h"

#include <sys/stat.h>

bool make_executable(const char *path) {
    struct stat information;
    return stat(path, &information) == 0 && chmod(path, information.st_mode | S_IXUSR | S_IXGRP | S_IXOTH) == 0;
}
