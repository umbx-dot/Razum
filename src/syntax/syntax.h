#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/list.h"
#include "common/target.h"

typedef enum {
    TYPE_UNKNOWN,
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_BYTE,
    TYPE_SBYTE,
    TYPE_SHORT,
    TYPE_USHORT,
    TYPE_INT,
    TYPE_UINT,
    TYPE_LONG,
    TYPE_ULONG,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_POINTER,
    TYPE_FUNCTION,
    TYPE_ARRAY,
    TYPE_STRUCT,
    TYPE_COUNT,
} ValueType;

typedef enum {
    KIND_NONE,
    KIND_BOOL,
    KIND_INTEGER,
    KIND_FLOATING,
    KIND_STRING,
    KIND_POINTER,
    KIND_AGGREGATE,
} TypeKind;

typedef struct {
    const char *name;
    const char *with_article;
    int size;
    TypeKind kind;
    bool is_signed;
} TypeInfo;

typedef enum {
    OP_ASSIGN,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_REMAINDER,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_AND,
    OP_OR,
    OP_NEGATE,
    OP_NOT,
    OP_BIT_AND,
    OP_BIT_OR,
    OP_BIT_XOR,
    OP_SHIFT_LEFT,
    OP_SHIFT_RIGHT,
    OP_BIT_NOT,
} Operator;

typedef struct StructType StructType;
typedef struct Shape Shape;
typedef struct EnumType EnumType;
struct Expression;
struct GlobalVariable;

struct Shape {
    ValueType kind;
    const Shape *element;
    int count;
    const char *struct_name;
    StructType *structure;
    PointerList parameters;
    const Shape *result;
    const EnumType *enumeration;
    struct Expression *count_expression;
    int line;
    int column;
};

typedef struct {
    const char *name;
    int64_t value;
    int line;
    int column;
} EnumMember;

struct EnumType {
    const char *name;
    PointerList members;
    Shape *shape;
    int line;
    int column;
    const char *source_path;
};

typedef struct {
    const char *name;
    Shape *shape;
    int offset;
    int line;
    int column;
} Field;

typedef enum {
    LAYOUT_PENDING,
    LAYOUT_RUNNING,
    LAYOUT_DONE,
} LayoutState;

struct StructType {
    const char *name;
    PointerList fields;
    int size;
    int alignment;
    int line;
    int column;
    const char *source_path;
    LayoutState layout;
};

typedef enum {
    SYMBOL_GLOBAL,
    SYMBOL_LOCAL,
    SYMBOL_PARAMETER,
} SymbolKind;

typedef struct {
    SymbolKind kind;
    ValueType type;
    const Shape *shape;
    const char *label;
    const char *length_label;
    int index;
    int size;
    bool is_reference;
    bool is_used;
    bool address_taken;
    bool is_const;
    const struct Expression *constant;
    struct GlobalVariable *global;
    const char *register_name;
    uint64_t loop_uses;
    const char *name;
    int line;
    int column;
} Symbol;

typedef struct Function Function;

typedef enum {
    EXPRESSION_INT,
    EXPRESSION_DOUBLE,
    EXPRESSION_STRING,
    EXPRESSION_NAME,
    EXPRESSION_CALL,
    EXPRESSION_UNARY,
    EXPRESSION_BINARY,
    EXPRESSION_CAST,
    EXPRESSION_INDEX,
    EXPRESSION_FIELD,
    EXPRESSION_LIST,
    EXPRESSION_CONDITIONAL,
    EXPRESSION_DEREF,
    EXPRESSION_ADDRESS,
    EXPRESSION_NEW,
    EXPRESSION_SIZEOF,
    EXPRESSION_FUNCTION,
} ExpressionKind;

typedef struct Expression Expression;

