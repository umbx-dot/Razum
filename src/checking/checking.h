#pragma once

#include "syntax/syntax.h"

typedef struct {
    const char *name;
    Symbol *symbol;
    int depth;
} ScopeEntry;

typedef struct {
    const char *name;
    int uses;
} LabelUse;

typedef struct {
    PointerList entries;
    PointerList label_uses;
    int depth;
    Function *function;
    const Program *program;
} Scopes;

typedef struct {
    Program *program;
    Function *function;
    Scopes scopes;
    int loop_depth;
    int switch_depth;
} Checker;

void check_program(Program *program, bool requires_main);
void check_globals(Program *program);
void check_structs(const Program *program);
void layout_struct(const Program *program, StructType *structure);
void resolve_shape(const Program *program, Shape *shape);
ValueType check_index(Checker *checker, Expression *expression);
ValueType check_field(Checker *checker, Expression *expression);
void check_initial_value(Checker *checker, Shape *shape, Expression *initial, const char *name);
void check_copy_source(Checker *checker, const Shape *target, Expression *value, const char *name);
void check_signatures(const Program *program, bool requires_main);
void check_function(Checker *checker, Function *function);
void check_statement(Checker *checker, Statement *statement);
void check_statement_safely(Checker *checker, Statement *statement);
void check_store(Checker *checker, const Shape *target, Expression *value, const char *name);
bool is_one_character_text(const Expression *value);
void text_as_char(Expression *value);
bool rewrite_array_method(Checker *checker, Expression *call);
bool rewrite_end_element(Expression *expression);
void store_text_as_char(const Shape *target, Expression *value);
ValueType check_expression(Checker *checker, Expression *expression);
ValueType check_any_expression(Checker *checker, Expression *expression);
ValueType check_call(Checker *checker, Expression *call);
Function *find_function(const Program *program, const char *name);

void scopes_push(Scopes *scopes);
void scopes_pop(Scopes *scopes);
Symbol *scopes_declare(Scopes *scopes, const char *name, const Shape *shape, SymbolKind kind, int line, int column, int index);
Symbol *scopes_lookup(const Scopes *scopes, const char *name, int line, int column);
Symbol *scopes_find(const Scopes *scopes, const char *name);
Shape *function_shape(const Function *function, int line, int column);
bool is_address_value(ValueType type);

void check_condition(Checker *checker, Expression *condition, const char *context);

bool literal_fits(const Expression *value, ValueType target);
bool can_store(ValueType target, const Expression *value);
bool can_store_shape(const Shape *target, const Expression *value);
ValueType check_address(Checker *checker, Expression *expression);
ValueType check_deref(Checker *checker, Expression *expression);
ValueType check_new(Checker *checker, Expression *expression);
ValueType check_sizeof(Checker *checker, Expression *expression);
bool pointers_compatible(const Expression *left, const Expression *right);
ValueType operation_type(const Expression *left, const Expression *right, int line, int column, const char *operator);
ValueType promoted_type(ValueType type);
void check_switch(Checker *checker, Statement *statement);
const EnumType *find_enum(const Program *program, const char *name);
void check_enums(Program *program);
void declare_globals(Program *program);
void separate_prototypes(Program *program);
void check_prototypes(const Program *program);
Expression *fold_constant(const Expression *expression);
const Expression *constant_value(Checker *checker, const Shape *shape, Expression *initial, const char *name);
void resolve_global_constant(Program *program, GlobalVariable *variable);
void resolve_count(Checker *checker, Shape *shape);
