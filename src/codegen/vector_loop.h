#pragma once

#include "codegen/codegen.h"

typedef struct {
    const Symbol *index;
    const Symbol *sum;
    const Symbol *store_base;
    Expression *bound;
    const Expression *value;
    ValueType comparison;
    int width;
    int lanes;
    bool avx;
    bool uses_index;
    bool nonnegative_index;
    bool index_fits_u32;
} VectorLoop;

bool plan_vector_loop(VectorLoop *loop, const Statement *statement, bool avx);
void emit_vector_expression(FunctionContext *context, const VectorLoop *loop, const Expression *value, int target);
void emit_vector_binary(FunctionContext *context, const VectorLoop *loop, const char *instruction, int target, int right);
void emit_vector_broadcast(FunctionContext *context, const VectorLoop *loop, int target, const char *source);
void emit_vector_access(FunctionContext *context, const VectorLoop *loop, const Symbol *base, int target, bool store);
