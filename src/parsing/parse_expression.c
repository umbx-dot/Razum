#include "parsing/parsing.h"

#include <inttypes.h>
#include <string.h>

#include "common/diagnostics.h"

typedef struct {
    const char *text;
    Operator operator;
} BinaryOperator;

static const BinaryOperator LEVEL_OR[] = {{"||", OP_OR}, {NULL, 0}};
static const BinaryOperator LEVEL_AND[] = {{"&&", OP_AND}, {NULL, 0}};
static const BinaryOperator LEVEL_BIT_OR[] = {{"|", OP_BIT_OR}, {NULL, 0}};
static const BinaryOperator LEVEL_BIT_XOR[] = {{"^", OP_BIT_XOR}, {NULL, 0}};
static const BinaryOperator LEVEL_BIT_AND[] = {{"&", OP_BIT_AND}, {NULL, 0}};
static const BinaryOperator LEVEL_EQUALITY[] = {{"==", OP_EQUAL}, {"!=", OP_NOT_EQUAL}, {NULL, 0}};
static const BinaryOperator LEVEL_ORDER[] = {
    {"<", OP_LESS}, {"<=", OP_LESS_EQUAL}, {">", OP_GREATER}, {">=", OP_GREATER_EQUAL}, {NULL, 0},
};
static const BinaryOperator LEVEL_SHIFT[] = {{"<<", OP_SHIFT_LEFT}, {">>", OP_SHIFT_RIGHT}, {NULL, 0}};
static const BinaryOperator LEVEL_SUM[] = {{"+", OP_ADD}, {"-", OP_SUBTRACT}, {NULL, 0}};
static const BinaryOperator LEVEL_PRODUCT[] = {
    {"*", OP_MULTIPLY}, {"/", OP_DIVIDE}, {"%", OP_REMAINDER}, {NULL, 0},
};
static const BinaryOperator *const LEVELS[] = {
    LEVEL_OR, LEVEL_AND, LEVEL_BIT_OR, LEVEL_BIT_XOR, LEVEL_BIT_AND, LEVEL_EQUALITY, LEVEL_ORDER, LEVEL_SHIFT, LEVEL_SUM, LEVEL_PRODUCT,
};
enum { LEVEL_COUNT = sizeof LEVELS / sizeof LEVELS[0] };

static Expression *parse_unary(TokenStream *stream);

static const BinaryOperator *match_operator(const TokenStream *stream, int level) {
    for (const BinaryOperator *candidate = LEVELS[level]; candidate->text != NULL; candidate++) {
        if (stream_is_operator(stream, candidate->text, 0)) {
            return candidate;
        }
    }
    return NULL;
}

static Expression *parse_binary(TokenStream *stream, int level) {
    if (level == LEVEL_COUNT) {
        return parse_unary(stream);
    }
    Expression *left = parse_binary(stream, level + 1);
    const BinaryOperator *found;
    while ((found = match_operator(stream, level)) != NULL) {
        const Token *token = stream_advance(stream);
        Expression *binary = new_expression(EXPRESSION_BINARY, token->line, token->column);
        binary->binary.operator = found->operator;
        binary->binary.left = left;
        binary->binary.right = parse_binary(stream, level + 1);
        left = binary;
    }
    return left;
}

static Expression *negate(const Token *token, Expression *operand) {
    if (operand->kind == EXPRESSION_INT && operand->int_literal.literal_type == TYPE_UNKNOWN) {
        if (operand->int_literal.too_large && (uint64_t)operand->int_literal.value != (uint64_t)INT64_MIN) {
            compile_error(token->line, token->column, "-%" PRIu64 " does not fit in a long; the smallest long is -9223372036854775808",
                          (uint64_t)operand->int_literal.value);
        }
        operand->int_literal.value = operand->int_literal.too_large ? INT64_MIN : -operand->int_literal.value;
        operand->int_literal.too_large = false;
        operand->line = token->line;
        operand->column = token->column;
        return operand;
    }
    if (operand->kind == EXPRESSION_DOUBLE) {
        operand->double_literal = -operand->double_literal;
        operand->line = token->line;
        operand->column = token->column;
        return operand;
    }
    Expression *unary = new_expression(EXPRESSION_UNARY, token->line, token->column);
    unary->unary.operator = OP_NEGATE;
    unary->unary.operand = operand;
    return unary;
}

