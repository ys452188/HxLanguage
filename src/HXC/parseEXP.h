#ifndef HXLANG_PARSE_EXP_H
#define HXLANG_PARSE_EXP_H

#include <stdlib.h>
#include "compiler.h"
#include <stdio.h>
#include <string.h>

// AST节点类型枚举
typedef enum {
    AST_NODE_LITERAL,    // 字面量 (数字, 字符串等)
    AST_NODE_VARIABLE,   // 变量
    AST_NODE_BINARY_OP   // 二元运算
} AstNodeType;
// 运算符类型
typedef enum {
    ADD, // +
    SUB, // -
    MUL, // *
    DIV, // /
} OperatorType;

typedef struct HxASTNode HxASTNode;
// 字面量
typedef struct {
    ResultType type; // 值的类型 (RESULT_TYPE_INT, etc.)
    void* value;     // 指向实际值的指针 (由 allocOpValueByType 分配)
} LiteralNodeData;
// 变量节点
typedef struct {
    wchar_t* name; // 变量名
} VariableNodeData;
// 二元运算节点的数据
typedef struct {
    OperatorType op;
    HxASTNode* left;
    HxASTNode* right;
} BinaryOpNodeData;


struct HxASTNode {
    AstNodeType node_type; // 节点的类型标签
    Token* token;          // 指向原始Token，便于报错时定位行号

    union {
        LiteralNodeData  literal;
        VariableNodeData variable;
        BinaryOpNodeData binary_op;
    } data;
};

#ifdef HX_DEBUG
static void _showAST(HxASTNode* AST, int depth) {
    if (!AST) {
        return;
    }
    // 打印缩进
    for (int i = 0; i < depth; i++) {
        printf(" ");
    }
    if (AST->node_type == AST_NODE_BINARY_OP) {
        printf("运算符节点 %ls：\n", AST->token->value);
        // 打印左子树
        for (int i = 0; i < depth + 1; i++) printf(" ");
        printf("左节点：");
        _showAST(AST->data.binary_op.left, depth + 2);
        // 打印右子树
        for (int i = 0; i < depth + 1; i++) printf(" ");
        printf("右节点：");
        _showAST(AST->data.binary_op.right, depth + 2);

    } else if (AST->node_type == AST_NODE_LITERAL) {
        switch (AST->data.literal.type.type) {
        case RESULT_TYPE_STR:
            printf("\"%ls\"\n", (wchar_t*)AST->data.literal.value);
            break;
        case RESULT_TYPE_CH:
            printf("'%lc'\n", *((wchar_t*)AST->data.literal.value));
            break;
        case RESULT_TYPE_INT:
            printf("%ld\n", *((long int*)AST->data.literal.value));
            break;
        case RESULT_TYPE_DOUBLE:
            printf("%lf\n", *((double*)AST->data.literal.value));
            break;
        case RESULT_TYPE_FLOAT:
            printf("%f\n", *((float*)AST->data.literal.value));
            break;
        default:
            printf("未知字面量\n");
            break;
        }
    } else if (AST->node_type == AST_NODE_VARIABLE) {
        printf("%ls\n", AST->token ? AST->token->value : L"(null)");
    } else {
        printf("未知节点类型\n");
    }
}

void showAST(HxASTNode* AST) {
    if (!AST) {
        printf("(空AST)\n");
        return;
    }
    printf("------AST------\n");
    _showAST(AST, 0);
    printf("---------------\n");
}
#endif

void free_ast_node(HxASTNode* node);
// 创建一个字面量节点
HxASTNode* create_literal_node(Token* token, int* err) {
    HxASTNode* node = (HxASTNode*)calloc(1, sizeof(HxASTNode));
    if (!node) {
        *err = -1;
        return NULL;
    }

    node->node_type = AST_NODE_LITERAL;
    node->token = token;

    ResultType type;
    if (token->mark == STR) {
        type.type = RESULT_TYPE_STR;
    } else if (token->mark == CH) {
        type.type = RESULT_TYPE_CH;
    } else {
        type = parseType(token->value);
    }

    if (type.type == UNKNOWN) {
        compileError(ERR_EXP, token->lin);
        *err = 255;
        free(node);
        return NULL;
    }
    node->data.literal.type = type;
    if(type.type == RESULT_TYPE_STR) {
        node->data.literal.value = (wchar_t*)calloc(wcslen(token->value)+1, sizeof(wchar));
        if(!(node->data.literal.value)) return NULL;
        if(token->value!=NULL) wcscpy(node->data.literal.value, token->value);
        stringEscape((wchar_t*)(node->data.literal.value));
    } else if(type.type == RESULT_TYPE_CH) {
        node->data.literal.value = (wchar_t*)calloc(1, sizeof(wchar));
        if(!(node->data.literal.value)) return NULL;
        if(token->value!=NULL) *(wchar_t*)(node->data.literal.value) = token->value[0];
    } else {

        *err = allocOpValueByType(&type, &node->data.literal.value);
        if (*err) {
            free(node);
            return NULL;
        }
        *err = setValueByType(&type, token->value, &node->data.literal.value);
        if (*err) {
            free(node->data.literal.value);
            free(node);
            return NULL;
        }
    }

    return node;
}

