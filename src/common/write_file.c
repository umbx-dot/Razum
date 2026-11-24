#include "common/files.h"

#include <stdio.h>

bool write_file(const char *path, const char *data, size_t length) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    bool written = fwrite(data, 1, length, file) == length;
    return fclose(file) == 0 && written;
}
