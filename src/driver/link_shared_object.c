#include "driver/driver.h"

#include <stdlib.h>

#include "platform/process.h"

int link_shared_object(const char *object_path, const char *output_path, const char *soname) {
    const char *compiler = getenv("RZ_CC");
    char path[4096];
    if (compiler == NULL || compiler[0] == '\0') {
        if (!find_program("cc", path, sizeof path)) {
            return -1;
        }
        compiler = path;
    }
    const char *const arguments[] = {compiler, "-shared", "-nostdlib", "-Wl,-Bsymbolic", "-Wl,--no-undefined",
                                     "-Xlinker", "-soname", "-Xlinker", soname, "-o", output_path, object_path, NULL};
    return run_process(arguments, NULL);
}
