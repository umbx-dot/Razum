#include "common/files.h"

#include <stdio.h>

#include "common/arena.h"

char *read_file(const char *path, size_t *length) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    char *data = NULL;
    if (fseek(file, 0, SEEK_END) == 0) {
        long size = ftell(file);
        if (size >= 0 && fseek(file, 0, SEEK_SET) == 0) {
            data = arena_alloc((size_t)size + 1);
            *length = fread(data, 1, (size_t)size, file);
            data[*length] = '\0';
        }
    }
    fclose(file);
    return data;
}
