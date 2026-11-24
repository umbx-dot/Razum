#include "checking/checking.h"

#include "common/diagnostics.h"

typedef struct {
    Checker *checker;
    Statement *statement;
} StatementWork;

static void check_one(void *data) {
    StatementWork *work = data;
    check_statement(work->checker, work->statement);
}

void check_statement_safely(Checker *checker, Statement *statement) {
    int entries = checker->scopes.entries.count;
    int depth = checker->scopes.depth;
    int loops = checker->loop_depth;
    int switches = checker->switch_depth;
    StatementWork work = {checker, statement};
    if (!diagnostic_try(check_one, &work)) {
        checker->scopes.entries.count = entries;
        checker->scopes.depth = depth;
        checker->loop_depth = loops;
        checker->switch_depth = switches;
    }
}
