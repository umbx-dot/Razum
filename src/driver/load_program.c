#include "driver/driver.h"

#include <stdio.h>
#include <string.h>

#include "common/arena.h"
#include "common/diagnostics.h"
#include "common/files.h"
#include "common/text_buffer.h"
#include "embedded/embedded_files.h"
#include "lexing/lexing.h"
#include "linking/linking.h"
#include "parsing/parsing.h"
#include "platform/filesystem.h"

static const char EMBEDDED_PREFIX[] = "<rzc>/libraries/";

typedef struct {
    Program *program;
    PointerList loaded;
    PointerList *units;
    PointerList folders;
} Loader;

PointerList import_folders = {0};

static char *joined(const char *first, const char *second, const char *third) {
    size_t size = strlen(first) + strlen(second) + strlen(third) + 1;
    char *result = arena_alloc(size);
    snprintf(result, size, "%s%s%s", first, second, third);
    return result;
}

static char *directory_of(const char *path) {
    const char *slash = NULL;
    for (const char *cursor = path; *cursor != '\0'; cursor++) {
        if (*cursor == '/' || *cursor == '\\') {
            slash = cursor;
        }
    }
    return slash == NULL ? arena_copy_text("", 0) : arena_copy_text(path, (size_t)(slash - path + 1));
}

static const char *without_dot_slash(const char *path) {
    return (path[0] == '.' && (path[1] == '/' || path[1] == '\\')) ? path + 2 : path;
}

#ifdef _WIN32
static char path_character(char character) {
    if (character == '\\') {
        return '/';
    }
    return character >= 'A' && character <= 'Z' ? (char)(character + 'a' - 'A') : character;
}
#endif

static bool same_path(const char *left, const char *right) {
    left = without_dot_slash(left);
    right = without_dot_slash(right);
#ifndef _WIN32
    return strcmp(left, right) == 0;
#else
    for (; *left != '\0' && *right != '\0'; left++, right++) {
        if (*left != *right && path_character(*left) != path_character(*right)) {
            return false;
        }
    }
    return *left == *right;
#endif
}

static bool was_loaded(const Loader *loader, const char *key) {
    for (int index = 0; index < loader->loaded.count; index++) {
        if (same_path(LIST_AT(loader->loaded, const char, index), key)) {
            return true;
        }
    }
    return false;
}

static bool is_loaded(Loader *loader, const char *key) {
    if (was_loaded(loader, key)) {
        return true;
    }
    list_push(&loader->loaded, (void *)key);
    return false;
}

static void parse_template(Loader *loader, const char *path, const char *text, bool integers_only) {
    for (int index = 0; index < TEMPLATE_TYPE_COUNT; index++) {
        if (!integers_only || TEMPLATE_TYPES[index].is_integer) {
            char *instance = instantiate_template(text, &TEMPLATE_TYPES[index]);
            parse_file(tokenize(instance, strlen(instance)), loader->program, path, instance, true);
        }
    }
}

typedef enum {
    NOT_A_TEMPLATE,
    TEMPLATE_FOR_EVERY_NUMBER,
    TEMPLATE_FOR_INTEGERS,
} TemplateSet;

static TemplateSet template_set(const char *path) {
    char *normalized = arena_copy_text(path, strlen(path));
    for (char *cursor = normalized; *cursor != '\0'; cursor++) {
        *cursor = *cursor == '\\' ? '/' : *cursor;
    }
    if (strstr(normalized, "libraries/array/each/") != NULL) {
        return TEMPLATE_FOR_EVERY_NUMBER;
    }
    return strstr(normalized, "libraries/array/integers/") != NULL ? TEMPLATE_FOR_INTEGERS : NOT_A_TEMPLATE;
}

static void parse_source(Loader *loader, const char *path, const char *text, size_t length, bool is_library) {
    if (is_loaded(loader, path)) {
        return;
    }
    diagnostic_source_path = path;
    TemplateSet set = template_set(path);
    if (set == NOT_A_TEMPLATE) {
        parse_file(tokenize(text, length), loader->program, path, text, is_library);
    } else {
        parse_template(loader, path, text, set == TEMPLATE_FOR_INTEGERS);
    }
}

static bool has_suffix(const char *text, const char *suffix) {
    size_t text_length = strlen(text);
    size_t suffix_length = strlen(suffix);
    return text_length >= suffix_length && strcmp(text + text_length - suffix_length, suffix) == 0;
}

static bool fits_target(const char *path, Target target) {
    const char *other = target == TARGET_WINDOWS ? "linux" : "windows";
    size_t length = strlen(other);
    for (const char *segment = path; segment != NULL; segment = strchr(segment, '/')) {
        segment += *segment == '/';
        if (strncmp(segment, other, length) == 0 && (segment[length] == '/' || segment[length] == '.' || segment[length] == '\0')) {
            return false;
        }
    }
    return true;
}

