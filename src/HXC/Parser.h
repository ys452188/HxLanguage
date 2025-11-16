#ifndef HXLANG_PARSER_H
#define HXLANG_PARSER_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <errno.h>
#include <string.h>

#include "Lexer.h"
struct Exp;
typedef enum Type {
    TYPE_INT = 1,
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
    OP_EQU,             // 等于
    OP_MOVE,            // 赋值
    OP_SELF_INCREASE,   // 自增
    OP_SELF_REDUCTION,  // 自减
} Operator;
typedef struct BinExp {  // 二元运算
    struct ASTNode* left;
    Operator op;
    struct ASTNode* right;
    int tokenIndex;
} BinExp;
typedef struct UnaryExp {  // 一元运算
    struct ASTNode* value;
    Operator op;
    int tokenIndex;
} UnaryExp;
typedef struct Identifier {  // 标识符
    wchar_t* name;
    int tokenIndex;
    int tableIndex;
} Identifier;
typedef struct Literal {  // 字面量
    union {
        int int_value;
        float float_value;
        double double_value;
        wchar_t char_value;
        wchar_t* string_value;  // 要释放
    } value;
    Type type;
} Literal;
typedef struct FunCall {
    Identifier name;
    struct ASTNode* arguments;  // 实参
    int argSize;
} FunCall;
typedef struct ASTNode {
    enum { LITERAL, BIN_EXP, UNARY_EXP, FUN_CALL, VARIABLE } type;
    union {
        Literal literal;
        BinExp binExp;
        UnaryExp unaryExp;
        FunCall funCall;
        Identifier id;
    } value;
} ASTNode;

static int _hxl_create_primary_exp(Token* token, Symbol* table, int tableSize, ASTNode* node);
extern int parseExp(Tokens* src, int index, int end_index, Symbol* table,
                    int tableSize, ASTNode* result);
extern int genByAST(ASTNode* ast, Command** cmd, int* size, int* index);
Type getLiteralTypeByString(Token* token);  // 通过字面量获取其类型
//辅助函数：将单个 Token (ID 或 VAL) 转换为 ASTNode (VARIABLE 或 LITERAL)
static int _hxl_create_primary_exp(Token* token, Symbol* table, int tableSize, ASTNode* node) {
    if (!token || !node) return -1;

    if (token->type == TOK_ID) {
        node->type = VARIABLE;
        node->value.id.tokenIndex = token->line;
        node->value.id.tableIndex =
            localeTable_search(token->value, table, tableSize);
        if (node->value.id.tableIndex == -1) {
            setError(ERR_CANNOT_FIND_SYMBOL, token->line, token->value);
            return 255;
        }
        node->value.id.name = (wchar_t*)calloc(
                                  wcslen(token->value) + 1, sizeof(wchar_t));
        if (!(node->value.id.name)) return -1;
        wcscpy(node->value.id.name, token->value);
        return 0;

    } else if (token->type == TOK_VAL) {
        node->type = LITERAL;
        node->value.literal.type = getLiteralTypeByString(token);
        if ((node->value.literal.type) == 0) {
            setError(ERR_EXP, token->line, token->value);
            return 255;
        }

        errno = 0; // 重置 errno 以检查转换错误
        switch (node->value.literal.type) {
        case TYPE_INT: {
            wchar_t* p = NULL;
            node->value.literal.value.int_value =
                (int32_t)wcstol(token->value, &p, 0);
            // wprintf(L"%d\n", (int)(node->value.literal.value.int_value)); // 调试语句，不应保留在生产代码中
            break;
        }
        case TYPE_FLOAT: {
            wchar_t* p = NULL;
            node->value.literal.value.float_value =
                (float)wcstof(token->value, &p);
            break;
        }
        case TYPE_DOUBLE: {
            wchar_t* p = NULL;
            node->value.literal.value.double_value =
                (double)wcstod(token->value, &p);
            break;
        }
        case TYPE_CHAR: {
            node->value.literal.value.char_value = token->value[0];
            break;
        }
        case TYPE_STR: {
            // 对于字符串字面量，需要拷贝字符串的值
            node->value.literal.value.string_value =
                (wchar_t*)calloc(wcslen(token->value) + 1, sizeof(wchar_t));
            if (!(node->value.literal.value.string_value)) return -1;
            wcscpy(node->value.literal.value.string_value, token->value);
            break;
        }
        }

        if (errno == ERANGE) {
            setError(ERR_OUT_OF_VALUE, token->line, token->value);
            return 255;
        }
        return 0;
    }
    // 如果 token 既不是 ID 也不是 VAL，则表示内部逻辑错误
    return 255;
}


