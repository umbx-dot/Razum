#include "project.h"

#include <stdlib.h>
#include <string.h>

RzOptions rz_options(int argc, char **argv) {
    static const char usage[] =
        "rz build [--manifest FILE] [--target NAME] [--profile NAME] [--offline|--locked]\n"
        "rz fetch [--manifest FILE] [--offline|--locked]\n"
        "rz add OWNER/REPO[@TAG] [--manifest FILE]\n"
        "rz update ALIAS[@TAG] [--manifest FILE]\n"
        "rz compiler-args --target NAME [--profile NAME] [--manifest FILE]\n";
    if (argc == 1 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "help") == 0) {
        fputs(usage, stdout);
        exit(argc == 1 ? 2 : 0);
    }
    RzOptions result = {0};
    result.command = argv[1];
    result.profile = "dev";
    bool add = strcmp(result.command, "add") == 0;
    bool edit = add || strcmp(result.command, "update") == 0;
    bool build = strcmp(result.command, "build") == 0;
    bool flags = strcmp(result.command, "compiler-args") == 0;
    if (!edit && !build && !flags && strcmp(result.command, "fetch") != 0) {
        rz_error("unknown command '%s'; run rz --help", result.command);
    }
    for (int index = 2; index < argc; index++) {
        const char *arg = argv[index];
        if (strcmp(arg, "--offline") == 0 && !edit) {
            result.offline = true;
            result.locked = true;
        } else if (strcmp(arg, "--locked") == 0 && !edit) {
            result.locked = true;
        } else if (strcmp(arg, "--manifest") == 0 && index + 1 < argc) {
            result.manifest = argv[++index];
        } else if ((build || flags) && strcmp(arg, "--profile") == 0 && index + 1 < argc) {
            result.profile = argv[++index];
        } else if ((build || flags) && strcmp(arg, "--target") == 0 && index + 1 < argc) {
            result.target = argv[++index];
        } else if (edit && result.package == NULL && arg[0] != '-') {
            result.package = arg;
        } else {
            rz_error("unexpected argument '%s'; run rz --help", arg);
        }
    }
    if (!rz_name(result.profile) || (result.target != NULL && !rz_name(result.target))) {
        rz_error("invalid profile or target name");
    }
    if (edit && result.package == NULL) {
        rz_error("specify %s", add ? "OWNER/REPO[@TAG]" : "ALIAS[@TAG]");
    }
    if (flags && result.target == NULL) {
        rz_error("compiler-args requires --target NAME");
    }
    return result;
}