// 创建一个变量节点
HxASTNode* create_variable_node(Token* token, int* err) {
    HxASTNode* node = (HxASTNode*)calloc(1, sizeof(HxASTNode));
    if (!node) {
        *err = -1;
        return NULL;
    }

    node->node_type = AST_NODE_VARIABLE;
    node->token = token;
    node->data.variable.name = wcsdup(token->value); // 使用 wcsdup 创建副本
    if (!node->data.variable.name) {
        *err = -1;
        free(node);
        return NULL;
    }

    return node;
}

// 创建一个二元运算节点
HxASTNode* create_binary_op_node(OperatorType op, HxASTNode* left, HxASTNode* right, Token* op_token, int* err) {
    if (!left || !right) {
        // 如果左右子节点有任何一个是NULL，说明之前的解析出错了
        // 这里需要释放已经成功解析的部分，避免内存泄漏
        if(left) free_ast_node(left);
        if(right) free_ast_node(right);
        *err = 255;
        return NULL;
    }
    HxASTNode* node = (HxASTNode*)calloc(1, sizeof(HxASTNode));
    if (!node) {
        *err = -1;
        return NULL;
    }

    node->node_type = AST_NODE_BINARY_OP;
    node->token = op_token;
    node->data.binary_op.op = op;
    node->data.binary_op.left = left;
    node->data.binary_op.right = right;

    return node;
}

// 递归释放AST树
void free_ast_node(HxASTNode* node) {
    if (!node) return;

    switch (node->node_type) {
    case AST_NODE_LITERAL:
        if (node->data.literal.value) {
            free(node->data.literal.value);
        }
        break;
    case AST_NODE_VARIABLE:
        if (node->data.variable.name) {
            free(node->data.variable.name);
        }
        break;
    case AST_NODE_BINARY_OP:
        free_ast_node(node->data.binary_op.left);
        free_ast_node(node->data.binary_op.right);
        break;
    }
    free(node);
}


typedef struct {
    Token* tokens;
    int size;
    int pos;
    int* err; // 指向外部错误码的指针
} ParserState;

