#include "codegen/codegen.h"

void emit_epilogue(FunctionContext *context) {
    for (int index = 0; index < context->saved_count; index++) {
        emit(context, "mov %s, qword [rsp + .@save%d]", context->saved_registers[index], index);
    }
    for (int index = 0; index < context->saved_vector_count; index++) {
        emit(context, "movups %s, [rsp + .@vsave%d]", context->saved_vectors[index], index);
    }
    emit(context, "add rsp, .@frame");
    emit(context, "ret");
}

void emit_simple_statement(FunctionContext *context, Statement *statement) {
    switch (statement->kind) {
        case STATEMENT_DECLARATION:
        case STATEMENT_ASSIGNMENT:
        case STATEMENT_INCREMENT:
            emit_assignment(context, statement);
            return;
        case STATEMENT_RETURN:
            if (context->tail_label != NULL && emit_tail_return(context, statement->return_value)) {
                return;
            }
            if (statement->return_value != NULL) {
                emit_value(context, statement->return_value, context->function->return_type);
                if (context->tail_sum != NULL) {
                    emit(context, "add rax, %s", context->tail_sum);
                }
            }
            emit_epilogue(context);
            return;
        case STATEMENT_BREAK:
            emit(context, "jmp %s", LIST_AT(context->loops, LoopLabels, context->loops.count - 1)->break_label);
            return;
        case STATEMENT_CONTINUE:
            emit(context, "jmp %s", LIST_AT(context->loops, LoopLabels, context->loops.count - 1)->continue_label);
            return;
        case STATEMENT_CALL:
            emit_call(context, statement->call);
            return;
        case STATEMENT_ASM:
            emit_asm_block(context, statement->asm_text);
            return;
        default:
            emit_statement(context, statement);
            return;
    }
}

void emit_statement(FunctionContext *context, Statement *statement) {
    switch (statement->kind) {
        case STATEMENT_BLOCK:
            for (int index = 0; index < statement->block.count; index++) {
                emit_statement(context, LIST_AT(statement->block, Statement, index));
            }
            return;
        case STATEMENT_IF:
        case STATEMENT_WHILE:
        case STATEMENT_FOR:
        case STATEMENT_DO:
        case STATEMENT_SWITCH:
            emit_source_comment(context, statement->line);
            emit_control(context, statement);
            return;
        default:
            emit_source_comment(context, statement->line);
            emit_simple_statement(context, statement);
            return;
    }
}
