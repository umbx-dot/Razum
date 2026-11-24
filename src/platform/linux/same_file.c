#include "platform/filesystem.h"

#include <sys/stat.h>

bool same_file(const char *left, const char *right) {
    struct stat first;
    struct stat second;
    return stat(right, &second) == 0 && stat(left, &first) == 0 &&
           first.st_dev == second.st_dev && first.st_ino == second.st_ino;
}