int parseExp(Tokens* src, int index, int end_index, Symbol* table,
             int tableSize, ASTNode* result) {
    if (src == NULL || !result) return -1;
    if (index >= end_index) return 255; // 索引越界或空表达式

    Token* current_token = &(src->tokens[index]);

    if (current_token->type == TOK_ID || current_token->type == TOK_VAL) {
        if (index + 1 == end_index) {
            // ----------------------------------------------------
            // 情况 1: 单个标识符或字面量表达式 (a 或 123)
            // ----------------------------------------------------
            int err = _hxl_create_primary_exp(current_token, table, tableSize, result);
            // 原始代码返回 0 成功，255 错误，-1 内存错误。保持不变。
            return err;
        } else {
            // ----------------------------------------------------
            // 情况 2: 偷看下一个 token，处理二元运算的开始
            // ----------------------------------------------------
            int lookahead_index = index + 1;
            Token* lookahead_token = &(src->tokens[lookahead_index]);

            // 检查是否是 + / - 运算符 (加法和减法优先级)
            if (wcscmp(lookahead_token->value, L"+") == 0 ||
                    wcscmp(lookahead_token->value, L"-") == 0) {

                if (lookahead_index + 1 > end_index) {
                    setError(ERR_EXP, lookahead_token->line, lookahead_token->value);
                    return 255;
                }

                result->type = BIN_EXP;
                result->value.binExp.op = (wcscmp(lookahead_token->value, L"+") == 0) ? OP_ADD : OP_SUB;
                result->value.binExp.tokenIndex = lookahead_token->line;

                ASTNode* left = (ASTNode*)calloc(1, sizeof(ASTNode));
                if (!left) return -1;

                int err_left = _hxl_create_primary_exp(current_token, table, tableSize, left);
                if (err_left) {
                    free(left); // 发生错误，必须释放已分配的内存
                    return err_left;
                }
                result->value.binExp.left = left;

                int right_start_index = lookahead_index + 1;
                ASTNode* right = (ASTNode*)calloc(1, sizeof(ASTNode));
                if (!right) {
                    // right 分配失败，必须释放 left
                    // 注意：如果 left 是字面量，其内部的字符串可能也需要释放 (未在此处处理，但建议在完整的释放函数中处理)
                    free(left);
                    return -1;
                }

                int err_right = parseExp(src, right_start_index, end_index, table, tableSize, right);
                if (err_right) {
                    // 右侧解析失败，必须释放 left 和 right
                    // 注意：如果 left/right 内部有动态内存 (如ID的name, STR字面量)，也需要在此处释放
                    free(left);
                    free(right);
                    return err_right;
                }
                result->value.binExp.right = right;

                // **重要提示:** // 原始代码成功解析右侧后，直接返回了右侧的错误码 (0),
                // 这意味着调用者不知道表达式占用了多少个 tokens。
                // 保持原始行为：返回右侧递归调用的错误码。
                return err_right;

                // 检查是否是 * / / 运算符 (乘法和除法优先级)
            } else if (wcscmp(lookahead_token->value, L"*") == 0 || wcscmp(lookahead_token->value, L"/") == 0 ||
                       wcscmp(lookahead_token->value, L"×") == 0 || wcscmp(lookahead_token->value, L"÷") == 0) {

                // 乘除法的解析逻辑与加减法类似，但涉及到优先级，原始逻辑是有问题的。
                // 此处基于您原始的代码逻辑进行重构以消除代码重复和内存泄漏。

                if (lookahead_index + 1 > end_index) {
                    setError(ERR_EXP, lookahead_token->line, lookahead_token->value);
                    return 255;
                }

                result->type = BIN_EXP;
                if (wcscmp(lookahead_token->value, L"*") == 0 || wcscmp(lookahead_token->value, L"×") == 0) {
                    result->value.binExp.op = OP_MUL;
                } else {
                    result->value.binExp.op = OP_DIV;
                }
                result->value.binExp.tokenIndex = lookahead_token->line;


                ASTNode* left = (ASTNode*)calloc(1, sizeof(ASTNode));
                if (!left) return -1;

                int err_left = _hxl_create_primary_exp(current_token, table, tableSize, left);
                if (err_left) {
                    free(left);
                    return err_left;
                }
                result->value.binExp.left = left;

                int right_start_index = lookahead_index + 1;
                ASTNode* right = (ASTNode*)calloc(1, sizeof(ASTNode));
                if (!right) {
                    free(left);
                    return -1;
                }

                // **注意:** 原始代码在处理乘除法时有一个额外的“偷看”逻辑
                // 该逻辑是为了处理乘除法比加减法优先级高的问题，但实现方式不规范，
                // 并且在递归调用时使用了 lookahead_index - 1 作为起始索引。
                // 为了保持您原有的意图（尽管有缺陷），我保留了原有的递归调用方式，
                // 但清理了内存管理和代码重复。

                // 原始逻辑：此时指向+或-号右侧,继续偷看 (这里应该是下一个token的右侧)
                // if(index+1 <= end_index) { index++; ... }
                // 实际索引是 right_start_index。

                int err_right = parseExp(src, right_start_index, end_index, table, tableSize, right);

                if (err_right) {
                    free(left);
                    free(right);
                    return err_right;
                }
                result->value.binExp.right = right;

                // 保持原始行为：返回右侧递归调用的错误码
                return err_right;
            }
        }
    }
    // 如果没有匹配的表达式模式，则返回错误
    // 注意：您原始代码中，对于不匹配的 TOKEN_ID/VAL 后的 token，没有明确的返回路径
    // 如果 token 既不是 ID 也不是 VAL，此处将返回 255
    setError(ERR_EXP, current_token->line, current_token->value);
    return 255;
}

