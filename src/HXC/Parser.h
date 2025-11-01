#ifndef HXLANG_PARSER_H
#define HXLANG_PARSER_H
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>
#include "Lexer.h"
struct Exp;
typedef enum Type {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_CHAR,
    TYPE_STR,
} Type;
typedef enum Operator {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_EQU,  //等于
    OP_MOVE, //赋值
    OP_SELF_INCREASE,   //自增
    OP_SELF_REDUCTION,  //自减
} Operator;
typedef struct BinExp {   //二元运算
    struct Exp* left;
    Operator op;
    struct Exp* right;
    int tokenIndex;
} BinExp;
typedef struct UnaryExp {  //一元运算
    struct Exp* value;
    Operator op;
    int tokenIndex;
} UnaryExp;
typedef struct Identifier {    //标识符
    wchar_t* name;
    int tokenIndex;
} Identifier;
typedef struct Literal {  //字面量
    union {
        int int_value;
        float float_value;
        double double_value;
        wchar_t char_value;
        wchar_t* string_value;   //要释放
    } value;
} Literal;
typedef struct FunCall {
    Identifier name;
    struct Exp* arguments;   //实参
    int argSize;
} FunCall;
typedef struct Exp {
    enum {
        LITERAL,
        BIN_EXP,
        UNARY_EXP,
        FUN_CALL,
    } type;
    union {
        Literal literal;
        BinExp binExp;
        UnaryExp unaryExp;
        FunCall funCall;
    } value
} Exp;
static int parseFun(Tokens*  src, int* index, Exp* result);
#endif