struct Expression {
    ExpressionKind kind;
    int line;
    int column;
    ValueType type;
    const Shape *shape;
    union {
        struct {
            int64_t value;
            bool too_large;
            ValueType literal_type;
            bool from_constant;
        } int_literal;
        double double_literal;
        struct {
            const char *bytes;
            size_t length;
        } string;
        struct {
            const char *name;
            Symbol *symbol;
        } name;
        struct {
            const char *name;
            PointerList arguments;
            Function *target;
            bool is_free;
            Expression *callee;
        } call;
        struct {
            Operator operator;
            Expression *operand;
        } unary;
        struct {
            Operator operator;
            ValueType operation;
            Expression *left;
            Expression *right;
        } binary;
        struct {
            ValueType target;
            Expression *operand;
            Shape *shape;
        } cast;
        struct {
            Shape *element;
            Expression *count;
        } allocation;
        Shape *size_of;
        struct {
            Expression *base;
            Expression *index;
        } index;
        struct {
            Expression *base;
            const char *name;
            const Field *field;
            bool is_length;
        } field;
        PointerList list;
        Function *function_value;
        struct {
            Expression *condition;
            Expression *when_true;
            Expression *when_false;
        } conditional;
    };
};

typedef enum {
    STATEMENT_BLOCK,
    STATEMENT_DECLARATION,
    STATEMENT_ASSIGNMENT,
    STATEMENT_INCREMENT,
    STATEMENT_IF,
    STATEMENT_WHILE,
    STATEMENT_FOR,
    STATEMENT_RETURN,
    STATEMENT_BREAK,
    STATEMENT_CONTINUE,
    STATEMENT_CALL,
    STATEMENT_ASM,
    STATEMENT_DO,
    STATEMENT_SWITCH,
} StatementKind;

typedef struct {
    PointerList values;
    struct Statement *body;
    bool is_default;
    int line;
    int column;
} SwitchCase;

typedef struct Statement Statement;

struct Statement {
    StatementKind kind;
    int line;
    int column;
    union {
        PointerList block;
        struct {
            ValueType type;
            Shape *shape;
            const char *name;
            Expression *initial;
            Symbol *symbol;
            bool is_const;
        } declaration;
        struct {
            Expression *target;
            Operator operator;
            Expression *value;
        } assignment;
        struct {
            Expression *target;
            bool is_increment;
        } increment;
        struct {
            Expression *condition;
            Statement *then_branch;
            Statement *else_branch;
        } if_statement;
        struct {
            Expression *condition;
            Statement *body;
        } while_statement;
        struct {
            Statement *initial;
            Expression *condition;
            Statement *step;
            Statement *body;
        } for_statement;
        struct {
            Expression *value;
            PointerList cases;
        } switch_statement;
        Expression *return_value;
        Expression *call;
        const char *asm_text;
    };
};

typedef struct {
    ValueType type;
    Shape *shape;
    const char *name;
    int line;
    int column;
    Symbol *symbol;
} Parameter;

struct Function {
    ValueType return_type;
    Shape *return_shape;
    const char *name;
    PointerList parameters;
    Statement *body;
    int line;
    int column;
    PointerList locals;
    PointerList declared;
    int end_line;
    const char *source_path;
    const char *source_text;
    bool is_library;
    bool is_used;
    const char *extern_dll;
    bool is_exported;
    bool address_taken;
    bool is_prototype;
};

typedef struct GlobalVariable {
    ValueType type;
    Shape *shape;
    const char *name;
    Expression *initial;
    int line;
    int column;
    Symbol *symbol;
    const char *source_path;
    int string_index;
    bool is_library;
    bool is_const;
    int constant_state;
} GlobalVariable;

typedef struct {
    const char *name;
    int line;
    int column;
    const char *source_path;
} Import;

typedef struct {
    PointerList imports;
    PointerList structs;
    PointerList enums;
    PointerList globals;
    PointerList constants;
    PointerList functions;
    PointerList prototypes;
    const char *main_path;
    bool is_window_program;
    bool optimize;
    bool zen3;
    Target target;
} Program;

Expression *new_expression(ExpressionKind kind, int line, int column);
Statement *new_statement(StatementKind kind, int line, int column);
Shape *new_shape(ValueType kind, int line, int column);
int shape_size(const Shape *shape);
int shape_alignment(const Shape *shape);
bool same_shape(const Shape *left, const Shape *right);
const char *describe_shape(const Shape *shape);
bool is_aggregate(ValueType type);
const TypeInfo *type_info(ValueType type);
ValueType type_from_name(const char *name);
const char *type_name(ValueType type);
const char *type_with_article(ValueType type);
bool is_integer(ValueType type);
bool is_floating(ValueType type);
bool is_numeric(ValueType type);
bool widens(ValueType source, ValueType target);
const char *operator_text(Operator operator);
