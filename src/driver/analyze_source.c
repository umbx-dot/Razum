#include "driver/driver.h"

#include "checking/checking.h"
#include "common/diagnostics.h"

typedef struct {
    LoadedProgram *loaded;
} AnalyzeWork;

static void check_work(void *data) {
    AnalyzeWork *work = data;
    check_program(work->loaded->program, false);
}

LoadedProgram analyze_source(const char *source_path, const char *text, size_t length) {
    LoadedProgram loaded = load_program(source_path, text, length, true);
    AnalyzeWork work = {&loaded};
    diagnostic_try(check_work, &work);
    return loaded;
}