// 偷看
static Token* peek(ParserState* state) {
    if (state->pos >= state->size) return NULL;
    return &state->tokens[state->pos];
}
// 消耗当前Token并前进
static Token* advance(ParserState* state) {
    if (state->pos < state->size) {
        state->pos++;
        return &state->tokens[state->pos - 1];
    }
    return NULL;
}
static HxASTNode* parse_expression(ParserState* state);
// factor -> NUMBER | FLOAT | IDENTIFIER | '(' expression ')'
static HxASTNode* parse_factor(ParserState* state) {
    Token* token = peek(state);
    if (!token) {
        *state->err = 255;
        return NULL;
    }
    if (token->type == TOK_VAL && token->mark != STR && token->mark != CH) {
        //偷看后面是否跟着 '.' 和另一个数值Token
        if (state->pos + 2 < state->size &&
                wcsequ(state->tokens[state->pos + 1].value, L".") &&
                state->tokens[state->pos + 2].type == TOK_VAL)
        {
            //浮点数の处理逻辑
            Token* int_part = advance(state);      // 消耗整数部分 "1144"
            advance(state);                        // 消耗小数点 "."
            Token* frac_part = advance(state);     // 消耗小数部分 "115"

            size_t len1 = wcslen(int_part->value);
            size_t len2 = wcslen(frac_part->value);
            wchar_t* full_float_str = (wchar_t*)malloc((len1 + 1 + len2 + 1) * sizeof(wchar_t));
            if (!full_float_str) {
                *state->err = -1;
                return NULL;
            }

            wcscpy(full_float_str, int_part->value);
            wcscat(full_float_str, L".");
            wcscat(full_float_str, frac_part->value);
            Token temp_token;
            temp_token.value = full_float_str;
            temp_token.lin = int_part->lin;
            temp_token.mark = -1; // 表示不是STR或CH
            temp_token.type = TOK_VAL;
            HxASTNode* node = create_literal_node(&temp_token, state->err);
            free(full_float_str);
            return node;

        } else {
            //普通的整数
            advance(state);
            return create_literal_node(token, state->err);
        }
    }
    // 处理字符串和字符字面量
    if (token->type == TOK_VAL) {
        advance(state);
        return create_literal_node(token, state->err);
    }
    if (token->type == TOK_ID) {
        advance(state); // 消耗标识符
        return create_variable_node(token, state->err);
    }
    if (wcsequ(token->value, L"(")) {
        advance(state); // 消耗 '('
        HxASTNode* node = parse_expression(state);

        Token* closing_paren = peek(state);
        if (!closing_paren || !wcsequ(closing_paren->value, L")")) {
            compileError(ERR_QUITE_NOT_CORRECTLY_CLOSE, token->lin);
            *state->err = 255;
            free_ast_node(node);
            return NULL;
        }
        advance(state); // 消耗 ')'
        return node;
    }
    // 未知的Token，不应出现在表达式的开头
    compileError(ERR_EXP, token->lin);
    *state->err = 255;
    return NULL;
}
// term -> factor (('*' | '/') factor)*
static HxASTNode* parse_term(ParserState* state) {
    HxASTNode* node = parse_factor(state);
    if (*state->err) return node;

    while (peek(state) && (wcsequ(peek(state)->value, L"*") || wcsequ(peek(state)->value, L"/")|| wcsequ(peek(state)->value, L"÷") || wcsequ(peek(state)->value, L"×"))) {
        Token* op_token = advance(state); // 消耗 '*' 或 '/'
        OperatorType op = {0};
        if(wcsequ(op_token->value, L"*")||wcsequ(op_token->value, L"×")) {
            op = MUL;
        } else {
            op = DIV;
        }

        HxASTNode* right = parse_factor(state);
        if (*state->err) {
            free_ast_node(node);
            return right;
        }

        node = create_binary_op_node(op, node, right, op_token, state->err);
        if (*state->err) return node; // 创建节点失败
    }
    return node;
}

// expression -> term (('+' | '-') term)*
static HxASTNode* parse_expression(ParserState* state) {
    HxASTNode* node = parse_term(state);
    if (*state->err) return node;

    while (peek(state) && (wcsequ(peek(state)->value, L"+") || wcsequ(peek(state)->value, L"-"))) {
        Token* op_token = advance(state); // 消耗 '+' 或 '-'
        OperatorType op = (wcsequ(op_token->value, L"+")) ? ADD : SUB;

        HxASTNode* right = parse_term(state);
        if (*state->err) {
            free_ast_node(node);
            return right;
        }

        node = create_binary_op_node(op, node, right, op_token, state->err);
        if (*state->err) return node;
    }
    return node;
}


HxASTNode* parseToAST(Token* exp, int exp_size, int* err) {
    if (!exp || exp_size <= 0 || !err) {
        if(err) *err = -1;
        return NULL;
    }

    ParserState state;
    state.tokens = exp;
    state.size = exp_size;
    state.pos = 0;
    state.err = err;
    *err = 0;

    HxASTNode* root = parse_expression(&state);

    // 检查解析后是否还有多余的Token（例如 "1+2 3"，"3"就是多余的）
    if (*err == 0 && state.pos < state.size) {
        compileError(ERR_EXP, peek(&state)->lin);
        *err = 255;
        free_ast_node(root);
        return NULL;
    }

    return root;
}
/* helper: 确保 cmd 数组能容纳索引 idx（包含 idx） */
static int ensure_cmd_capacity(Command** cmd, int idx, int* cmd_size) {
    if (idx < 0) return -1;
    if (*cmd == NULL) {
        int initial = idx + 1;
        *cmd = (Command*)calloc((size_t)initial, sizeof(Command));
        if (!*cmd) return -1;
        *cmd_size = initial;
        return 0;
    }
    if (idx >= *cmd_size) {
        int old = *cmd_size;
        int newsize = idx + 1; /* 至少能容纳 idx */
        /* 可以改为 growth factor (old * 2) 来减少 realloc 次数，保持简单先按 idx+1 */
        void* tmp = realloc(*cmd, (size_t)newsize * sizeof(Command));
        if (!tmp) return -1;
        /* 将新扩展出来的区域置零，防止未初始化使用 */
        if (newsize > old) {
            memset((char*)tmp + (size_t)old * sizeof(Command), 0,
                   (size_t)(newsize - old) * sizeof(Command));
        }
        *cmd = (Command*)tmp;
        *cmd_size = newsize;
    }
    return 0;
}

