#include "analysis/analysis.h"

static bool breaks_out(const Statement *statement) {
    if (statement == NULL) {
        return false;
    }
    switch (statement->kind) {
        case STATEMENT_BREAK:
            return true;
        case STATEMENT_BLOCK:
            for (int index = 0; index < statement->block.count; index++) {
                if (breaks_out(LIST_AT(statement->block, Statement, index))) {
                    return true;
                }
            }
            return false;
        case STATEMENT_IF:
            return breaks_out(statement->if_statement.then_branch) || breaks_out(statement->if_statement.else_branch);
        default:
            return false;
    }
}

static bool switch_always_returns(const Statement *statement) {
    bool has_default = false;
    for (int index = 0; index < statement->switch_statement.cases.count; index++) {
        const SwitchCase *switch_case = LIST_AT(statement->switch_statement.cases, SwitchCase, index);
        has_default = has_default || switch_case->is_default;
        if (breaks_out(switch_case->body) || !always_returns(switch_case->body)) {
            return false;
        }
    }
    return has_default;
}

static bool is_always_true(const Expression *condition) {
    return condition == NULL || (condition->kind == EXPRESSION_INT && condition->int_literal.value != 0);
}

static bool never_ends(const Statement *statement) {
    switch (statement->kind) {
        case STATEMENT_WHILE:
            return is_always_true(statement->while_statement.condition) && !breaks_out(statement->while_statement.body);
        case STATEMENT_DO:
            return is_always_true(statement->while_statement.condition) && !breaks_out(statement->while_statement.body);
        case STATEMENT_FOR:
            return is_always_true(statement->for_statement.condition) && !breaks_out(statement->for_statement.body);
        default:
            return false;
    }
}

bool always_returns(const Statement *statement) {
    if (statement == NULL) {
        return false;
    }
    switch (statement->kind) {
        case STATEMENT_RETURN:
            return true;
        case STATEMENT_BLOCK:
            for (int index = 0; index < statement->block.count; index++) {
                if (always_returns(LIST_AT(statement->block, Statement, index))) {
                    return true;
                }
            }
            return false;
        case STATEMENT_IF:
            return always_returns(statement->if_statement.then_branch) && always_returns(statement->if_statement.else_branch);
        case STATEMENT_SWITCH:
            return switch_always_returns(statement);
        case STATEMENT_WHILE:
        case STATEMENT_DO:
        case STATEMENT_FOR:
            return never_ends(statement);
        default:
            return false;
    }
}