static const char *const WINDOWS_ONLY_LIBRARIES[] = {"windows", "winsock"};

static bool is_windows_only_library(const char *name, size_t length) {
    for (size_t index = 0; index < sizeof WINDOWS_ONLY_LIBRARIES / sizeof WINDOWS_ONLY_LIBRARIES[0]; index++) {
        if (strlen(WINDOWS_ONLY_LIBRARIES[index]) == length && strncmp(WINDOWS_ONLY_LIBRARIES[index], name, length) == 0) {
            return true;
        }
    }
    return false;
}

static bool load_embedded(Loader *loader, const char *relative) {
    char *file = joined(relative, ".rz", "");
    char *folder = joined(relative, "/", "");
    bool found = false;
    for (int index = 0; index < EMBEDDED_FILE_COUNT; index++) {
        const EmbeddedFile *embedded = &EMBEDDED_FILES[index];
        bool in_folder = strncmp(embedded->path, folder, strlen(folder)) == 0 && has_suffix(embedded->path, ".rz");
        if ((strcmp(embedded->path, file) == 0 || in_folder) && fits_target(embedded->path, loader->program->target)) {
            parse_source(loader, joined(EMBEDDED_PREFIX, embedded->path, ""), embedded->text, strlen(embedded->text), true);
            found = true;
        }
    }
    return found;
}

static void load_disk_file(Loader *loader, const char *path) {
    size_t length = 0;
    char *text = read_file(path, &length);
    if (text == NULL) {
        fatal_error("cannot read %s", path);
    }
    parse_source(loader, path, text, length, true);
}

static void load_disk_units(Loader *loader, const char *folder) {
    PointerList paths = {0};
    list_files(folder, ".asm", true, &paths);
    for (int index = 0; index < paths.count; index++) {
        const char *path = LIST_AT(paths, const char, index);
        size_t length = 0;
        AsmUnit *unit = arena_alloc(sizeof *unit);
        unit->origin = path;
        unit->text = read_file(path, &length);
        if (unit->text == NULL) {
            fatal_error("cannot read %s", path);
        }
        list_push(loader->units, unit);
    }
}

static bool load_disk(Loader *loader, const char *base, const char *relative) {
    char *file = joined(base, relative, ".rz");
    char *folder = joined(base, relative, "");
    if (path_kind(file) == PATH_FILE) {
        load_disk_file(loader, file);
        return true;
    }
    if (path_kind(folder) != PATH_DIRECTORY) {
        return false;
    }
    PointerList paths = {0};
    list_files(folder, ".rz", true, &paths);
    for (int index = 0; index < paths.count; index++) {
        load_disk_file(loader, LIST_AT(paths, const char, index));
    }
    load_disk_units(loader, folder);
    return true;
}

static const char *library_names(Target target) {
    TextBuffer names = {0};
    for (int index = 0; index < EMBEDDED_FILE_COUNT; index++) {
        const char *path = EMBEDDED_FILES[index].path;
        size_t length = strcspn(path, "/.");
        if (!has_suffix(path, ".rz") || !fits_target(path, target) || (target == TARGET_LINUX && is_windows_only_library(path, length))) {
            continue;
        }
        char *name = arena_copy_text(path, length);
        if (names.data == NULL || strstr(names.data, name) == NULL) {
            text_append(&names, names.length > 0 ? ", " : "");
            text_append(&names, name);
        }
    }
    return names.data != NULL ? names.data : "none";
}

static void load_import(Loader *loader, const Import *import) {
    char *relative = arena_copy_text(import->name, strlen(import->name));
    for (char *cursor = relative; *cursor != '\0'; cursor++) {
        if (*cursor == '.') {
            *cursor = '/';
        }
    }
    const char *base = directory_of(import->source_path);
    size_t prefix_length = strlen(EMBEDDED_PREFIX);
    bool is_library_file = strncmp(base, EMBEDDED_PREFIX, prefix_length) == 0;
    bool is_platform_library = strcmp(relative, "windows") == 0 || strcmp(relative, "linux") == 0;
    bool found = is_platform_library ? false
               : is_library_file     ? load_embedded(loader, joined(base + prefix_length, relative, ""))
                                     : load_disk(loader, base, relative);
    for (int index = 0; !found && !is_platform_library && index < loader->folders.count; index++) {
        const char *folder = LIST_AT(loader->folders, const char, index);
        found = strcmp(base, folder) != 0 && load_disk(loader, folder, relative);
    }
    if (!found && loader->program->target == TARGET_LINUX && !is_library_file && is_windows_only_library(relative, strcspn(relative, "/"))) {
        diagnostic_source_path = import->source_path;
        compile_error(import->line, import->column, "the %.*s library uses Windows and is not available in Linux programs yet (see docs/platforms.md)",
                      (int)strcspn(relative, "/"), relative);
    }
    if (!found && !load_embedded(loader, relative)) {
        diagnostic_source_path = import->source_path;
        compile_error(import->line, import->column,
                      "unknown library or module '%s': no %s.rz file or %s folder next to this file, the main file or an -I folder, and no built-in library of that name (built in: %s)",
                      import->name, relative, relative, library_names(loader->program->target));
    }
}