int gen(HxASTNode* root, Command** cmd, int* cmd_index, int* cmd_size, ResultType* result_type, CheckerSymbol** table, int table_size) {
    if (!root || !cmd || !cmd_index || !cmd_size || !result_type) return -1;

    if (ensure_cmd_capacity(cmd, *cmd_index, cmd_size) != 0) return -1;

    memset(&(*cmd)[*cmd_index], 0, sizeof(Command));
    (*cmd)[*cmd_index].op = 0;
    (*cmd)[*cmd_index].op_value = NULL;
    (*cmd)[*cmd_index].op_value_size = 0;

    if (root->node_type == AST_NODE_VARIABLE) {
        Command* cur = &(*cmd)[*cmd_index];
        cur->op = OP_PUSH;
        cur->op_value_size = 1;
        cur->op_value = (ObjValue*)calloc(1, sizeof(ObjValue));
        if (!cur->op_value) return -1;

        size_t nchars = wcslen(root->data.variable.name) + 1;
        size_t bytes = nchars * sizeof(wchar_t);
        wchar_t* s = (wchar_t*)calloc(nchars, sizeof(wchar_t));
        if (!s) return -1;
        wcscpy(s, root->data.variable.name);

        CheckerSymbol* symbol = NULL;
        if(!findSymbol(s, table, table_size, &symbol)) {
            compileError(ERR_SYM_NOT_DEFINED, root->token->lin);
            return 255;
        }
        ResultType result_type_temp= parseTypeByTypeStr(symbol->type);
        if(result_type_temp.type <= result_type->type) result_type->type=result_type_temp.type;
        if(result_type->type==UNKNOWN) {
            *result_type = result_type_temp;
        }

        cur->op_value[0].value = (void*)s;
        cur->op_value[0].size = (int)bytes;
        cur->op_value[0].type = TYPE_SYM;

#ifdef HX_DEBUG
        printf("\33[33m生成指令：OP_PUSH\t%ls\n\33[0m", s);
#endif

    } else if (root->node_type == AST_NODE_LITERAL) {
        Command* cur = &(*cmd)[*cmd_index];
        cur->op = OP_PUSH;
        cur->op_value_size = 1;
        cur->op_value = (ObjValue*)calloc(1, sizeof(ObjValue));
        if (!cur->op_value) return -1;

        int rtype = root->data.literal.type.type;
        if (rtype == RESULT_TYPE_STR) {
            wchar_t* src = (wchar_t*)root->data.literal.value;
            size_t nchars = wcslen(src) + 1;
            size_t bytes = nchars * sizeof(wchar_t);
            wchar_t* dst = (wchar_t*)calloc(nchars, sizeof(wchar_t));
            if (!dst) return -1;
            wcscpy(dst, src);
            cur->op_value[0].value = (void*)dst;
            cur->op_value[0].size = (int)bytes;
            cur->op_value[0].type = TYPE_STR;
            result_type->type = RESULT_TYPE_STR;

#ifdef HX_DEBUG
            printf("\33[33m生成指令：OP_PUSH\t\"%ls\"\n\33[0m", dst);
#endif

        } else if (rtype == RESULT_TYPE_CH) {
            wchar_t* ch = (wchar_t*)calloc(1, sizeof(wchar_t));
            if (!ch) return -1;
            *ch = *((wchar_t*)root->data.literal.value);
            cur->op_value[0].value = (void*)ch;
            cur->op_value[0].size = (int)sizeof(wchar_t);
            cur->op_value[0].type = TYPE_CH;
            if (result_type->type <= RESULT_TYPE_CH) result_type->type = RESULT_TYPE_CH;

#ifdef HX_DEBUG
            printf("\33[33m生成指令：OP_PUSH\t'%lc'\n\33[0m", *ch);
#endif

        } else {
            cur->op_value[0].value = NULL;
            int err = allocOpValueByType(&(root->data.literal.type), &cur->op_value[0].value);
            if (err) return err;

            switch (root->data.literal.type.type) {
            case RESULT_TYPE_INT:
                cur->op_value[0].type = TYPE_INT;
                cur->op_value[0].size = (int)sizeof(long int);
                *((long int*)cur->op_value[0].value) = *((long int*)root->data.literal.value);
#ifdef HX_DEBUG
                printf("\33[33m生成指令：OP_PUSH\t%ld\n\33[0m", *((long int*)cur->op_value[0].value));
#endif
                break;
            case RESULT_TYPE_FLOAT:
                cur->op_value[0].type = TYPE_FLOAT;
                cur->op_value[0].size = (int)sizeof(float);
                *((float*)cur->op_value[0].value) = *((float*)root->data.literal.value);
#ifdef HX_DEBUG
                printf("\33[33m生成指令：OP_PUSH\t%f\n\33[0m", *((float*)cur->op_value[0].value));
#endif
                break;
            case RESULT_TYPE_DOUBLE:
                cur->op_value[0].type = TYPE_DOUBLE;
                cur->op_value[0].size = (int)sizeof(double);
                *((double*)cur->op_value[0].value) = *((double*)root->data.literal.value);
#ifdef HX_DEBUG
                printf("\33[33m生成指令：OP_PUSH\t%lf\n\33[0m", *((double*)cur->op_value[0].value));
#endif
                break;
            default:
                compileError(ERR_EXP, root->token ? root->token->lin : 0);
                return 255;
            }
            if (root->data.literal.type.type <= result_type->type) {
                result_type->type = root->data.literal.type.type;
            }
        }

    } else if (root->node_type == AST_NODE_BINARY_OP) {
        /* 先生成左、右子树指令 */
        if (!root->data.binary_op.left || !root->data.binary_op.right) return -1;

        int err = gen(root->data.binary_op.left, cmd, cmd_index, cmd_size, result_type,table, table_size);
        if (err) return err;
        err = gen(root->data.binary_op.right, cmd, cmd_index, cmd_size, result_type,table, table_size);
        if (err) return err;

        /* 递归返回后，*cmd_index 已经指向下一个空位；在写入前再次保证容量 */
        if (ensure_cmd_capacity(cmd, *cmd_index, cmd_size) != 0) return -1;
        Command* cur = &(*cmd)[*cmd_index];
        /* 清零并初始化 */
        memset(cur, 0, sizeof(Command));
        cur->op_value = NULL;
        cur->op_value_size = 0;

        switch (root->data.binary_op.op) {
        case ADD:
            cur->op = OP_ADD;
            break;

        case MUL:
            cur->op = OP_MUL;
            break;

        case SUB:
            cur->op = OP_SUB;
            break;

        case DIV:
            cur->op = OP_DIV;
            break;

        default:
            compileError(ERR_EXP, root->token ? root->token->lin : 0);
            return 255;
        }
#ifdef HX_DEBUG
        wprintf(L"\33[33m生成指令：OP_%ls\n\33[0m",
                root->data.binary_op.op == ADD ? L"ADD" :
                root->data.binary_op.op == MUL ? L"MUL" :
                root->data.binary_op.op == SUB ? L"SUB" : L"DIV");
#endif
    } else {
        return -1;
    }
    /* 成功生成当前命令，索引递增 */
    (*cmd_index)++;
    return 0;
}
int parseEXP(Command** cmd,int* cmd_index,int* cmd_size,Token* exp, int exp_size, ResultType* result_type, CheckerSymbol** table, int table_size) {
    int err = 0;
    if(exp==NULL||exp_size==0) return -1;
    HxASTNode* AST = parseToAST(exp, exp_size, &err);
    if(err) return err;
#ifdef HX_DEBUG
    showAST(AST);
#endif
    result_type->type = UNKNOWN;
    err = gen(AST, cmd, cmd_index, cmd_size, result_type,table, table_size);
    if(err) return err;
    free_ast_node(AST);
    AST = NULL;
    return 0;
}
#endif