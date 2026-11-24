#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/text_buffer.h"
#include "syntax/syntax.h"

enum { OPERAND_SIZE = 96 };

typedef struct {
    char text[OPERAND_SIZE];
    bool is_address;
} Operand;

typedef struct {
    const char *bytes;
    size_t length;
} StringConstant;

typedef struct {
    const char *text;
    PointerList starts;
} LineIndex;

typedef struct {
    PointerList line_indexes;
    int label_count;
    PointerList strings;
    PointerList numbers;
    int relocation_count;
    bool optimize;
    bool zen3;
} ProgramContext;

typedef struct {
    const char *continue_label;
    const char *break_label;
} LoopLabels;

typedef struct {
    const Symbol *left;
    const Symbol *right;
    const char *reg;
    int last_statement;
    bool ready;
} LoopProduct;

typedef struct {
    LoopProduct entries[2];
    int count;
    int statement;
    bool testing;
} LoopProducts;

typedef struct {
    ProgramContext *program;
    Function *function;
    TextBuffer body;
    int temps_in_use;
    int temp_count;
    int stack_argument_count;
    int last_comment_line;
    PointerList loops;
    const char *saved_registers[7];
    int saved_count;
    const char *saved_vectors[10];
    int saved_vector_count;
    int loop_vector_base;
    LoopProducts *loop_products;
    const char *tail_label;
    const char *tail_sum;
} FunctionContext;

char *emit_program(const Program *program, const PointerList *units, const char *dll_name);
char *emit_def_file(const Program *program, const char *dll_name);
char *emit_c_header(const Program *program);
const char *relocation_label(ProgramContext *program);
void emit_relocations(TextBuffer *output, const ProgramContext *program, bool is_dll);
void emit_export_section(TextBuffer *output, const Program *program, const char *dll_name);
void emit_dll_entry_point(TextBuffer *output, bool flushes_output);
const char *source_line(ProgramContext *program, const char *text, int line, size_t *length);

int string_constant_index(ProgramContext *program, const char *bytes, size_t length);
int number_constant_index(ProgramContext *program, double value);
uint64_t double_bits(double value);
void append_nasm_string(TextBuffer *buffer, const char *bytes, size_t length);

void emit(FunctionContext *context, const char *format, ...);
void emit_label(FunctionContext *context, const char *label);
void emit_source_comment(FunctionContext *context, int line);
const char *new_label(FunctionContext *context, const char *kind);
Operand acquire_temp(FunctionContext *context);
void release_temp(FunctionContext *context);

Operand memory_operand(const Symbol *symbol);
Operand sized_operand(const Symbol *symbol, const char *size);
bool int_operand(const Expression *expression, Operand *operand);
bool double_operand(ProgramContext *program, const Expression *expression, Operand *operand);
bool is_immediate(const Operand *operand);
bool is_memory_symbol(const Symbol *symbol);
void emit_new(FunctionContext *context, Expression *expression);