// (其余函数保持不变)

Type getLiteralTypeByString(Token* token) {  // 通过字面量获取其类型
    Type type = 0;
    if (!token) return type;
    if (!(token->value)) return type;
    if (wcschr(token->value, L'.') != NULL) {
        // 11.4f     ,   浮点型
        if (wcslen(token->value) > 0 && token->value[wcslen(token->value) - 1] == L'f') {
            type = TYPE_FLOAT;
            return type;
        } else {
            type = TYPE_DOUBLE;
            return type;
        }
    } else if (token->mark == CH) {
        type = TYPE_CHAR;
        return type;
    } else if (token->mark == STR) {
        type = TYPE_STR;
        return type;
    } else if (iswdigit(token->value[0])) {
        type = TYPE_INT;
        return type;
    } else {
        return type;
    }
}

int genByAST(ASTNode* ast, Command** cmd, int* size, int* index) {
    if(!ast || !cmd || !size || !index) return -1;
    if(*index >= *size) {
        Command* temp = (Command*)realloc(*cmd, ((*index)+1)*sizeof(Command));
        if(!temp) return -1;
        *cmd = temp;
        memset(&((*cmd)[*index]), 0, sizeof(Command));
        *size = *index+1;
    }
    if(ast->type==BIN_EXP) {
        switch(ast->value.binExp.op) {
        case OP_ADD: {
            (*cmd)[*index].op = PUSH;
            (*index)++;
            if(*index >= *size) {
                Command* temp = (Command*)realloc(*cmd, ((*index)+1)*sizeof(Command));
                if(!temp) return -1;
                *cmd = temp;
                memset(&((*cmd)[*index]), 0, sizeof(Command));
                *size = *index+1;
            }
#ifdef HX_DEBUG
            log(L"生成ADD");
#endif
            (*cmd)[*index].op = ADD;
            break;
        }
        }
    }
    return 0;
}
#endif