static void add_embedded_units(PointerList *units, Target target) {
    for (int index = 0; index < EMBEDDED_FILE_COUNT; index++) {
        if (has_suffix(EMBEDDED_FILES[index].path, ".asm") && fits_target(EMBEDDED_FILES[index].path, target)) {
            AsmUnit *unit = arena_alloc(sizeof *unit);
            unit->origin = joined(EMBEDDED_PREFIX, EMBEDDED_FILES[index].path, "");
            unit->text = EMBEDDED_FILES[index].text;
            list_push(units, unit);
        }
    }
}

static bool defines_main(const Program *program) {
    for (int index = 0; index < program->functions.count; index++) {
        if (strcmp(LIST_AT(program->functions, Function, index)->name, "main") == 0) {
            return true;
        }
    }
    return false;
}

static void append_all(PointerList *target, const PointerList *source) {
    for (int index = 0; index < source->count; index++) {
        list_push(target, source->items[index]);
    }
}

static void load_module_siblings(Loader *loader, const char *main_path) {
    const char *base = directory_of(main_path);
    size_t length = strlen(base);
    const char *folder = length == 0 ? "." : arena_copy_text(base, length - 1);
    PointerList paths = {0};
    const char *platform = loader->program->target == TARGET_WINDOWS ? "windows" : "linux";
    list_files(folder, ".rz", false, &paths);
    list_files(joined(folder, "/", platform), ".rz", false, &paths);
    const char *name = folder;
    for (const char *cursor = folder; *cursor != '\0'; cursor++) {
        name = *cursor == '/' || *cursor == '\\' ? cursor + 1 : name;
    }
    if (strcmp(name, platform) == 0 && name != folder) {
        list_files(arena_copy_text(folder, (size_t)(name - folder) - 1), ".rz", false, &paths);
    }
    for (int index = 0; index < paths.count; index++) {
        const char *path = LIST_AT(paths, const char, index);
        size_t text_length = 0;
        char *text = was_loaded(loader, path) ? NULL : read_file(path, &text_length);
        if (text == NULL) {
            continue;
        }
        if (template_set(path) != NOT_A_TEMPLATE) {
            parse_source(loader, path, text, text_length, true);
            continue;
        }
        Program sibling = {0};
        diagnostic_source_path = path;
        parse_file(tokenize(text, text_length), &sibling, path, text, true);
        if (defines_main(&sibling)) {
            continue;
        }
        is_loaded(loader, path);
        append_all(&loader->program->imports, &sibling.imports);
        append_all(&loader->program->structs, &sibling.structs);
        append_all(&loader->program->enums, &sibling.enums);
        append_all(&loader->program->globals, &sibling.globals);
        append_all(&loader->program->functions, &sibling.functions);
    }
    diagnostic_source_path = main_path;
}

LoadedProgram load_program(const char *main_path, const char *main_text, size_t main_length, bool allow_module) {
    LoadedProgram result = {0};
    result.program = arena_alloc(sizeof *result.program);
    result.program->main_path = main_path;
    result.program->target = host_target();
    Loader loader = {result.program, {0}, &result.units, {0}};
    list_push(&loader.folders, directory_of(main_path));
    for (int index = 0; index < import_folders.count; index++) {
        const char *folder = LIST_AT(import_folders, const char, index);
        size_t length = strlen(folder);
        bool ends_in_slash = length > 0 && (folder[length - 1] == '/' || folder[length - 1] == '\\');
        list_push(&loader.folders, ends_in_slash ? (char *)folder : joined(folder, "/", ""));
    }
    add_embedded_units(&result.units, result.program->target);
    diagnostic_source_path = main_path;
    parse_source(&loader, main_path, main_text, main_length, false);
    result.is_module = allow_module && diagnostic_count == 0 && !defines_main(result.program);
    if (result.is_module) {
        load_module_siblings(&loader, main_path);
    }
    for (int index = 0; index < result.program->imports.count; index++) {
        load_import(&loader, LIST_AT(result.program->imports, Import, index));
    }
    return result;
}
