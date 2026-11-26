#include "project.h"

#include <string.h>

#include "common/arena.h"
#include "common/files.h"
#include "platform/process.h"

static bool stable_version(const char *tag, uint64_t version[3]) {
    const char *at = tag + (*tag == 'v');
    for (int index = 0; index < 3; index++) {
        if (*at < '0' || *at > '9' || (*at == '0' && at[1] >= '0' && at[1] <= '9')) {
            return false;
        }
        uint64_t number = 0;
        do {
            unsigned digit = (unsigned)(*at++ - '0');
            if (number > (UINT64_MAX - digit) / 10) {
                return false;
            }
            number = number * 10 + digit;
        } while (*at >= '0' && *at <= '9');
        version[index] = number;
        if (index == 2) {
            return *at == '\0';
        }
        if (*at++ != '.') {
            return false;
        }
    }
    return false;
}

static int compare_versions(const uint64_t left[3], const uint64_t right[3]) {
    for (int index = 0; index < 3; index++) {
        if (left[index] != right[index]) {
            return left[index] > right[index] ? 1 : -1;
        }
    }
    return 0;
}

const char *rz_latest_tag(const RzProject *project, const char *url, const char *current) {
    uint64_t best_version[3];
    if (current != NULL && !stable_version(current, best_version)) {
        rz_error("'%s' is not a stable version tag; specify the desired tag explicitly", current);
    }
    const char *state = rz_join(project->root, "build");
    const char *stage = rz_temporary_directory(state, "tags");
    const char *output = rz_join(stage, "refs");
    const char *command[] = {rz_tool("git", NULL), "-c", "protocol.ext.allow=never",
                             "ls-remote", "--tags", "--refs", "--", url, NULL};
    int status = run_process_output(command, project->root, output);
    size_t size = 0;
    char *refs = status == 0 ? read_file(output, &size) : NULL;
    remove(output);
    rz_remove_directory(stage);
    if (refs == NULL) {
        rz_error("cannot list package tags at %s", url);
    }
    const char *best = current;
    for (char *line = refs; *line != '\0';) {
        char *end = strchr(line, '\n');
        if (end == NULL) {
            end = line + strlen(line);
        }
        char *next = *end == '\0' ? end : end + 1;
        if (end > line && end[-1] == '\r') {
            end--;
        }
        *end = '\0';
        const char *separator = strchr(line, '\t');
        static const char prefix[] = "\trefs/tags/";
        uint64_t version[3];
        if (separator != NULL && strncmp(separator, prefix, sizeof prefix - 1) == 0 &&
            stable_version(separator + sizeof prefix - 1, version)) {
            const char *tag = separator + sizeof prefix - 1;
            int order = best != NULL ? compare_versions(version, best_version) : 1;
            if (order > 0 || (best != current && order == 0 && tag[0] == 'v')) {
                best = tag;
                memcpy(best_version, version, sizeof best_version);
            }
        }
        line = next;
    }
    if (best == NULL) {
        rz_error("no stable version tags at %s; specify an exact tag", url);
    }
    return best;
}
