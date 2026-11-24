#include "syntax/syntax.h"

const char *operator_text(Operator operator) {
    static const char *const TEXT[] = {
        [OP_ASSIGN] = "=", [OP_ADD] = "+", [OP_SUBTRACT] = "-", [OP_MULTIPLY] = "*",
        [OP_DIVIDE] = "/", [OP_REMAINDER] = "%", [OP_EQUAL] = "==", [OP_NOT_EQUAL] = "!=",
        [OP_LESS] = "<", [OP_LESS_EQUAL] = "<=", [OP_GREATER] = ">", [OP_GREATER_EQUAL] = ">=",
        [OP_AND] = "&&", [OP_OR] = "||", [OP_NEGATE] = "-", [OP_NOT] = "!",
        [OP_BIT_AND] = "&", [OP_BIT_OR] = "|", [OP_BIT_XOR] = "^", [OP_SHIFT_LEFT] = "<<",
        [OP_SHIFT_RIGHT] = ">>", [OP_BIT_NOT] = "~",
    };
    return TEXT[operator];
}
