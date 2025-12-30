#ifndef HXLANG_PARSER_H
#define HXLANG_PARSER_H
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "Lexer.h"
// 结构体定义 (与你原文件一致)
struct ASTNode;
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

extern int parseExp(Tokens* src, int index, int end_index, Symbol* table,
                    int tableSize, ASTNode* result);
extern int genByAST(ASTNode* ast, Command** cmd, int* size, int* index);
Type getLiteralTypeByString(Token* token);  // 通过字面量获取其类型
// 辅助函数：释放AST
void freeAST(ASTNode* node) {
    if (!node) return;
    switch (node->type) {
    case LITERAL:
        if (node->value.literal.type == TYPE_STR &&
                node->value.literal.value.string_value) {
            free(node->value.literal.value.string_value);
        }
        break;
    case BIN_EXP:
        freeAST(node->value.binExp.left);
        freeAST(node->value.binExp.right);
        break;
    case UNARY_EXP:
        freeAST(node->value.unaryExp.value);
        break;
    case VARIABLE:
        if (node->value.id.name) {
            free(node->value.id.name);
        }
        break;
    case FUN_CALL:
        // (暂未实现)
        break;
    }
    free(node);
}
// 辅助函数：创建基本表达式
static int _hxl_create_primary_exp(Token* token, Symbol* table, int tableSize,
                                   ASTNode* node) {
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
        node->value.id.name =
            (wchar_t*)calloc(wcslen(token->value) + 1, sizeof(wchar_t));
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

        errno = 0;  // 重置 errno 以检查转换错误
        switch (node->value.literal.type) {
        case TYPE_INT: {
            wchar_t* p = NULL;
            node->value.literal.value.int_value =
                (int32_t)wcstol(token->value, &p, 0);
            break;
        }
        case TYPE_FLOAT: {
            wchar_t* p = NULL;
            node->value.literal.value.float_value = (float)wcstof(token->value, &p);
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
// 表达式解析器
// 需要前向声明，因为 parse_primary 会调用 parse_additive (用于括号)
static int parse_additive(Tokens* src, int* index, int end_index, Symbol* table,
                          int tableSize, ASTNode** result_node);
// 解析一个 "基本" 表达式 (字面量, 变量, 或括号)
static int parse_primary(Tokens* src, int* index, int end_index, Symbol* table,
                         int tableSize, ASTNode** result_node) {
    if (*index >= end_index) {
        setError(ERR_EXP, (end_index > 0 ? src->tokens[end_index - 1].line : 0),
                 L"预期应有表达式");
        return 255;
    }
    Token* token = &(src->tokens[*index]);
    // 1. 处理括号 ( ... )
    if (wcscmp(token->value, L"(") == 0) {
        (*index)++;  // 消耗 '('
        int err =
            parse_additive(src, index, end_index, table, tableSize, result_node);
        if (err) return err;  // 递归解析括号内的表达式

        if (*index >= end_index || (wcscmp(src->tokens[*index].value, L")") != 0)) {
            setError(ERR_EXP, token->line, L"括号不匹配，缺少 ')'");
            freeAST(*result_node);  // 释放已解析的子树
            *result_node = NULL;
            return 255;
        }
        (*index)++;  // 消耗 ')'
        return 0;
    }
    // 2. 处理字面量或变量
    if (token->type == TOK_ID || token->type == TOK_VAL) {
        *result_node = (ASTNode*)calloc(1, sizeof(ASTNode));
        if (!*result_node) return -1;  // 内存错误

        int err = _hxl_create_primary_exp(token, table, tableSize, *result_node);
        if (err) {
            free(*result_node);  // 注意：这里只 free 节点本身，因为
            // _hxl_create_primary_exp 失败时不会分配内部数据
            *result_node = NULL;
            return err;
        }
        (*index)++;  // 消耗 token
        return 0;    // 成功
    }
    // 3. 错误
    setError(ERR_EXP, token->line, token->value);
    return 255;
}

/**
 *解析乘法/除法 (高优先级)
 * multiplicative ::= primary ( ( '*' | '/' ) primary )*
 */
static int parse_multiplicative(Tokens* src, int* index, int end_index,
                                Symbol* table, int tableSize,
                                ASTNode** result_node) {
    int err = parse_primary(src, index, end_index, table, tableSize, result_node);
    if (err) return err;

    while (*index < end_index) {
        Token* op_token = &(src->tokens[*index]);
        Operator op;

        if (wcscmp(op_token->value, L"*") == 0 ||
                wcscmp(op_token->value, L"×") == 0) {
            op = OP_MUL;
        } else if (wcscmp(op_token->value, L"/") == 0 ||
                   wcscmp(op_token->value, L"÷") == 0) {
            op = OP_DIV;
        } else {
            break;  // 不是乘除法运算符，退出循环
        }
        (*index)++;  // 消耗 '*, /'

        ASTNode* right_node = NULL;
        err = parse_primary(src, index, end_index, table, tableSize, &right_node);
        if (err) {
            freeAST(*result_node);  // 释放左侧子树
            return err;
        }

        // 创建新的二元表达式节点
        ASTNode* new_left_node = (ASTNode*)calloc(1, sizeof(ASTNode));
        if (!new_left_node) {
            freeAST(*result_node);
            freeAST(right_node);
            return -1;
        }

        new_left_node->type = BIN_EXP;
        new_left_node->value.binExp.op = op;
        new_left_node->value.binExp.tokenIndex = op_token->line;
        new_left_node->value.binExp.left =
            *result_node;  // 旧的左侧节点成为新节点的左子节点
        new_left_node->value.binExp.right = right_node;

        *result_node = new_left_node;  // 新节点成为新的 "左侧"
    }
    return 0;  // 成功
}

/**
 *   解析加法/减法 (低优先级)
 * additive ::= multiplicative ( ( '+' | '-' ) multiplicative )*
 */
static int parse_additive(Tokens* src, int* index, int end_index, Symbol* table,
                          int tableSize, ASTNode** result_node) {
    // 1. 首先解析一个高优先级的 "乘法表达式"
    int err = parse_multiplicative(src, index, end_index, table, tableSize,
                                   result_node);
    if (err) return err;
    // 2. 循环查找 '+' 或 '-'
    while (*index < end_index) {
        Token* op_token = &(src->tokens[*index]);
        Operator op;

        if (wcscmp(op_token->value, L"+") == 0) {
            op = OP_ADD;
        } else if (wcscmp(op_token->value, L"-") == 0) {
            op = OP_SUB;
        } else {
            break;  // 不是加减法运算符，退出循环
        }
        (*index)++;  // 消耗 '+, -'

        ASTNode* right_node = NULL;
        // 关键：右侧操作数必须是 "乘法表达式" 级别，以保证优先级
        err = parse_multiplicative(src, index, end_index, table, tableSize,
                                   &right_node);
        if (err) {
            freeAST(*result_node);  // 释放左侧子树
            return err;
        }

        // 创建新的二元表达式节点
        ASTNode* new_left_node = (ASTNode*)calloc(1, sizeof(ASTNode));
        if (!new_left_node) {
            freeAST(*result_node);
            freeAST(right_node);
            return -1;
        }

        new_left_node->type = BIN_EXP;
        new_left_node->value.binExp.op = op;
        new_left_node->value.binExp.tokenIndex = op_token->line;
        new_left_node->value.binExp.left = *result_node;
        new_left_node->value.binExp.right = right_node;

        *result_node = new_left_node;
    }
    return 0;  // 成功
}
/*-----表达式解析的入口函数----*/
int parseExp(Tokens* src, int index, int end_index, Symbol* table,
             int tableSize, ASTNode* result) {
    if (!src || !result) return -1;
    if (index >= end_index) {
        setError(ERR_EXP, (index > 0 ? src->tokens[index - 1].line : 0),
                 L"空表达式");
        return 255;
    }
    int current_index = index;
    ASTNode* temp_result = NULL;
    // 1. 调用解析器
    int err = parse_additive(src, &current_index, end_index, table, tableSize,
                             &temp_result);
    if (err) {
        // freeAST(temp_result); // parse_additive 失败时会自行清理
        return err;
    }
    // 2. 检查是否解析了所有 tokens
    if (current_index != end_index) {
        // 表达式解析提前结束，说明有语法错误 (例如 "a b" 或 "a + * b")
        setError(ERR_EXP, src->tokens[current_index].line,
                 src->tokens[current_index].value);
        freeAST(temp_result);
        return 255;
    }
    // 3. 成功 - 将结果复制到 'result' 指针中
    memcpy(result, temp_result, sizeof(ASTNode));
    free(temp_result);  // 释放临时包装节点 (内部指针已被复制)
    return 0;           // 成功
}
Type getLiteralTypeByString(Token* token) {  // 通过字面量获取其类型
    Type type = 0;
    if (!token) return type;
    if (!(token->value)) return type;
    if (wcschr(token->value, L'.') != NULL) {
        // 11.4f     ,   浮点型
        if (wcslen(token->value) > 0 &&
                token->value[wcslen(token->value) - 1] == L'f') {
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
    if (!ast || !cmd || !size || !index) return -1;
    if(!(*cmd)) return -1;
    // (处理当前节点)
    if (ast->type == BIN_EXP) {
        // 细节：右
        int err = genByAST(ast->value.binExp.left, cmd, size, index);
        if (err) return err;
        int rightErr = genByAST(ast->value.binExp.right, cmd, size, index);
        if (rightErr) return rightErr;

        if (*index >= *size) {
            Command* temp = (Command*)realloc(*cmd, ((*index) + 1) * sizeof(Command));
            if (!temp) return -1;
            *cmd = temp;
            memset(&((*cmd)[*index]), 0, sizeof(Command));
            *size = *index + 1;
        }

        switch (ast->value.binExp.op) {
        case OP_ADD: {
#ifdef HX_DEBUG
            log(L"生成ADD");
#endif
            (*cmd)[*index].op = ADD;
            break;
        }
        case OP_SUB: {
#ifdef HX_DEBUG
            log(L"生成SUB");
#endif
            (*cmd)[*index].op = SUB;
            break;
        }
        case OP_MUL: {
#ifdef HX_DEBUG
            log(L"生成MUL");
#endif
            (*cmd)[*index].op = MUL;
            break;
        }
        case OP_DIV: {
#ifdef HX_DEBUG
            log(L"生成DIV");
#endif
            (*cmd)[*index].op = DIV;
            break;
        }
        default:
            break;
        }
        (*index)++;
    } else if (ast->type == LITERAL) {
        if (*index >= *size) {
            Command* temp = (Command*)realloc(*cmd, ((*index) + 1) * sizeof(Command));
            if (!temp) return -1;
            *cmd = temp;
            memset(&((*cmd)[*index]), 0, sizeof(Command));
            *size = *index + 1;
        }

        (*cmd)[*index].op = PUSH;
        (*cmd)[*index].args[0].type = DATA;
        switch (ast->value.literal.type) {
        case TYPE_INT: {
            (*cmd)[*index].args[0].size = sizeof(int32_t);
            (*cmd)[*index].args[0].value.data = calloc(1, sizeof(int32_t));
            if (!((*cmd)[*index].args[0].value.data)) return -1;
            *((int32_t*)((*cmd)[*index].args[0].value.data)) =
                (int32_t)(ast->value.literal.value.int_value);
#ifdef HX_DEBUG
            fwprintf(logStream, L"\33[33m[DEB]\33[0m生成PUSH %d(size:%d)\n",
                     *((int32_t*)((*cmd)[*index].args[0].value.data)),
                     (int)(*cmd)[*index].args[0].size);
#endif
            break;
        }
        case TYPE_FLOAT: {
            (*cmd)[*index].args[0].size = sizeof(float);
            (*cmd)[*index].args[0].value.data = calloc(1, sizeof(float));
            if (!((*cmd)[*index].args[0].value.data)) return -1;
            *((float*)((*cmd)[*index].args[0].value.data)) =
                (float)(ast->value.literal.value.float_value);
#ifdef HX_DEBUG
            fwprintf(logStream, L"\33[33m[DEB]\33[0m生成PUSH %f(size:%d)\n",
                     *((float*)((*cmd)[*index].args[0].value.data)),
                     (int)(*cmd)[*index].args[0].size);
#endif
            break;
        }
        case TYPE_DOUBLE: {
            (*cmd)[*index].args[0].size = sizeof(double);
            (*cmd)[*index].args[0].value.data = calloc(1, sizeof(double));
            if (!((*cmd)[*index].args[0].value.data)) return -1;
            *((double*)((*cmd)[*index].args[0].value.data)) =
                (double)(ast->value.literal.value.double_value);
#ifdef HX_DEBUG
            fwprintf(logStream, L"\33[33m[DEB]\33[0m生成PUSH %lf(size:%d)\n",
                     *((double*)((*cmd)[*index].args[0].value.data)),
                     (int)(*cmd)[*index].args[0].size);
#endif
            break;
        }
        case TYPE_CHAR: {
            (*cmd)[*index].args[0].size = sizeof(uint16_t);
            (*cmd)[*index].args[0].value.data = calloc(1, sizeof(uint16_t));
            if (!((*cmd)[*index].args[0].value.data)) return -1;
            *((uint16_t*)((*cmd)[*index].args[0].value.data)) =
                (uint16_t)(ast->value.literal.value.char_value);
#ifdef HX_DEBUG
            fwprintf(logStream, L"\33[33m[DEB]\33[0m生成PUSH %lc(size:%d)\n",
                     (wchar_t) * ((uint16_t*)((*cmd)[*index].args[0].value.data)),
                     (int)(*cmd)[*index].args[0].size);
#endif
            break;
        }
        }
        (*index)++;
    } else if (ast->type == VARIABLE) {
        if (*index >= *size) {
            Command* temp = (Command*)realloc(*cmd, ((*index) + 1) * sizeof(Command));
            if (!temp) return -1;
            *cmd = temp;
            memset(&((*cmd)[*index]), 0, sizeof(Command));
            *size = *index + 1;
        }
        // ... (待实现：生成 PUSH <variable_address> 或 LOAD 指令)
        (*index)++;
    }
    return 0;
}
#endif