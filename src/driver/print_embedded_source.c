#include "driver/driver.h"

#include <stdio.h>
#include <string.h>

#include "embedded/embedded_files.h"

bool print_embedded_source(const char *path) {
    static const char PREFIX[] = "<rzc>/libraries/";
    const char *relative = strncmp(path, PREFIX, sizeof PREFIX - 1) == 0 ? path + sizeof PREFIX - 1 : path;
    for (int index = 0; index < EMBEDDED_FILE_COUNT; index++) {
        if (strcmp(EMBEDDED_FILES[index].path, relative) == 0) {
            fputs(EMBEDDED_FILES[index].text, stdout);
            return true;
        }
    }
    fprintf(stderr, "rzc: no built-in library file %s\n", path);
    return false;
}
