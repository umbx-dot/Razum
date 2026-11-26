#include "project.h"

#include <string.h>
#include "platform/filesystem.h"

RzPackage *rz_find_package(PointerList packages, const char *name) {
    for (int index = 0; index < packages.count; index++) {
        RzPackage *package = LIST_AT(packages, RzPackage, index);
        if (strcmp(package->alias, name) == 0) {
            return package;
        }
    }
    return NULL;
}

static bool same_package(const RzPackage *first, const RzPackage *second) {
    return strcmp(first->url, second->url) == 0 && strcmp(first->tag, second->tag) == 0;
}

static void package_dependencies(RzProject *project, const char *checkout) {
    const char *file = rz_join(checkout, "razum.toml");
    if (path_kind(file) != PATH_FILE) {
        return;
    }
    toml_result_t parsed = rz_read_toml(file, NULL);
    if (rz_toml_field(parsed.toptab, "manifest_version", TOML_INT64, true).u.int64 != 1) {
        rz_error("%s: unsupported package manifest_version", file);
    }
    toml_datum_t dependencies = rz_toml_field(parsed.toptab, "dependencies", TOML_TABLE, false);
    for (int index = 0; index < dependencies.u.tab.size; index++) {
        RzPackage *item = rz_parse_package(dependencies.u.tab.key[index], dependencies.u.tab.value[index], false);
        RzPackage *already = rz_find_package(project->packages, item->alias);
        if (already == NULL) {
            list_push(&project->packages, item);
        } else if (!same_package(already, item)) {
            rz_error("conflicting versions or sources for package '%s'", item->alias);
        }
    }
    toml_free(parsed);
}

PointerList rz_packages(RzProject *project, const RzOptions *options) {
    PointerList paths = {0};
    const char *lockfile = rz_join(project->root, "razum.lock");
    PointerList pins = rz_load_lock(lockfile);
    for (int index = 0; index < project->packages.count; index++) {
        if (index >= 256) {
            rz_error("too many source packages");
        }
        RzPackage *package = LIST_AT(project->packages, RzPackage, index);
        RzPackage *pin = rz_find_package(pins, package->alias);
        if (pin != NULL && same_package(pin, package)) {
            package->revision = pin->revision;
        } else if (options->locked) {
            rz_error("package '%s' has no matching pin in razum.lock; run rz fetch", package->alias);
        }
        const char *checkout = rz_cached_package(package, options->offline);
        if (path_kind(rz_join(checkout, package->alias)) != PATH_DIRECTORY &&
            path_kind(rz_join(checkout, rz_format("%s.rz", package->alias))) != PATH_FILE) {
            rz_error("package '%s' must provide %s.rz or a %s/ directory at its root",
                     package->alias, package->alias, package->alias);
        }
        rz_add_argument(&paths, checkout);
        package_dependencies(project, checkout);
    }
    if (options->locked && pins.count != project->packages.count) {
        rz_error("razum.lock has unused entries; run rz fetch");
    }
    if (!options->locked && strcmp(options->command, "add") != 0 && strcmp(options->command, "update") != 0 &&
        (project->packages.count != 0 || pins.count != 0)) {
        rz_save_lock(lockfile, project->packages);
    }
    return paths;
}
