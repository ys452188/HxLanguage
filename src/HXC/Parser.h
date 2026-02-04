#ifndef HX_PARSER_H
#define HX_PARSER_H

#include "Error.h"
#include "IR.h"
#include "Lexer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <wchar.h>

typedef struct Symbol {
  wchar_t *name;
  IR_DataType type;
} Symbol;

typedef struct SymbolTable {
  Symbol *symbols;
  uint32_t size;
} SymbolTable;

typedef struct ASTNode {
  enum { NODE_VALUE, NODE_VAR, NODE_UNARY, NODE_BINARY } kind;
  union {
    struct {
      IR_DataType type;
      union {
        double f;
        int32_t i;
        wchar_t *s;
        uint16_t c;
      } val;
    } value;
    struct {
      wchar_t *name;
      int index;
      IR_DataType type;
    } var;
    struct {
      int op;
    } unary; // NEG, POS
    struct {
      int op;
    } binary; // ADD, SUB, MUL, DIV
  } data;
  struct ASTNode *left;
  struct ASTNode *right;
  Token *token; // 用于错误定位
} ASTNode;

void freeAST(ASTNode *node) {
  if (!node)
    return;
  freeAST(node->left);
  freeAST(node->right);
  if (node->kind == NODE_VAR)
    free(node->data.var.name);
  if (node->kind == NODE_VALUE && node->data.value.type.kind == IR_DT_STRING)
    free(node->data.value.val.s);
  free(node);
}
static int getVarIndex(const wchar_t *name, SymbolTable *table) {
  for (uint32_t i = 0; i < table->size; i++) {
    if (wcscmp(table->symbols[i].name, name) == 0)
      return i;
  }
  return -1;
}
static int getPrec(HxTokenType t) {
  if (t == TOK_OPR_MUL || t == TOK_OPR_DIV)
    return 2;
  if (t == TOK_OPR_ADD || t == TOK_OPR_SUB)
    return 1;
  return 0;
}
static ASTNode *parsePrimary(Token *tokens, int *index, int size,
                             SymbolTable *table, int *err);
static ASTNode *parseExprRec(Token *tokens, int *index, int size,
                             SymbolTable *table, int *err, int min_prec);

// 解析数字、变量、括号
static ASTNode *parsePrimary(Token *tokens, int *index, int size,
                             SymbolTable *table, int *err) {
  if (*index >= size) {
    *err = 255;
    return NULL;
  }

  Token *curr = &tokens[*index];
  ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
  if (!node) {
    *err = -1;
    return NULL;
  }
  node->token = curr;

  // 处理括号
  if (curr->type == TOK_OPR_LQUOTE) {
    (*index)++;
    free(node); // 括号本身不产生节
    ASTNode *inner = parseExprRec(tokens, index, size, table, err, 0);
    if (*err == 0 && (*index < size && tokens[*index].type == TOK_OPR_RQUOTE)) {
      (*index)++;
      return inner;
    }
    if (*err == 0)
      *err = 255; // 缺少右括号
    return NULL;
  }

  // 处理字面量
  if (curr->type == TOK_VAL) {
    node->kind = NODE_VALUE;
    if (curr->mark == STR) {
      node->data.value.type.kind = IR_DT_STRING;
      node->data.value.val.s = wcsdup(curr->value);
    } else if (wcschr(curr->value, L'.')) {
      node->data.value.type.kind = IR_DT_FLOAT;
      node->data.value.val.f = wcstod(curr->value, NULL);
    } else if (curr->mark==CH) {
      node->data.value.type.kind = IR_DT_CHAR;
      node->data.value.val.c = (uint16_t)(curr->value[0]);
    } else {
      node->data.value.type.kind = IR_DT_INT;
      node->data.value.val.i = (int32_t)wcstol(curr->value, NULL, 10);
    }
    (*index)++;
    return node;
  }

  // 处理变量
  if (curr->type == TOK_ID) {
    node->kind = NODE_VAR;
    node->data.var.name = wcsdup(curr->value);
    int symIdx = getVarIndex(node->data.var.name, table);
    node->data.var.index = symIdx;
    if (symIdx != -1) {
      node->data.var.type = table->symbols[symIdx].type; 
    }
    (*index)++;
    return node;
  }

  *err = 255;
  free(node);
  return NULL;
}
// 优先级爬升
ASTNode *parseExprRec(Token *tokens, int *index, int size, SymbolTable *table,
                      int *err, int min_prec) {
  ASTNode *lhs = parsePrimary(tokens, index, size, table, err);
  if (*err != 0 || lhs == NULL)
    return lhs;

  while (*index < size) {
    Token *opTok = &tokens[*index];
    int prec = getPrec(opTok->type);
    // 当前表达式结束
    if (prec <= 0 || prec < min_prec)
      break;
    // 消耗掉运算符
    (*index)++;
    // 递归解析右侧操作数
    ASTNode *rhs = parseExprRec(tokens, index, size, table, err, prec + 1);

    if (rhs == NULL) {
      if (*err == 0)
        *err = 255;
      setError(ERR_EXP, opTok->line, opTok->value); // 报错：期待表达式
      if (lhs)
        freeAST(lhs);
      return NULL;
    }
    // 构建二元运算节点
    ASTNode *combined = (ASTNode *)calloc(1, sizeof(ASTNode));
    if (!combined) {
      *err = -1;
      freeAST(lhs);
      freeAST(rhs);
      return NULL;
    }

    combined->kind = NODE_BINARY;
    combined->left = lhs;
    combined->right = rhs;
    combined->token = opTok;

    if (opTok->type == TOK_OPR_ADD)
      combined->data.binary.op = 0;
    else if (opTok->type == TOK_OPR_SUB)
      combined->data.binary.op = 1;
    else if (opTok->type == TOK_OPR_MUL)
      combined->data.binary.op = 2;
    else if (opTok->type == TOK_OPR_DIV)
      combined->data.binary.op = 3;

    lhs = combined;
  }
  return lhs;
}
ASTNode *parseExpression(Token *exp, int *index, int size, SymbolTable *table,
                         int *err) {
#ifdef HX_DEBUG
  log(L"分析表达式...");
#endif
  if (!exp || !index || !err || !table)
    return NULL;
  *err = 0;

  ASTNode *root = parseExprRec(exp, index, size, table, err, 0);
  if (*err == 0 && root != NULL && *index < size) {
    if (exp[*index].type != TOK_END) {
      *err = 255;
      setError(ERR_EXP, exp[*index].line, exp[*index].value);
      return NULL;
    }
  }
  (*index)--;  //指向表达式最后一个有效单元
  return root;
}
#endif