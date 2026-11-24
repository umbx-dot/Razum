#include "platform/process.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool find_program(const char *name, char *path, size_t size) {
    const char *search = getenv("PATH");
    if (search == NULL) {
        return false;
    }
    while (*search != '\0') {
        size_t length = strcspn(search, ":");
        int written = snprintf(path, size, "%.*s/%s", (int)length, length == 0 ? "." : search, name);
        if (written > 0 && (size_t)written < size && access(path, X_OK) == 0) {
            return true;
        }
        search += length + (search[length] == ':');
    }
    return false;
}
