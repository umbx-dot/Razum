#include "project.h"

#include <limits.h>
#include <string.h>

#include "common/arena.h"

static const char *insert_package(const RzProject *project, const char *alias, const char *url, const char *tag) {
    const char *source = project->manifest_text;
    toml_result_t parsed = toml_parse(source, (int)strlen(source));
    toml_datum_t dependencies = toml_get(parsed.toptab, "dependencies");
    const char *updated;
    if ((dependencies.flag & TOML_FLAG_INLINED) != 0) {
        const char *start = rz_toml_position(source, dependencies);
        if (*start != '{') {
            rz_error("cannot locate the inline dependencies table");
        }
        updated = rz_format("%.*s %s = { git = \"%s\", tag = \"%s\" }%s %s",
                            (int)(start + 1 - source), source, alias, url, tag,
                            dependencies.u.tab.size > 0 ? "," : "", start + 1);
    } else {
        updated = rz_format("%s\n[dependencies.%s]\ngit = \"%s\"\ntag = \"%s\"\n", source, alias, url, tag);
    }
    toml_free(parsed);
    return updated;
}

void rz_add_package(RzProject *project, const RzOptions *options) {
    const char *name = options->package;
    const char *end = name + strlen(name);
    const char *slash = strchr(name, '/');
    const char *at = strchr(name, '@');
    const char *repo_end = at != NULL ? at : end;
    if (slash == NULL || slash == name || slash >= repo_end || slash + 1 == repo_end) {
        rz_error("specify OWNER/REPO[@TAG]");
    }
    const char *owner = arena_copy_text(name, (size_t)(slash - name));
    const char *repo = arena_copy_text(slash + 1, (size_t)(repo_end - slash - 1));
    if (!rz_name(owner) || !rz_name(repo)) {
        rz_error("invalid GitHub owner or repository name");
    }
    const char *alias = strncmp(repo, "razum-", 6) == 0 ? repo + 6 : repo;
    if (!rz_name(alias) || strchr(alias, '-') != NULL || !((alias[0] >= 'a' && alias[0] <= 'z') || alias[0] == '_')) {
        rz_error("repository '%s' does not yield a valid module alias; declare an alias in razum.toml", repo);
    }
    const char *url = rz_format("https://github.com/%s/%s", owner, repo);
    RzPackage *prior = rz_find_package(project->packages, alias);
    if (prior != NULL) {
        rz_error("package '%s' already exists in razum.toml; use rz update %s", alias, alias);
    }
    const char *tag = at != NULL ? at + 1 : rz_latest_tag(project, url, NULL);
    if (!rz_tag_name(tag)) {
        rz_error("invalid exact package tag");
    }
    const char *updated = insert_package(project, alias, url, tag);
    size_t size = strlen(updated);
    if (size > INT_MAX) {
        rz_error("manifest is too large");
    }
    toml_result_t parsed = toml_parse(updated, (int)size);
    if (!parsed.ok) {
        rz_error("cannot append package to manifest: %s", parsed.errmsg);
    }
    RzPackage *package = rz_parse_package(alias, toml_get(toml_get(parsed.toptab, "dependencies"), alias), false);
    toml_free(parsed);
    list_push(&project->packages, package);
    rz_packages(project, options);
    rz_save_package_edit(project, updated);
    printf("Added %s@%s at %s\n", package->alias, package->tag, package->revision);
}
