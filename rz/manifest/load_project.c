#include "project.h"

#include <string.h>

static RzFlags select_profile(toml_datum_t table, const char *profile) {
    static const char *const flag_keys[] = {"optimize", "keep_asm", "cpu", NULL};
    if (table.type != TOML_UNKNOWN) {
        for (int index = 0; index < table.u.tab.size; index++) {
            if (!rz_name(table.u.tab.key[index]) || table.u.tab.value[index].type != TOML_TABLE) {
                rz_error("invalid profile '%s'", table.u.tab.key[index]);
            }
            rz_toml_keys(table.u.tab.value[index], flag_keys);
            rz_read_flags(table.u.tab.value[index], (RzFlags){0});
        }
    }
    toml_datum_t selected = rz_toml_field(table, profile, TOML_TABLE, false);
    bool release = strcmp(profile, "release") == 0;
    if (selected.type == TOML_UNKNOWN && !release && strcmp(profile, "dev") != 0) {
        rz_error("unknown profile '%s'", profile);
    }
    return rz_read_flags(selected, (RzFlags){release, 0, 1});
}

RzProject rz_load_project(const char *path, const char *profile) {
    static const char *const root_keys[] = {"manifest_version", "project", "profile", "target", "dependencies", NULL};
    static const char *const project_keys[] = {"name", "version", NULL};
    RzProject project = {0};
    project.file = path;
    project.root = rz_parent(path);
    toml_result_t parsed = rz_read_toml(path, &project.manifest_text);
    toml_datum_t top = parsed.toptab;
    rz_toml_keys(top, root_keys);
    if (rz_toml_field(top, "manifest_version", TOML_INT64, true).u.int64 != 1) {
        rz_error("%s: unsupported manifest_version", path);
    }
    toml_datum_t metadata = rz_toml_field(top, "project", TOML_TABLE, true);
    rz_toml_keys(metadata, project_keys);
    project.name = rz_toml_string(rz_toml_field(metadata, "name", TOML_STRING, true), "project.name");
    if (!rz_name(project.name)) {
        rz_error("invalid project name '%s'", project.name);
    }
    toml_datum_t version = rz_toml_field(metadata, "version", TOML_STRING, false);
    if (version.type != TOML_UNKNOWN) {
        rz_toml_string(version, "project.version");
    }
    project.flags = select_profile(rz_toml_field(top, "profile", TOML_TABLE, false), profile);
    toml_datum_t targets = rz_toml_field(top, "target", TOML_ARRAY, false);
    for (int index = 0; index < targets.u.arr.size; index++) {
        RzTarget *target = rz_load_target(targets.u.arr.elem[index]);
        if (rz_find_target(&project, target->name) != NULL) {
            rz_error("duplicate target '%s'", target->name);
        }
        list_push(&project.targets, target);
    }
    toml_datum_t packages = rz_toml_field(top, "dependencies", TOML_TABLE, false);
    for (int index = 0; index < packages.u.tab.size; index++) {
        list_push(&project.packages, rz_parse_package(packages.u.tab.key[index], packages.u.tab.value[index], false));
    }
    toml_free(parsed);
    return project;
}