void emit_int(FunctionContext *context, Expression *expression);
bool emit_wide_offset(FunctionContext *context, Expression *expression, const char *destination);
bool emit_wide_tree(FunctionContext *context, Expression *expression, const char *destination);
const char *integer_arithmetic_instruction(Operator operator);
void emit_place_load(FunctionContext *context, Expression *expression);
const char *shift_instruction(Operator operator, ValueType type);
bool is_shift(Operator operator);
void emit_conditional(FunctionContext *context, Expression *expression, bool is_floating_result);
bool emit_conditional_move(FunctionContext *context, const Expression *expression);
Operand emit_int_pair(FunctionContext *context, Expression *left, Expression *right);
void emit_double(FunctionContext *context, Expression *expression);
bool emit_double_tree(FunctionContext *context, Expression *expression, const char *target);
bool emit_double_tree_to_symbol(FunctionContext *context, Expression *expression, const Symbol *target);
bool double_tree_fits(const Expression *expression);
bool double_product_operands(const Expression *expression, const Symbol **left, const Symbol **right);
const char *cached_double_product(FunctionContext *context, const Expression *expression);
const char *double_arithmetic_instruction(Operator operator);
Operand emit_double_pair(FunctionContext *context, Expression *left, Expression *right);
void emit_value(FunctionContext *context, Expression *expression, ValueType target);
void emit_narrow(FunctionContext *context, ValueType type);
void emit_int_to_double(FunctionContext *context, ValueType source);
void emit_double_to_int(FunctionContext *context, ValueType target);
void emit_divide(FunctionContext *context, ValueType operation);
void emit_load_floating(FunctionContext *context, const Symbol *symbol, const char *target);
void emit_store_floating(FunctionContext *context, const Symbol *symbol);
bool place_text(const Expression *expression, char *text, size_t size);
void emit_address(FunctionContext *context, Expression *expression);
void emit_length(FunctionContext *context, const Expression *expression);
const char *size_word(ValueType type);
const char *sized_register(ValueType type, bool is_rcx);
void emit_load_from(FunctionContext *context, ValueType type, const char *place);
void emit_store_to(FunctionContext *context, ValueType type, const char *place, bool from_rcx);
void emit_place_store(FunctionContext *context, Expression *target, Expression *value);
void emit_place_compound(FunctionContext *context, Expression *target, Operator operator, Expression *value);
void emit_place_increment(FunctionContext *context, Expression *target, bool is_increment);
void emit_copy(FunctionContext *context, Expression *target, Expression *source);
void emit_aggregate_declaration(FunctionContext *context, Statement *statement);
void jump_if(FunctionContext *context, Expression *expression, const char *target, bool when_true);
void emit_condition_value(FunctionContext *context, Expression *expression);
void emit_call(FunctionContext *context, Expression *call);

void emit_statement(FunctionContext *context, Statement *statement);
void emit_simple_statement(FunctionContext *context, Statement *statement);
void emit_assignment(FunctionContext *context, Statement *statement);
void emit_control(FunctionContext *context, Statement *statement);
void emit_vector_loop_prefix(FunctionContext *context, const Statement *statement);
bool emit_packed_double_loop_prefix(FunctionContext *context, const Statement *statement);
Statement *loop_invariant_declaration(const Statement *body, const Statement *step);
void plan_loop_products(FunctionContext *context, Expression *condition, const Statement *body, LoopProducts *products);
void emit_cached_loop_test(FunctionContext *context, Expression *condition, const char *top, const char *end, LoopProducts *products);
const char *reserve_loop_vector(FunctionContext *context, int index);
bool emit_mask_select(FunctionContext *context, Statement *statement);
bool mask_test_operands(const Expression *expression, const char **value, Operand *mask);
void emit_epilogue(FunctionContext *context);
void setup_tail_recursion(FunctionContext *context);
bool emit_tail_return(FunctionContext *context, Expression *expression);
void emit_fast_return(FunctionContext *context, TextBuffer *output);
bool small_recurrence(const Function *function, const Statement *guard, uint64_t *table, int size, int *first);
void assign_registers(FunctionContext *context);
void emit_asm_block(FunctionContext *context, const char *text);
Operand string_address(FunctionContext *context, const char *bytes, size_t length);
void emit_function(ProgramContext *program, Function *function, TextBuffer *output);

void emit_pe_header(TextBuffer *output, bool is_dll, bool is_window_program);
void emit_entry_point(TextBuffer *output, bool flushes_output, bool takes_arguments);
void emit_linux_entry_point(TextBuffer *output, bool flushes_output, bool takes_arguments, bool keeps_environment);
void emit_elf_header(TextBuffer *output);
void emit_data_section(TextBuffer *output, ProgramContext *program, const Program *source_program);
void emit_aggregate_global(TextBuffer *output, ProgramContext *program, const GlobalVariable *variable);
void emit_import_section(TextBuffer *output, const PointerList *imports);
