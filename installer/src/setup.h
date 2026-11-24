#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdnoreturn.h>
#include <wchar.h>

enum { PATH_SIZE = 1024, MESSAGE_SIZE = 4096 };

typedef struct {
    wchar_t root[PATH_SIZE];
    wchar_t bin[PATH_SIZE];
    wchar_t cmake[PATH_SIZE];
} InstallPaths;

typedef struct {
    wchar_t command[PATH_SIZE];
    wchar_t executable[PATH_SIZE];
} VsCode;

void install(bool include_vscode);
void uninstall(void);

bool install_paths(InstallPaths *paths);
bool extract_resource(int id, const wchar_t *path);

bool path_list_contains(const wchar_t *list, const wchar_t *entry);
bool path_list_add(const wchar_t *list, const wchar_t *entry, wchar_t *out, size_t capacity);
bool path_list_remove(const wchar_t *list, const wchar_t *entry, wchar_t *out, size_t capacity);
bool add_to_user_path(const wchar_t *directory);
bool remove_from_user_path(const wchar_t *directory);

bool set_user_string(const wchar_t *key, const wchar_t *name, const wchar_t *value);
bool register_file_type(const InstallPaths *paths, const VsCode *vscode);
bool forget_open_with_choice(void);
void unregister_file_type(void);
bool register_uninstall_entry(const InstallPaths *paths);
void unregister_uninstall_entry(void);
bool register_cmake_package(const InstallPaths *paths);
void unregister_cmake_package(void);

bool find_vscode(VsCode *vscode);
int run_hidden(wchar_t *command_line);

void set_quiet(bool is_quiet);
bool ask(const wchar_t *question);
void inform(const wchar_t *message);
noreturn void fail(const wchar_t *format, ...);
