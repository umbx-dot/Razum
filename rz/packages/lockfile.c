#include "project.h"

#include <string.h>
#include "common/files.h"
#include "platform/filesystem.h"

void rz_save_file(const char *path, const char *contents, size_t size) {
    const char *stage = rz_temporary_directory(rz_parent(path), "write");
    const char *temp = rz_join(stage, "new");
    if (!write_file(temp, contents, size) || !rz_replace(temp, path)) {
        remove(temp);
        rz_remove_directory(stage);
        rz_error("cannot write %s", path);
    }
    rz_remove_directory(stage);
}

PointerList rz_load_lock(const char *file) {
    PointerList pins = {0};
    if (path_kind(file) == PATH_MISSING) {
        return pins;
    }
    static const char *const keys[] = {"lock_version", "package", NULL};
    toml_result_t parsed = rz_read_toml(file, NULL);
    rz_toml_keys(parsed.toptab, keys);
    if (rz_toml_field(parsed.toptab, "lock_version", TOML_INT64, true).u.int64 != 1) {
        rz_error("%s: unsupported lock_version", file);
    }
    toml_datum_t packages = rz_toml_field(parsed.toptab, "package", TOML_TABLE, true);
    for (int index = 0; index < packages.u.tab.size; index++) {
        list_push(&pins, rz_parse_package(packages.u.tab.key[index], packages.u.tab.value[index], true));
    }
    toml_free(parsed);
    return pins;
}

TextBuffer rz_lock_contents(PointerList packages) {
    TextBuffer text = {0};
    text_line(&text, "lock_version = 1");
    text_line(&text, "[package]");
    for (int index = 0; index < packages.count; index++) {
        const RzPackage *item = LIST_AT(packages, RzPackage, index);
        text_line(&text, "");
        text_line(&text, "[package.%s]", item->alias);
        text_line(&text, "git = \"%s\"", item->url);
        text_line(&text, "tag = \"%s\"", item->tag);
        text_line(&text, "commit = \"%s\"", item->revision);
    }
    return text;
}

void rz_save_lock(const char *file, PointerList packages) {
    TextBuffer text = rz_lock_contents(packages);
    size_t prior_size = 0;
    char *prior = read_file(file, &prior_size);
    if (prior == NULL || prior_size != text.length || memcmp(prior, text.data, text.length) != 0) {
        rz_save_file(file, text.data, text.length);
    }
}
