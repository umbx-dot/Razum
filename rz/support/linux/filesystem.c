#define _XOPEN_SOURCE 700
#include "project.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "common/arena.h"
#include "platform/filesystem.h"

const char *rz_absolute(const char *path) {
    char *resolved = realpath(path, NULL);
    if (resolved == NULL) {
        rz_error("cannot resolve %s", path);
    }
    const char *result = arena_copy_text(resolved, strlen(resolved));
    free(resolved);
    return result;
}

const char *rz_sibling(const char *name) {
    char path[4096];
    ssize_t size = readlink("/proc/self/exe", path, sizeof path - 1);
    if (size < 0 || size >= (ssize_t)sizeof path - 1) {
        return NULL;
    }
    path[size] = '\0';
    return rz_join(rz_parent(path), name);
}

bool rz_make_directory(const char *path) {
    return mkdir(path, 0777) == 0;
}

bool rz_replace(const char *from, const char *to) {
    return rename(from, to) == 0;
}

bool rz_remove_directory(const char *path) {
    return rmdir(path) == 0;
}

uintptr_t rz_lock_file(const char *path) {
    int descriptor = open(path, O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (descriptor < 0) return 0;
    struct flock lock = {0};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    if (fcntl(descriptor, F_SETLK, &lock) == 0) return (uintptr_t)descriptor + 1;
    close(descriptor);
    return 0;
}

void rz_unlock_file(uintptr_t lock) {
    if (lock != 0) close((int)(lock - 1));
}

bool rz_no_links(const char *path, bool recursive) {
    struct stat entry;
    if (lstat(path, &entry) != 0 || !(S_ISREG(entry.st_mode) || S_ISDIR(entry.st_mode))) {
        return false;
    }
    if (!recursive || !S_ISDIR(entry.st_mode)) {
        return true;
    }
    DIR *folder = opendir(path);
    if (folder == NULL) {
        return false;
    }
    bool okay = true;
    for (struct dirent *item = readdir(folder); item != NULL; item = readdir(folder)) {
        if (strcmp(item->d_name, ".") != 0 && strcmp(item->d_name, "..") != 0 &&
            !rz_no_links(rz_join(path, item->d_name), true)) {
            okay = false;
            break;
        }
    }
    closedir(folder);
    return okay;
}

double rz_milliseconds(void) {
    struct timespec stamp;
    if (clock_gettime(CLOCK_MONOTONIC, &stamp) != 0) {
        return 0;
    }
    return (double)stamp.tv_sec * 1000 + (double)stamp.tv_nsec / 1000000;
}
