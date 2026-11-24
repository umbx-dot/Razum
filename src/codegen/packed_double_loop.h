#pragma once

#include "codegen/codegen.h"

enum { PACKED_DOUBLE_REGISTERS = 16, PACKED_DOUBLE_VARIABLES = 6, FIRST_SAVED_DOUBLE_VECTOR = 6 };

typedef struct {
    const Symbol *symbol;
    const Expression *initial;
    int next;
} PackedDoubleVariable;

typedef struct {
    const Expression *input;
    Operator operator;
    int left;
    int right;
} PackedDoubleValue;

typedef struct {
    const Symbol *index;
    const Symbol *total;
    Expression *bound;
    Expression *limit;
    PackedDoubleVariable variables[PACKED_DOUBLE_VARIABLES];
    PackedDoubleValue values[PACKED_DOUBLE_REGISTERS];
    int variable_count;
    int init_register_count;
    int value_count;
    int comparison;
    int compare_left;
    int compare_right;
    int compare_predicate;
    bool avx;
} PackedDoubleLoop;

bool plan_packed_double_loop(PackedDoubleLoop *loop, const Statement *statement);
bool plan_packed_double_step(PackedDoubleLoop *loop, const Expression *condition, const Statement *body);
void emit_packed_double_inputs(FunctionContext *context, const PackedDoubleLoop *loop,
                               const Operand *saved, const char *lanes);
void emit_packed_double_operations(FunctionContext *context, const PackedDoubleLoop *loop, int first, int end);