static bool starts_cast(const TokenStream *stream) {
    if (!stream_is_operator(stream, "(", 0)) {
        return false;
    }
    const Token *next = stream_peek(stream, 1);
    if (stream_is_type(stream, 1) || (next->kind == TOKEN_KEYWORD && strcmp(next->text, "fn") == 0)) {
        return true;
    }
    if (next->kind == TOKEN_NAME && stream_is_operator(stream, ")", 2)) {
        const Token *after = stream_peek(stream, 3);
        bool is_value = after->kind == TOKEN_NAME || after->kind == TOKEN_INT || after->kind == TOKEN_DOUBLE ||
                        after->kind == TOKEN_CHAR || after->kind == TOKEN_STRING || after->kind == TOKEN_KEYWORD;
        return is_value || stream_is_operator(stream, "(", 3) || stream_is_operator(stream, "!", 3) || stream_is_operator(stream, "~", 3);
    }
    if (next->kind != TOKEN_NAME || !stream_is_operator(stream, "*", 2)) {
        return false;
    }
    int offset = 2;
    while (stream_is_operator(stream, "*", offset)) {
        offset++;
    }
    return stream_is_operator(stream, ")", offset);
}

static Expression *parse_cast(TokenStream *stream) {
    const Token *start = stream_advance(stream);
    const Token *target = stream_current(stream);
    Shape *shape = parse_shape(stream);
    if (shape->kind == TYPE_VOID) {
        compile_error(target->line, target->column, "a value cannot be converted to void");
    }
    stream_expect_operator(stream, ")");
    Expression *cast = new_expression(EXPRESSION_CAST, start->line, start->column);
    cast->cast.target = shape->kind;
    cast->cast.shape = shape;
    cast->cast.operand = parse_unary(stream);
    return cast;
}

Expression *parse_postfix(TokenStream *stream, Expression *base) {
    for (;;) {
        const Token *token = stream_current(stream);
        if (stream_is_operator(stream, "(", 0)) {
            Expression *call = new_expression(EXPRESSION_CALL, token->line, token->column);
            call->call.callee = base;
            call->call.arguments = parse_arguments(stream);
            base = call;
        } else if (stream_accept_operator(stream, "[")) {
            Expression *index = new_expression(EXPRESSION_INDEX, token->line, token->column);
            index->index.base = base;
            index->index.index = parse_expression(stream);
            stream_expect_operator(stream, "]");
            base = index;
        } else if (stream_accept_operator(stream, ".") || stream_accept_operator(stream, "->")) {
            Expression *field = new_expression(EXPRESSION_FIELD, token->line, token->column);
            field->field.base = base;
            field->field.name = stream_expect_name(stream, "a field name after '.'")->text;
            base = field;
        } else {
            return base;
        }
    }
}

