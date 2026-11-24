#pragma once

typedef struct {
    const char *path;
    const char *text;
} EmbeddedFile;

extern const EmbeddedFile EMBEDDED_FILES[];
extern const int EMBEDDED_FILE_COUNT;
