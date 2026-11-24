#include "analysis/analysis.h"

bool contains_asm(const Statement *statement) {
    if (statement == NULL) {
        return false;
    }
    switch (statement->kind) {
        case STATEMENT_ASM:
            return true;
        case STATEMENT_BLOCK:
            for (int index = 0; index < statement->block.count; index++) {
                if (contains_asm(LIST_AT(statement->block, Statement, index))) {
                    return true;
                }
            }
            return false;
        case STATEMENT_IF:
            return contains_asm(statement->if_statement.then_branch) || contains_asm(statement->if_statement.else_branch);
        case STATEMENT_WHILE:
        case STATEMENT_DO:
            return contains_asm(statement->while_statement.body);
        case STATEMENT_FOR:
            return contains_asm(statement->for_statement.initial) || contains_asm(statement->for_statement.step) ||
                   contains_asm(statement->for_statement.body);
        case STATEMENT_SWITCH:
            for (int index = 0; index < statement->switch_statement.cases.count; index++) {
                if (contains_asm(LIST_AT(statement->switch_statement.cases, SwitchCase, index)->body)) {
                    return true;
                }
            }
            return false;
        default:
            return false;
    }
}
