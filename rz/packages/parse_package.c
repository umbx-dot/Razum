#include "project.h"

#include <string.h>

#include "common/arena.h"

bool rz_tag_name(const char *tag) {
    if (*tag == '\0' || *tag == '-' || *tag == '.' || strlen(tag) > 128 || strstr(tag, "..") != NULL) {
        return false;
    }
    for (const char *ch = tag; *ch != '\0'; ch++) {
        if (!(*ch >= 'a' && *ch <= 'z') && !(*ch >= 'A' && *ch <= 'Z') &&
            !(*ch >= '0' && *ch <= '9') && strchr("._-", *ch) == NULL) {
            return false;
        }
    }
    size_t length = strlen(tag);
    return tag[length - 1] != '.' && (length < 5 || strcmp(tag + length - 5, ".lock") != 0);
}

bool rz_revision(const char *revision) {
    size_t length = strlen(revision);
    return (length == 40 || length == 64) && strspn(revision, "0123456789abcdef") == length;
}

RzPackage *rz_parse_package(const char *name, toml_datum_t table, bool locked) {
    static const char *const request_keys[] = {"git", "tag", NULL};
    static const char *const lock_keys[] = {"git", "tag", "commit", NULL};
    if (!rz_name(name) || table.type != TOML_TABLE || strchr(name, '-') != NULL ||
        !((name[0] >= 'a' && name[0] <= 'z') || name[0] == '_')) {
        rz_error("package '%s': expected a module alias and a table", name);
    }
    rz_toml_keys(table, locked ? lock_keys : request_keys);
    RzPackage *package = arena_alloc(sizeof *package);
    package->alias = arena_copy_text(name, strlen(name));
    package->url = rz_toml_string(rz_toml_field(table, "git", TOML_STRING, true), "git");
    package->tag = rz_toml_string(rz_toml_field(table, "tag", TOML_STRING, true), "tag");
    const char *url = package->url;
    size_t protocol = strncmp(url, "https://", 8) == 0 ? 8u : strncmp(url, "file:///", 8) == 0 ? 7u : 0u;
    if (protocol == 0 || strlen(url) <= protocol || !rz_tag_name(package->tag) ||
        strpbrk(url, "\r\n\t\"\\") != NULL) {
        rz_error("package '%s': use an HTTPS or absolute file URL and an exact tag", name);
    }
    if (locked) {
        package->revision = rz_toml_string(rz_toml_field(table, "commit", TOML_STRING, true), "commit");
        if (!rz_revision(package->revision)) {
            rz_error("package '%s': invalid locked commit", name);
        }
    }
    return package;
}