static Expression *parse_primary(TokenStream *stream) {
    const Token *token = stream_current(stream);
    Expression *expression;
    switch (token->kind) {
        case TOKEN_INT:
            expression = new_expression(EXPRESSION_INT, token->line, token->column);
            expression->int_literal.too_large = token->int_value > INT64_MAX;
            expression->int_literal.value = (int64_t)token->int_value;
            break;
        case TOKEN_CHAR:
            expression = new_expression(EXPRESSION_INT, token->line, token->column);
            expression->int_literal.value = (int64_t)token->int_value;
            expression->int_literal.literal_type = TYPE_CHAR;
            break;
        case TOKEN_DOUBLE:
            expression = new_expression(EXPRESSION_DOUBLE, token->line, token->column);
            expression->double_literal = token->double_value;
            break;
        case TOKEN_STRING:
            expression = new_expression(EXPRESSION_STRING, token->line, token->column);
            expression->string.bytes = token->string_value;
            expression->string.length = token->string_length;
            break;
        case TOKEN_NAME:
            stream_advance(stream);
            if (stream_is_operator(stream, "(", 0)) {
                expression = new_expression(EXPRESSION_CALL, token->line, token->column);
                expression->call.name = token->text;
                expression->call.arguments = parse_arguments(stream);
                expression = parse_postfix(stream, expression);
            } else {
                expression = new_expression(EXPRESSION_NAME, token->line, token->column);
                expression->name.name = token->text;
                expression = parse_postfix(stream, expression);
            }
            return expression;
        default:
            if (stream_accept_keyword(stream, "null")) {
                expression = new_expression(EXPRESSION_INT, token->line, token->column);
                expression->int_literal.literal_type = TYPE_POINTER;
                return expression;
            }
            if (stream_accept_keyword(stream, "new")) {
                expression = new_expression(EXPRESSION_NEW, token->line, token->column);
                expression->allocation.element = parse_shape(stream);
                if (stream_accept_operator(stream, "[")) {
                    expression->allocation.count = parse_expression(stream);
                    stream_expect_operator(stream, "]");
                }
                return expression;
            }
            if (stream_accept_keyword(stream, "sizeof")) {
                expression = new_expression(EXPRESSION_SIZEOF, token->line, token->column);
                stream_expect_operator(stream, "(");
                expression->size_of = parse_array_suffix(stream, parse_shape(stream));
                stream_expect_operator(stream, ")");
                return expression;
            }
            if (stream_is_keyword(stream, "true") || stream_is_keyword(stream, "false")) {
                expression = new_expression(EXPRESSION_INT, token->line, token->column);
                expression->int_literal.value = stream_is_keyword(stream, "true");
                expression->int_literal.literal_type = TYPE_BOOL;
                break;
            }
            if (stream_accept_operator(stream, "(")) {
                expression = parse_expression(stream);
                stream_expect_operator(stream, ")");
                return parse_postfix(stream, expression);
            }
            compile_error(token->line, token->column, "expected a value, found %s", describe_token(token));
    }
    stream_advance(stream);
    return expression;
}

static Expression *parse_unary(TokenStream *stream) {
    const Token *token = stream_current(stream);
    if (stream_accept_operator(stream, "-")) {
        return negate(token, parse_unary(stream));
    }
    if (stream_is_operator(stream, "!", 0) || stream_is_operator(stream, "~", 0)) {
        stream_advance(stream);
        Expression *unary = new_expression(EXPRESSION_UNARY, token->line, token->column);
        unary->unary.operator = token->text[0] == '!' ? OP_NOT : OP_BIT_NOT;
        unary->unary.operand = parse_unary(stream);
        return unary;
    }
    if (stream_is_operator(stream, "&", 0) || stream_is_operator(stream, "*", 0)) {
        stream_advance(stream);
        Expression *pointer = new_expression(token->text[0] == '&' ? EXPRESSION_ADDRESS : EXPRESSION_DEREF, token->line, token->column);
        pointer->unary.operand = parse_unary(stream);
        return pointer;
    }
    if (starts_cast(stream)) {
        return parse_cast(stream);
    }
    return parse_primary(stream);
}

Expression *parse_expression(TokenStream *stream) {
    Expression *condition = parse_binary(stream, 0);
    const Token *token = stream_current(stream);
    if (!stream_accept_operator(stream, "?")) {
        return condition;
    }
    Expression *conditional = new_expression(EXPRESSION_CONDITIONAL, token->line, token->column);
    conditional->conditional.condition = condition;
    conditional->conditional.when_true = parse_expression(stream);
    stream_expect_operator(stream, ":");
    conditional->conditional.when_false = parse_expression(stream);
    return conditional;
}
