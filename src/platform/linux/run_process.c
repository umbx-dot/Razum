#include "platform/process.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

int run_process(const char *const *arguments, const char *directory) {
    int report[2];
    if (pipe(report) != 0) {
        return -1;
    }
    fcntl(report[1], F_SETFD, FD_CLOEXEC);
    pid_t child = fork();
    if (child < 0) {
        close(report[0]);
        close(report[1]);
        return -1;
    }
    if (child == 0) {
        close(report[0]);
        if (directory == NULL || chdir(directory) == 0) {
            execv(arguments[0], (char *const *)arguments);
        }
        int error = errno;
        ssize_t written = write(report[1], &error, sizeof error);
        _exit(written == (ssize_t)sizeof error ? 127 : 126);
    }
    close(report[1]);
    int error = 0;
    ssize_t received = read(report[0], &error, sizeof error);
    close(report[0]);
    int status = 0;
    if (waitpid(child, &status, 0) < 0 || received > 0) {
        return -1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}
