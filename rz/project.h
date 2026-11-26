#pragma once

#include <stdnoreturn.h>
#include <stdint.h>

#include "common/list.h"
#include "common/text_buffer.h"
#include "vendor/tomlc17/tomlc17.h"

typedef enum { RZ_EXE, RZ_DLL, RZ_OBJECT, RZ_SHARED, RZ_CMAKE } RzKind;

typedef struct {
    int optimize;
    int assembly;
    int cpu;
} RzFlags;

typedef struct {
    const char *name;
    const char *source;
    RzKind kind;
    int platforms;
    int subsystem;
    const char *cmake_target;
    const char *artifact;
    const char *preset;
    RzFlags flags;
    PointerList needs;
    PointerList imports;
    int visiting;
} RzTarget;

typedef struct {
    const char *alias;
    const char *url;
    const char *tag;
    const char *revision;
} RzPackage;

typedef struct {
    const char *file;
    const char *manifest_text;
    const char *root;
    const char *name;
    RzFlags flags;
    PointerList targets;
    PointerList packages;
} RzProject;

typedef struct {
    const char *command;
    const char *manifest;
    const char *profile;
    const char *target;
    const char *package;
    bool offline;
    bool locked;
} RzOptions;

#ifdef _WIN32
enum { RZ_HOST = 1 };
#else
enum { RZ_HOST = 2 };
#endif

noreturn void rz_error(const char *format, ...);
const char *rz_format(const char *format, ...);
bool rz_name(const char *name);
bool rz_relative(const char *path);
const char *rz_join(const char *directory, const char *name);
const char *rz_parent(const char *path);
const char *rz_absolute(const char *path);
const char *rz_sibling(const char *name);
const char *rz_manifest_path(const char *given);
const char *rz_platform(void);
const char *rz_exe_suffix(void);
bool rz_make_directory(const char *path);
bool rz_replace(const char *from, const char *to);
bool rz_remove_directory(const char *path);
uintptr_t rz_lock_file(const char *path);
void rz_unlock_file(uintptr_t lock);
bool rz_no_links(const char *path, bool recursive);
const char *rz_temporary_directory(const char *parent, const char *prefix);
void rz_make_directories(const char *path);
const char *rz_tool(const char *name, const char *override);
void rz_run(PointerList arguments, const char *directory);
void rz_add_argument(PointerList *arguments, const char *argument);
double rz_milliseconds(void);

toml_result_t rz_read_toml(const char *file, const char **source);
const char *rz_toml_string(toml_datum_t value, const char *field);
toml_datum_t rz_toml_field(toml_datum_t table, const char *name, toml_type_t type, bool required);
void rz_toml_keys(toml_datum_t table, const char *const *keys);
void rz_toml_strings(toml_datum_t array, PointerList *result, const char *field, bool paths);

RzOptions rz_options(int argc, char **argv);
RzProject rz_load_project(const char *path, const char *profile);
RzTarget *rz_load_target(toml_datum_t table);
RzFlags rz_read_flags(toml_datum_t table, RzFlags flags);
RzTarget *rz_find_target(const RzProject *project, const char *name);
PointerList rz_build_order(const RzProject *project, const char *requested);
const char *rz_target_filename(const RzTarget *target);
void rz_compiler_args(const RzProject *project, const RzTarget *target, PointerList *arguments);
PointerList rz_packages(RzProject *project, const RzOptions *options);
RzPackage *rz_parse_package(const char *name, toml_datum_t table, bool locked);
bool rz_revision(const char *revision);
const char *rz_cached_package(RzPackage *package, bool offline);
void rz_save_file(const char *path, const char *contents, size_t size);
PointerList rz_load_lock(const char *file);
void rz_save_lock(const char *file, PointerList packages);
TextBuffer rz_lock_contents(PointerList packages);
void rz_add_package(RzProject *project, const RzOptions *options);
void rz_update_package(RzProject *project, const RzOptions *options);
void rz_save_package_edit(RzProject *project, const char *manifest);
const char *rz_latest_tag(const RzProject *project, const char *url, const char *current);
const char *rz_toml_position(const char *source, toml_datum_t value);
bool rz_tag_name(const char *tag);
RzPackage *rz_find_package(PointerList packages, const char *name);
void rz_build(const RzProject *project, const RzOptions *options, PointerList imports);
void rz_build_cmake(const RzProject *project, const RzTarget *target, const RzOptions *options, const char *stage);
void rz_stage_native_inputs(const RzProject *project, const RzTarget *target, const char *stage, const char *dir);
bool rz_list_inputs(const char *folder, const char *excluded, const char *extension, PointerList *paths);
const char *rz_build_fingerprint(const RzProject *project, const RzOptions *options, const char *compiler,
                                 const char *linker, PointerList imports);
void rz_publish_outputs(const char *stage, const char *output, PointerList files);
