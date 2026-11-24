#include "driver/driver.h"

#include <string.h>

#include "common/arena.h"

char *replace_extension(const char *path, const char *extension) {
    const char *name = path;
    for (const char *cursor = path; *cursor != '\0'; cursor++) {
        if (*cursor == '/' || *cursor == '\\') {
            name = cursor + 1;
        }
    }
    const char *dot = strrchr(name, '.');
    size_t stem = dot != NULL && dot != name ? (size_t)(dot - path) : strlen(path);
    size_t extension_length = strlen(extension);
    char *result = arena_alloc(stem + extension_length + 1);
    memcpy(result, path, stem);
    memcpy(result + stem, extension, extension_length + 1);
    return result;
}
