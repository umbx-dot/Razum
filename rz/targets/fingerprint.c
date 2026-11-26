#include "project.h"

#include <stdint.h>
#include <string.h>

static void hash_bytes(uint64_t *hash, const void *data, size_t size) {
    const unsigned char *bytes = data;
    for (size_t index = 0; index < size; index++) {
        *hash = (*hash ^ bytes[index]) * UINT64_C(1099511628211);
    }
}

static bool hash_file(uint64_t *hash, const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    hash_bytes(hash, path, strlen(path) + 1);
    unsigned char bytes[16384];
    size_t count = 0;
    while ((count = fread(bytes, 1, sizeof bytes, file)) != 0) {
        hash_bytes(hash, bytes, count);
    }
    bool ok = ferror(file) == 0;
    fclose(file);
    hash_bytes(hash, "", 1);
    return ok;
}

static bool hash_sources(uint64_t *hash, const char *folder, const char *excluded) {
    static const char *const extensions[] = {".rz", ".asm", NULL};
    for (int kind = 0; extensions[kind] != NULL; kind++) {
        PointerList files = {0};
        if (!rz_list_inputs(folder, excluded, extensions[kind], &files)) {
            return false;
        }
        for (int index = 0; index < files.count; index++) {
            if (!hash_file(hash, LIST_AT(files, const char, index))) {
                return false;
            }
        }
    }
    return true;
}

const char *rz_build_fingerprint(const RzProject *project, const RzOptions *options, const char *compiler,
                                 const char *linker, PointerList imports) {
    uint64_t hash = UINT64_C(14695981039346656037);
    hash_bytes(&hash, "rz build v1", sizeof "rz build v1");
    hash_bytes(&hash, options->profile, strlen(options->profile) + 1);
    hash_bytes(&hash, project->file, strlen(project->file) + 1);
    hash_bytes(&hash, project->manifest_text, strlen(project->manifest_text) + 1);
    for (int index = 0; index < project->packages.count; index++) {
        const RzPackage *package = LIST_AT(project->packages, RzPackage, index);
        hash_bytes(&hash, package->alias, strlen(package->alias) + 1);
        hash_bytes(&hash, package->revision, strlen(package->revision) + 1);
    }
    if (!hash_file(&hash, compiler) || !hash_file(&hash, rz_tool("nasm", "RZ_NASM")) ||
        (linker != NULL && !hash_file(&hash, linker)) ||
        !hash_sources(&hash, project->root, rz_join(project->root, "build"))) {
        return NULL;
    }
    for (int index = 0; index < imports.count; index++) {
        if (!hash_sources(&hash, LIST_AT(imports, const char, index), NULL)) {
            return NULL;
        }
    }
    return rz_format("%016llx", (unsigned long long)hash);
}
