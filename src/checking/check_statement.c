#include "checking/checking.h"

#include "common/diagnostics.h"

static void check_branch(Checker *checker, Statement *statement) {
    scopes_push(&checker->scopes);
    check_statement(checker, statement);
    scopes_pop(&checker->scopes);
}

static void check_loop_body(Checker *checker, Statement *body) {
    checker->loop_depth++;
    check_branch(checker, body);
    checker->loop_depth--;
}

static void check_constant(Checker *checker, Statement *statement) {
    Shape *shape = statement->declaration.shape;
    const char *name = statement->declaration.name;
    const Expression *constant = constant_value(checker, shape, statement->declaration.initial, name);
    Symbol *symbol = scopes_declare(&checker->scopes, name, shape, SYMBOL_GLOBAL, statement->line, statement->column, 0);
    symbol->is_const = true;
    symbol->constant = constant;
    statement->kind = STATEMENT_BLOCK;
    statement->block = (PointerList){0};
}

typedef struct {
    Checker *checker;
    Shape *shape;
    Expression *initial;
    const char *name;
} InitialWork;

static void check_initial_work(void *data) {
    InitialWork *work = data;
    check_initial_value(work->checker, work->shape, work->initial, work->name);
}

static void check_declaration(Checker *checker, Statement *statement) {
    Shape *shape = statement->declaration.shape;
    const char *name = statement->declaration.name;
    for (Shape *level = shape; level->kind == TYPE_ARRAY; level = (Shape *)level->element) {
        resolve_count(checker, level);
    }
    resolve_shape(checker->program, shape);
    statement->declaration.type = shape->kind;
    if (statement->declaration.is_const) {
        check_constant(checker, statement);
        return;
    }
    if (shape->kind == TYPE_VOID) {
        compile_error(statement->line, statement->column, "a variable cannot be void");
    }
    if (statement->declaration.initial != NULL) {
        InitialWork work = {checker, shape, statement->declaration.initial, name};
        diagnostic_try(check_initial_work, &work);
    } else if (shape->kind == TYPE_ARRAY && shape->count < 0) {
        compile_error(statement->line, statement->column, "array '%s' needs a size, as in int %s[10], or a { } list", name, name);
    }
    statement->declaration.symbol = scopes_declare(&checker->scopes, name, shape, SYMBOL_LOCAL, statement->line, statement->column, 0);
}

static const char *describe_target(const Expression *target) {
    switch (target->kind) {
        case EXPRESSION_NAME: return target->name.name;
        case EXPRESSION_INDEX: return describe_target(target->index.base);
        default: return target->field.name;
    }
}

static ValueType check_target(Checker *checker, Expression *target) {
    if (target->kind == EXPRESSION_NAME) {
        const Symbol *symbol = scopes_find(&checker->scopes, target->name.name);
        if (symbol != NULL && symbol->is_const) {
            compile_error(target->line, target->column, "'%s' is a const; its value cannot change", target->name.name);
        }
    }
    ValueType type = check_any_expression(checker, target);
    if (target->kind == EXPRESSION_FIELD && target->field.is_length) {
        compile_error(target->line, target->column, "the length of an array is fixed and cannot be assigned");
    }
    if (target->kind == EXPRESSION_INDEX && target->index.base->type == TYPE_STRING) {
        compile_error(target->line, target->column, "the text of a string cannot be changed; copy it into a char array first");
    }
    return type;
}

static void check_assignment(Checker *checker, Statement *statement) {
    Expression *target = statement->assignment.target;
    ValueType type = check_target(checker, target);
    const char *name = describe_target(target);
    Operator operator = statement->assignment.operator;
    if (is_aggregate(type)) {
        if (operator != OP_ASSIGN) {
            compile_error(statement->line, statement->column, "'%s=' needs a number, but '%s' is %s", operator_text(operator), name, describe_shape(target->shape));
        }
        check_copy_source(checker, target->shape, statement->assignment.value, name);
        return;
    }
    bool is_shift = operator == OP_SHIFT_LEFT || operator == OP_SHIFT_RIGHT;
    if (is_shift) {
        check_expression(checker, statement->assignment.value);
    } else {
        check_store(checker, target->shape, statement->assignment.value, name);
    }
    if (operator != OP_ASSIGN && !is_numeric(type)) {
        compile_error(statement->line, statement->column, "'%s=' needs a number, but '%s' is %s",
                      operator_text(operator), name, type_with_article(type));
    }
    bool is_bitwise = operator == OP_BIT_AND || operator == OP_BIT_OR || operator == OP_BIT_XOR;
    bool needs_integers = operator == OP_REMAINDER || operator == OP_SHIFT_LEFT || operator == OP_SHIFT_RIGHT || is_bitwise;
    if (needs_integers && (!is_integer(type) || !is_integer(statement->assignment.value->type))) {
        compile_error(statement->line, statement->column, "'%s=' needs integers on both sides", operator_text(operator));
    }
}

