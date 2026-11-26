#include "project.h"

#include <stdlib.h>
#include <string.h>
#include "common/files.h"
#include "platform/filesystem.h"

static const char *cache_root(void) {
    const char *root = getenv("RZ_CACHE");
    if (root != NULL && *root != '\0') {
        return root;
    }
#ifdef _WIN32
    root = getenv("LOCALAPPDATA");
    return root != NULL ? rz_join(root, "razum/cache/packages") : NULL;
#else
    root = getenv("XDG_CACHE_HOME");
    if (root != NULL && *root != '\0') {
        return rz_join(root, "razum/packages");
    }
    root = getenv("HOME");
    return root != NULL ? rz_join(root, ".cache/razum/packages") : NULL;
#endif
}

static const char *head_revision(const char *checkout) {
    size_t size = 0;
    char *head = read_file(rz_join(checkout, ".git/HEAD"), &size);
    if (head == NULL) {
        return NULL;
    }
    while (size > 0 && (head[size - 1] == '\r' || head[size - 1] == '\n')) {
        head[--size] = '\0';
    }
    return rz_revision(head) ? head : NULL;
}

static void git(const char *checkout, const char *const *args) {
    PointerList arguments = {0};
    rz_add_argument(&arguments, rz_tool("git", NULL));
    rz_add_argument(&arguments, "-c");
    rz_add_argument(&arguments, "core.hooksPath=.git/razum-no-hooks");
    rz_add_argument(&arguments, "-c");
    rz_add_argument(&arguments, "protocol.ext.allow=never");
    for (int index = 0; args[index] != NULL; index++) {
        rz_add_argument(&arguments, args[index]);
    }
    rz_run(arguments, checkout);
}

const char *rz_cached_package(RzPackage *package, bool offline) {
    const char *root = cache_root();
    if (root == NULL) {
        rz_error("cannot locate the package cache; set RZ_CACHE");
    }
    if (path_kind(root) == PATH_DIRECTORY) {
        root = rz_absolute(root);
    }
    if (package->revision != NULL) {
        const char *path = rz_join(root, package->revision);
        if (path_kind(path) == PATH_DIRECTORY) {
            const char *head = head_revision(path);
            if (head == NULL || strcmp(head, package->revision) != 0) {
                rz_error("package '%s': checkout does not match locked commit in %s", package->alias, path);
            }
            return path;
        }
    }
    if (offline) {
        rz_error("package '%s' is missing from cache; run rz fetch", package->alias);
    }
    rz_make_directories(root);
    root = rz_absolute(root);
    const char *checkout = rz_temporary_directory(root, "fetch");
    git(checkout, (const char *const[]){"init", "-q", "--template=", NULL});
    const char *ref = package->revision != NULL ? package->revision : rz_format("refs/tags/%s", package->tag);
    git(checkout, (const char *const[]){"fetch", "-q", "--no-tags", "--depth=1", "--", package->url, ref, NULL});
    git(checkout, (const char *const[]){"checkout", "-q", "--detach", "FETCH_HEAD", NULL});
    const char *head = head_revision(checkout);
    if (head == NULL || (package->revision != NULL && strcmp(head, package->revision) != 0) ||
        !rz_no_links(checkout, true)) {
        rz_error("package '%s': checkout has unexpected contents or commit", package->alias);
    }
    package->revision = head;
    const char *path = rz_join(root, head);
    if (!rz_replace(checkout, path)) {
        const char *existing = head_revision(path);
        if (existing == NULL || strcmp(existing, head) != 0) {
            rz_error("cannot publish package cache directory %s", path);
        }
    }
    return path;
}
