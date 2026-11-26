#include "project.h"

#include <limits.h>
#include <string.h>

#include "common/arena.h"

static const char *tag_end(const char *start) {
    char quote = *start;
    if (quote != '"' && quote != '\'') {
        rz_error("cannot locate the package tag string");
    }
    size_t width = start[1] == quote && start[2] == quote ? 3 : 1;
    for (const char *at = start + width; *at != '\0'; at++) {
        if (*at == quote && (width == 1 || (at[1] == quote && at[2] == quote))) {
            return at + width;
        }
        if (*at == '\\' && quote == '"' && at[1] != '\0') {
            at++;
        }
    }
    rz_error("cannot locate the end of the package tag string");
}

static const char *replace_tag(const RzProject *project, const char *alias, const char *tag) {
    const char *source = project->manifest_text;
    toml_result_t parsed = toml_parse(source, (int)strlen(source));
    toml_datum_t package = toml_get(toml_get(parsed.toptab, "dependencies"), alias);
    const char *start = rz_toml_position(source, toml_get(package, "tag"));
    const char *updated = rz_format("%.*s\"%s\"%s", (int)(start - source), source, tag, tag_end(start));
    toml_free(parsed);
    size_t size = strlen(updated);
    if (size > INT_MAX) {
        rz_error("manifest is too large");
    }
    parsed = toml_parse(updated, (int)size);
    if (!parsed.ok) {
        rz_error("cannot update package in manifest: %s", parsed.errmsg);
    }
    package = toml_get(toml_get(parsed.toptab, "dependencies"), alias);
    if (strcmp(rz_toml_string(toml_get(package, "tag"), "tag"), tag) != 0) {
        rz_error("cannot locate the manifest tag for '%s'", alias);
    }
    toml_free(parsed);
    return updated;
}

void rz_update_package(RzProject *project, const RzOptions *options) {
    const char *request = options->package;
    const char *at = strchr(request, '@');
    const char *alias = at != NULL ? arena_copy_text(request, (size_t)(at - request)) : request;
    RzPackage *package = rz_find_package(project->packages, alias);
    if (package == NULL) {
        rz_error("'%s' is not a direct dependency in razum.toml", alias);
    }
    const char *prior = package->tag;
    const char *tag = at != NULL ? at + 1 : rz_latest_tag(project, package->url, prior);
    if (!rz_tag_name(tag)) {
        rz_error("invalid exact package tag");
    }
    bool changed = strcmp(prior, tag) != 0;
    const char *updated = changed ? replace_tag(project, alias, tag) : NULL;
    package->tag = tag;
    rz_packages(project, options);
    if (changed) {
        rz_save_package_edit(project, updated);
        printf("Updated %s %s -> %s at %s\n", alias, prior, tag, package->revision);
    } else {
        rz_save_lock(rz_join(project->root, "razum.lock"), project->packages);
        printf("%s is pinned to %s at %s\n", alias, tag, package->revision);
    }
}