static void check_increment(Checker *checker, Statement *statement) {
    Expression *target = statement->increment.target;
    ValueType type = check_target(checker, target);
    if (!is_numeric(type)) {
        compile_error(statement->line, statement->column, "'%s' needs a number, but '%s' is %s",
                      statement->increment.is_increment ? "++" : "--", describe_target(target),
                      is_aggregate(type) ? describe_shape(target->shape) : type_with_article(type));
    }
}

static void check_return(Checker *checker, Statement *statement) {
    Function *function = checker->function;
    Expression *value = statement->return_value;
    if (value == NULL) {
        if (function->return_type != TYPE_VOID) {
            compile_error(statement->line, statement->column, "'%s' must return %s", function->name, type_with_article(function->return_type));
        }
        return;
    }
    if (function->return_type == TYPE_VOID) {
        compile_error(statement->line, statement->column, "'%s' is void and cannot return a value", function->name);
    }
    ValueType type = check_expression(checker, value);
    store_text_as_char(function->return_shape, value);
    if (!can_store_shape(function->return_shape, value)) {
        if (function->return_shape->enumeration != NULL) {
            const char *wanted = describe_shape(function->return_shape);
            compile_error(value->line, value->column, "'%s' returns %s, not %s; convert it with (%s)", function->name, wanted,
                          value->shape != NULL && value->shape->enumeration != NULL ? describe_shape(value->shape) : type_with_article(type), wanted);
        }
        compile_error(value->line, value->column, "'%s' returns %s, not %s; convert it with (%s)", function->name,
                      type_with_article(function->return_type), type_with_article(type), type_name(function->return_type));
    }
}

static void check_for(Checker *checker, Statement *statement) {
    scopes_push(&checker->scopes);
    if (statement->for_statement.initial != NULL) {
        check_statement(checker, statement->for_statement.initial);
    }
    if (statement->for_statement.condition != NULL) {
        check_condition(checker, statement->for_statement.condition, "a condition");
    }
    if (statement->for_statement.step != NULL) {
        check_statement(checker, statement->for_statement.step);
    }
    check_loop_body(checker, statement->for_statement.body);
    scopes_pop(&checker->scopes);
}

void check_statement(Checker *checker, Statement *statement) {
    switch (statement->kind) {
        case STATEMENT_BLOCK:
            scopes_push(&checker->scopes);
            for (int index = 0; index < statement->block.count; index++) {
                check_statement_safely(checker, LIST_AT(statement->block, Statement, index));
            }
            scopes_pop(&checker->scopes);
            break;
        case STATEMENT_DECLARATION:
            check_declaration(checker, statement);
            break;
        case STATEMENT_ASSIGNMENT:
            check_assignment(checker, statement);
            break;
        case STATEMENT_INCREMENT:
            check_increment(checker, statement);
            break;
        case STATEMENT_IF:
            check_condition(checker, statement->if_statement.condition, "a condition");
            check_branch(checker, statement->if_statement.then_branch);
            if (statement->if_statement.else_branch != NULL) {
                check_branch(checker, statement->if_statement.else_branch);
            }
            break;
        case STATEMENT_WHILE:
            check_condition(checker, statement->while_statement.condition, "a condition");
            check_loop_body(checker, statement->while_statement.body);
            break;
        case STATEMENT_DO:
            check_loop_body(checker, statement->while_statement.body);
            check_condition(checker, statement->while_statement.condition, "a condition");
            break;
        case STATEMENT_SWITCH:
            check_switch(checker, statement);
            break;
        case STATEMENT_FOR:
            check_for(checker, statement);
            break;
        case STATEMENT_RETURN:
            check_return(checker, statement);
            break;
        case STATEMENT_BREAK:
            if (checker->loop_depth == 0 && checker->switch_depth == 0) {
                compile_error(statement->line, statement->column, "break is only allowed inside a loop or a switch");
            }
            break;
        case STATEMENT_CONTINUE:
            if (checker->loop_depth == 0) {
                compile_error(statement->line, statement->column, "continue is only allowed inside a loop");
            }
            break;
        case STATEMENT_CALL:
            statement->call->type = check_any_expression(checker, statement->call);
            break;
        case STATEMENT_ASM:
            break;
    }
}
