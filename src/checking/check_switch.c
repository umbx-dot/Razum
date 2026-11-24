#include "checking/checking.h"

#include <inttypes.h>

#include "common/diagnostics.h"

static void check_case_value(const Expression *value, ValueType type) {
    bool is_literal = value->kind == EXPRESSION_INT;
    bool fits = is_literal && (value->int_literal.literal_type == type || literal_fits(value, type) ||
                               (value->int_literal.literal_type == TYPE_CHAR && is_integer(type)));
    if (!fits) {
        compile_error(value->line, value->column, "a case value must be a constant that fits %s, such as 3, \"a\" or a const", type_with_article(type));
    }
}

static void check_duplicates(const PointerList *cases) {
    for (int index = 0; index < cases->count; index++) {
        const SwitchCase *switch_case = LIST_AT(*cases, SwitchCase, index);
        for (int value_index = 0; value_index < switch_case->values.count; value_index++) {
            const Expression *value = LIST_AT(switch_case->values, Expression, value_index);
            for (int other = 0; other <= index; other++) {
                const SwitchCase *earlier = LIST_AT(*cases, SwitchCase, other);
                int limit = other == index ? value_index : earlier->values.count;
                for (int earlier_index = 0; earlier_index < limit; earlier_index++) {
                    if (LIST_AT(earlier->values, Expression, earlier_index)->int_literal.value == value->int_literal.value) {
                        compile_error(value->line, value->column, "case %" PRId64 " appears twice in this switch", value->int_literal.value);
                    }
                }
            }
        }
    }
}

void check_switch(Checker *checker, Statement *statement) {
    Expression *value = statement->switch_statement.value;
    ValueType type = check_expression(checker, value);
    if (!is_integer(type)) {
        compile_error(value->line, value->column, "switch needs an integer or a char, not %s", type_with_article(type));
    }
    bool has_default = false;
    const PointerList *cases = &statement->switch_statement.cases;
    for (int index = 0; index < cases->count; index++) {
        SwitchCase *switch_case = LIST_AT(*cases, SwitchCase, index);
        if (switch_case->is_default && has_default) {
            compile_error(switch_case->line, switch_case->column, "a switch has at most one default");
        }
        has_default = has_default || switch_case->is_default;
        for (int value_index = 0; value_index < switch_case->values.count; value_index++) {
            Expression *case_value = LIST_AT(switch_case->values, Expression, value_index);
            check_any_expression(checker, case_value);
            text_as_char(case_value);
            Expression *folded = case_value->kind == EXPRESSION_INT ? NULL : fold_constant(case_value);
            if (folded != NULL && folded->kind == EXPRESSION_INT && is_integer(folded->type)) {
                folded->int_literal.from_constant = true;
                *case_value = *folded;
            }
            check_case_value(case_value, type);
            const EnumType *enumeration = value->shape != NULL ? value->shape->enumeration : NULL;
            if (enumeration != NULL && (case_value->shape == NULL || case_value->shape->enumeration != enumeration)) {
                compile_error(case_value->line, case_value->column, "a case in a switch on %s must be a %s member, such as %s.%s",
                              enumeration->name, enumeration->name, enumeration->name, LIST_AT(enumeration->members, EnumMember, 0)->name);
            }
        }
        checker->switch_depth++;
        scopes_push(&checker->scopes);
        check_statement(checker, switch_case->body);
        scopes_pop(&checker->scopes);
        checker->switch_depth--;
    }
    check_duplicates(cases);
}
