#include "driver/driver.h"

#include <stdio.h>

#include "common/diagnostics.h"
#include "common/files.h"
#include "common/text_buffer.h"

char *read_source(const Options *options, size_t *length) {
    if (!options->read_stdin) {
        char *text = read_file(options->source_path, length);
        if (text == NULL) {
            fatal_error("cannot read %s", options->source_path);
        }
        return text;
    }
    TextBuffer text = {0};
    char chunk[4096];
    size_t count;
    while ((count = fread(chunk, 1, sizeof chunk, stdin)) > 0) {
        text_append_bytes(&text, chunk, count);
    }
    if (ferror(stdin)) {
        fatal_error("cannot read the source from standard input");
    }
    text_append(&text, "");
    *length = text.length;
    return text.data;
}
