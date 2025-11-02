#ifndef HXLANG_FIRST_PASS_H
#define HXLANG_FIRST_PASS_H
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "Error.h"
#include "Lexer.h"

typedef struct _Symbol {  // 变/常量符号
  int line;
  wchar_t* name;
  wchar_t* type;           // type为NULL表示暂未确定类型
  unsigned int array_num;  // 数组维数
  bool isOnlyRead;
  Tokens* exp;
} _Symbol;
typedef struct _Function {  // 函数
  wchar_t* name;
  wchar_t* ret_type;     // 为NULL表示返回值为void
  int ret_type_arr_num;  // 返回类型的数组层
  bool isRetTypeKnown;
  _Symbol* args;  // 为NULL表示参数为空
  int args_size;
  Tokens* body;
} _Function;
typedef struct _ClassMember {  // 类成员
  _Function* funcs;
  int funcs_size;
  int funcs_count;
  _Symbol* syms;
  int syms_size;
  int syms_count;
} _ClassMember;
typedef struct _Class {  // 类
  int line;
  wchar_t* name;
  _ClassMember* publicMember;     // 公有成员
  _ClassMember* protectedMember;  // 受保护成员
  _ClassMember* privateMember;    // 私有成员
  wchar_t* parent_name;
  int parent;  // 父类索引,为-1表示此类没有父类
} _Class;
typedef struct IR_1 {  // 中间中间代码
  int start_fun;       // 主函数索引,-1表示无主函数
  _Class* classes;     // 类
  int class_size;
  _Function* funcs;  // 函数
  int funcs_size;
  _Symbol* global_syms;  // 全局变量
  int global_syms_size;
} IR_1;

static int getNextToken(int* index, int size) {
  if (*index < size - 1) {
    (*index)++;
    return 0;
  }
  return 1;
}
#define getNextTokenDo getNextToken(&token_index, token_size)
static _Class* findClassByName(wchar_t* name, _Class* class,
                               int size);  // 辅助：查找父类
static int parseVarDef(Tokens* tokens, int* index, _Symbol* sym);
static int parseFunDef(Tokens* tokens, int* index, _Function* fun);
static int parseClassNotExtend(
    Tokens* tokens, int* index,
    _Class* class);     // 辅助函数：分析无继承的类 (从花括号开始分析)
void freeIR_1(IR_1**);  // 释放IR_1对象
static int setParentIndex(_Class*,
                          int class_size);  // 辅助：为每一个类设置父类的索引
// 第一次遍历将分析出所有类
int pass(Tokens* tokens, IR_1* ir) {
#ifdef HX_DEBUG
  log(L"第一遍...");
#endif
  if (tokens == NULL) return -1;
  if (ir == NULL) return -1;
  ir->start_fun = -1;
  int token_index = 0;
  int token_size = tokens->count;
  int global_syms_index = 0;
  while (token_index < token_size) {
    // wprintf(L"%ls\n", tokens->tokens[token_index].value);
    if (wcscmp(tokens->tokens[token_index].value, L"class") == 0 ||
        wcscmp(tokens->tokens[token_index].value, L"定义类") == 0) {
      // wprintf(L"L67   %ls\n", tokens->tokens[token_index].value);
      if (getNextTokenDo ||
          (wcscmp(tokens->tokens[token_index].value, L":") != 0 &&
           wcscmp(tokens->tokens[token_index].value, L"：") != 0)) {
        setError(ERR_DEF_CLASS, tokens->tokens[token_index].line, NULL);
        return 255;
      }
      //<"class:"> <ID> [<",parent"><":"><ID>] <"{"> ... <"}">
      // wprintf(L"L72   %ls\n", tokens->tokens[token_index].value);
      int class_line = tokens->tokens[token_index].line;
      if (getNextTokenDo || tokens->tokens[token_index].type != TOK_ID) {
        wchar_t* errCode = NULL;
        if (token_index - 1 >= 0) {
          token_index--;
          errCode = (wchar_t*)calloc(
              wcslen(tokens->tokens[token_index].value) +
                  wcslen(tokens->tokens[token_index + 1].value) + 3,
              sizeof(wchar_t));
          if (!errCode) return -1;
          wcscpy(errCode, tokens->tokens[token_index].value);
          wcscat(errCode, L" ");
          token_index++;
        } else {
          errCode = (wchar_t*)calloc(
              wcslen(tokens->tokens[token_index].value) + 1, sizeof(wchar_t));
        }
        if (!errCode) return -1;
        wcscat(errCode, tokens->tokens[token_index].value);
        setError(ERR_BEHIND_CLASS_SHOULD_BE_ID,
                 tokens->tokens[token_index].line, errCode);
        free(errCode);
        return 255;
      }
      wchar_t* class_name = tokens->tokens[token_index].value;
      // 类名后应为花括号
      if (getNextTokenDo) {
        setError(ERR_BEHIND_CLASS_NAME_SHOULD_BE_HUAKUOHAO,
                 tokens->tokens[token_index].line,
                 tokens->tokens[token_index].value);
        return 255;
      }
      // wprintf(L"L96   %ls\n", tokens->tokens[token_index].value);
      wchar_t* parent_name = NULL;
      // 无继承
      if (wcscmp(tokens->tokens[token_index].value, L"{") == 0) {
        // 继承
      } else if (wcscmp(tokens->tokens[token_index].value, L",") == 0 ||
                 wcscmp(tokens->tokens[token_index].value, L"，") == 0) {
        if (getNextTokenDo) {
          setError(ERR_DEF_CLASS, tokens->tokens[token_index].line, NULL);
          return 255;
        }
        if (wcscmp(tokens->tokens[token_index].value, L"它的父类是") != 0 &&
            wcscmp(tokens->tokens[token_index].value, L"parent") != 0) {
          setError(ERR_DEF_CLASS, tokens->tokens[token_index].line, NULL);
          return 255;
        }
        if (getNextTokenDo) {
          setError(ERR_DEF_CLASS, tokens->tokens[token_index].line, NULL);
          return 255;
        }
        if (wcscmp(tokens->tokens[token_index].value, L":") != 0 &&
            wcscmp(tokens->tokens[token_index].value, L"：") != 0) {
          setError(ERR_DEF_CLASS, tokens->tokens[token_index].line, NULL);
          return 255;
        }
        if (getNextTokenDo) {
          setError(ERR_DEF_CLASS, tokens->tokens[token_index].line, NULL);
          return 255;
        }
        if (tokens->tokens[token_index].type != TOK_ID) {
          setError(ERR_DEF_CLASS, tokens->tokens[token_index].line, NULL);
          return 255;
        }
        parent_name = tokens->tokens[token_index].value;
        if (getNextTokenDo) {
          setError(ERR_BEHIND_CLASS_NAME_SHOULD_BE_HUAKUOHAO,
                   tokens->tokens[token_index].line,
                   tokens->tokens[token_index].value);
          return 255;
        }
        if (wcscmp(tokens->tokens[token_index].value, L"{") != 0) {
          wchar_t* errCode = NULL;
          if (token_index - 1 >= 0) {
            token_index--;
            errCode = (wchar_t*)calloc(
                wcslen(tokens->tokens[token_index].value) +
                    wcslen(tokens->tokens[token_index + 1].value) + 3,
                sizeof(wchar_t));
            if (!errCode) return -1;
            wcscpy(errCode, tokens->tokens[token_index].value);
            wcscat(errCode, L" ");
            token_index++;
          } else {
            errCode = (wchar_t*)calloc(
                wcslen(tokens->tokens[token_index].value) + 1, sizeof(wchar_t));
          }
          if (!errCode) return -1;
          wcscat(errCode, tokens->tokens[token_index].value);
          setError(ERR_BEHIND_CLASS_NAME_SHOULD_BE_HUAKUOHAO,
                   tokens->tokens[token_index].line, errCode);
          free(errCode);
          return 255;
        }
      } else {
        wchar_t* errCode = NULL;
        if (token_index - 1 >= 0) {
          token_index--;
          errCode = (wchar_t*)calloc(
              wcslen(tokens->tokens[token_index].value) +
                  wcslen(tokens->tokens[token_index + 1].value) + 3,
              sizeof(wchar_t));
          if (!errCode) return -1;
          wcscpy(errCode, tokens->tokens[token_index].value);
          wcscat(errCode, L" ");
          token_index++;
        } else {
          errCode = (wchar_t*)calloc(
              wcslen(tokens->tokens[token_index].value) + 1, sizeof(wchar_t));
        }
        if (!errCode) return -1;
        wcscat(errCode, tokens->tokens[token_index].value);
        setError(ERR_BEHIND_CLASS_NAME_SHOULD_BE_HUAKUOHAO,
                 tokens->tokens[token_index].line, errCode);
        free(errCode);
        return 255;
      }
      void* temp = realloc(ir->classes, (ir->class_size + 1) * sizeof(_Class));
      if (!temp) return -1;
      ir->classes = (_Class*)temp;
      ir->class_size++;
      memset((ir->classes + ir->class_size - 1), 0, sizeof(_Class));
      ir->classes[ir->class_size - 1].name =
          (wchar_t*)calloc(wcslen(class_name) + 1, sizeof(wchar_t));
      if (!(ir->classes[ir->class_size - 1].name)) return -1;
      wcscpy(ir->classes[ir->class_size - 1].name, class_name);
      ir->classes[ir->class_size - 1].line = class_line;

      if (parent_name) {
        ir->classes[ir->class_size - 1].parent_name =
            (wchar_t*)calloc(wcslen(parent_name) + 1, sizeof(wchar_t));
        if (!(ir->classes[ir->class_size - 1].parent_name)) return -1;
        wcscpy(ir->classes[ir->class_size - 1].parent_name, parent_name);
#ifdef HX_DEBUG
        fwprintf(logStream, L"\33[33m[DEB]已复制类%ls的父类名\n\33[0m",
                 ir->classes[ir->class_size - 1].name,
                 ir->classes[ir->class_size - 1].parent_name);
#endif
      }
#ifdef HX_DEBUG
      fwprintf(logStream, L"\33[33m[DEB]分析类%ls\n\33[0m",
               ir->classes[ir->class_size - 1].name);
#endif
      int parseError = parseClassNotExtend(tokens, &token_index,
                                           &(ir->classes[ir->class_size - 1]));
      if (parseError) return parseError;
    } else if (wcscmp(tokens->tokens[token_index].value, L"fun") == 0 ||
               wcscmp(tokens->tokens[token_index].value, L"定义函数") == 0) {
      void* temp = realloc(ir->funcs, (ir->funcs_size + 1) * sizeof(_Function));
      if (!temp) return -1;
      ir->funcs = (_Function*)temp;
      ir->funcs_size++;
      memset((ir->funcs + ir->funcs_size - 1), 0, sizeof(_Function));
      int fun_line = token_index;

      int parseError =
          parseFunDef(tokens, &token_index, &(ir->funcs[ir->funcs_size - 1]));
      if (parseError) return parseError;

      if (wcscmp(ir->funcs[ir->funcs_size - 1].name, L"main") == 0 ||
          wcscmp(ir->funcs[ir->funcs_size - 1].name, L"主函数") == 0) {
#ifdef HX_DEBUG
        log(L"分析主函数");
#endif
        // printf("argc:%d\n", ir->funcs[ir->funcs_size-1].args_size);
        if (ir->funcs[ir->funcs_size - 1].args_size == 0) {
          ir->start_fun = ir->funcs_size - 1;
        } else if (ir->funcs[ir->funcs_size - 1].args_size == 1) {
          if ((wcscmp(ir->funcs[ir->funcs_size - 1].args[0].type, L"str") ==
                   0 ||
               wcscmp(ir->funcs[ir->funcs_size - 1].args[0].type,
                      L"字符串型") == 0) &&
              ir->funcs[ir->funcs_size - 1].args[0].array_num == 1) {
            ir->start_fun = ir->funcs_size - 1;
          } else {
            setError(ERR_MAIN, tokens->tokens[fun_line].line, NULL);
            return 255;
          }
        } else {
          setError(ERR_MAIN, tokens->tokens[fun_line].line, NULL);
          return 255;
        }
      }
    } else if (wcscmp(tokens->tokens[token_index].value, L"var") == 0 ||
               wcscmp(tokens->tokens[token_index].value, L"定义变量") == 0 ||
               wcscmp(tokens->tokens[token_index].value, L"con") == 0 ||
               wcscmp(tokens->tokens[token_index].value, L"定义常量") == 0) {
      if (!ir->global_syms) {
        ir->global_syms = (_Symbol*)calloc(1, sizeof(_Symbol));
        if (!(ir->global_syms)) return -1;
        ir->global_syms_size = 1;
      }
      if (global_syms_index >= ir->global_syms_size) {
        void* temp =
            realloc(ir->global_syms, (global_syms_index + 1) * sizeof(_Symbol));
        if (!temp) return -1;
        ir->global_syms = (_Symbol*)temp;
        ir->global_syms_size = global_syms_index + 1;
        memset(&(ir->global_syms[global_syms_index]), 0, sizeof(_Symbol));
      }
      int parseError = parseVarDef(tokens, &token_index,
                                   &(ir->global_syms[global_syms_index]));
      if (parseError) return parseError;
      global_syms_index++;
    }
    if (getNextTokenDo) break;
  }
  // 检查全局变量
  if (ir->global_syms) {
    for (int i = 0; i < ir->global_syms_size; i++) {
      if (ir->global_syms[i].type) {
        if (!findClassByName(ir->global_syms[i].type, ir->classes,
                             ir->class_size)) {
          if (!(wcscmp(ir->global_syms[i].type, L"int") == 0 ||
                wcscmp(ir->global_syms[i].type, L"整型") == 0) &&
              !(wcscmp(ir->global_syms[i].type, L"float") == 0 ||
                wcscmp(ir->global_syms[i].type, L"浮点型") == 0) &&
              !(wcscmp(ir->global_syms[i].type, L"double") == 0 ||
                wcscmp(ir->global_syms[i].type, L"精确浮点型") == 0) &&
              !(wcscmp(ir->global_syms[i].type, L"char") == 0 ||
                wcscmp(ir->global_syms[i].type, L"字符型") == 0) &&
              !(wcscmp(ir->global_syms[i].type, L"str") == 0 ||
                wcscmp(ir->global_syms[i].type, L"字符串型") == 0)) {
            setError(ERR_UNKOWN_TYPE, ir->global_syms[i].line,
                     ir->global_syms[i].type);
            return 255;
          }
        }
      }
    }
  }
#ifdef HX_DEBUG
  log(L"分析父类...");
#endif
  int parseParentError = setParentIndex(ir->classes, ir->class_size);
  if (parseParentError) return parseParentError;
  return 0;
}
static _Class* findClassByName(wchar_t* name, _Class* class_array, int size) {
  if (!name) return NULL;
  // class_array 是数组的起始地址，检查它是否为 NULL
  if (!class_array || size == 0) return NULL;
#ifdef HX_DEBUG
  fwprintf(logStream, L"\33[33m[DEB]查找类(\"%ls\")\n\33[0m", name);
#endif
  for (int i = 0; i < size; i++) {
    // 直接通过索引访问数组元素
    if (wcscmp(class_array[i].name, name) == 0) {
      // 返回该元素的地址
      return &class_array[i];
    }
  }
  return NULL;
}
static int setParentIndex(_Class* class_array, int class_size) {
  if (!class_array || class_size == 0) return 0;
  for (int i = 0; i < class_size; i++) {
    wchar_t* parent = class_array[i].parent_name;
    if (parent) {
      _Class* parent_class = findClassByName(parent, class_array, class_size);
      if (!parent_class) {
        setError(ERR_COUNLD_NOT_FIND_PARENT, class_array[i].line, parent);
        return 255;
      }
      class_array[i].parent = parent_class - class_array;
    }
  }
  return 0;
}
// 分析变量定义
static int parseVarDef(Tokens* tokens, int* index, _Symbol* sym) {
  if (sym == NULL || tokens == NULL || index == NULL) return -1;
#ifdef HX_DEBUG
  log(L"分析变量的定义");
#endif
  sym->isOnlyRead = false;
  // var:<id> "->" <id>|<kw>   var:v1->int
  bool isOnlyRead = false;
  sym->line = tokens->tokens[*index].line;
  if (wcscmp(tokens->tokens[*index].value, L"con") == 0 ||
      wcscmp(tokens->tokens[*index].value, L"定义常量") == 0)
    isOnlyRead = true;
  if (wcscmp(tokens->tokens[*index].value, L"var") == 0 ||
      wcscmp(tokens->tokens[*index].value, L"con") == 0) {
    if (*index >= tokens->count) {
      setError(ERR_NO_SYM_NAME, tokens->tokens[*index].line, NULL);
      return 255;
    }
    (*index)++;
    // var : <id>
    //     ^
    if (wcscmp(tokens->tokens[*index].value, L":") != 0 &&
        wcscmp(tokens->tokens[*index].value, L"：") != 0) {
      setError(ERR_DEF_VAR, tokens->tokens[*index].line, NULL);
      return 255;
    }
    if (*index >= tokens->count) {
      setError(ERR_NO_SYM_NAME, tokens->tokens[*index].line, NULL);
      return 255;
    }
    (*index)++;
    // wprintf(L"value:%ls\n", tokens->tokens[*index].value);
    // 变量名
    if (tokens->tokens[*index].type != TOK_ID) {
      setError(ERR_NO_SYM_NAME, tokens->tokens[*index].line, NULL);
      return 255;
    }
    sym->name = (wchar_t*)calloc(wcslen(tokens->tokens[*index].value) + 1,
                                 sizeof(wchar_t));
    if (!sym->name) return -1;
    wcscpy(sym->name, tokens->tokens[*index].value);
#ifdef HX_DEBUG
    fwprintf(logStream, L"\33[33m[DEB]已复制变量名(\"%ls\")\n\33[0m",
             sym->name);
#endif
    if (*index >= tokens->count) {
      setError(ERR_NO_END, tokens->tokens[*index].line,
               tokens->tokens[tokens->count - 1].value);
      return 255;
    }
    (*index)++;
    // 结束
    if (tokens->tokens[*index].type == TOK_END) {
      sym->type = NULL;
      return 0;
    } else if (wcscmp(tokens->tokens[*index].value, L"->") == 0) {  // 分析类型
      if (*index >= tokens->count) {
        setError(ERR_NO_SYM_TYPE, tokens->tokens[*index].line,
                 tokens->tokens[tokens->count - 1].value);
        return 255;
      }
      (*index)++;
      if (tokens->tokens[*index].type != TOK_ID &&
          tokens->tokens[*index].type != TOK_KW) {
        setError(ERR_NO_SYM_TYPE, tokens->tokens[*index].line,
                 tokens->tokens[tokens->count - 1].value);
        return 255;
      }
      sym->type = (wchar_t*)calloc(wcslen(tokens->tokens[*index].value) + 1,
                                   sizeof(wchar_t));
      if (!sym->type) return -1;
      wcscpy(sym->type, tokens->tokens[*index].value);
#ifdef HX_DEBUG
      fwprintf(logStream, L"\33[33m[DEB]已复制变量类型(\"%ls\")\n\33[0m",
               sym->type);
#endif
      if (*index >= tokens->count) {
        setError(ERR_NO_END, tokens->tokens[tokens->count - 1].line,
                 tokens->tokens[tokens->count - 1].value);
        return 255;
      }
      (*index)++;
      // 数组
      if (wcscmp(tokens->tokens[*index].value, L"[") == 0 ||
          wcscmp(tokens->tokens[*index].value, L"【") == 0) {
        sym->array_num++;
        while (*index + 1 < tokens->count) {
          (*index)++;
          if (wcscmp(tokens->tokens[*index].value, L"]") != 0 &&
              wcscmp(tokens->tokens[*index].value, L"】") != 0) {
            setError(ERR_ARR_TYPE, tokens->tokens[*index].line, NULL);
            return 255;
          }
          if (*index + 1 >= tokens->count) {
            setError(ERR_NO_END, tokens->tokens[*index].line,
                     tokens->tokens[*index].value);
            return 255;
          }
          (*index)++;
          if (tokens->tokens[*index].type == TOK_END) break;
          if (wcscmp(tokens->tokens[*index].value, L"[") == 0 ||
              wcscmp(tokens->tokens[*index].value, L"【") == 0)
            sym->array_num++;
        }
      }
      if (tokens->tokens[*index].type != TOK_END) {
        setError(ERR_NO_END, tokens->tokens[*index - 1].line,
                 tokens->tokens[*index - 1].value);
        wprintf(L"L231    %ls\n", tokens->tokens[*index].value);
        return 255;
      }
#ifdef HX_DEBUG
      fwprintf(logStream, L"\33[33m[DEB]数组维数%d\n\33[0m", sym->array_num);
#endif
    } else {
      setError(ERR_NO_END, tokens->tokens[*index].line,
               tokens->tokens[tokens->count - 2].value);
      return 255;
    }
  } else if (wcscmp(tokens->tokens[*index].value, L"定义变量") == 0 ||
             wcscmp(tokens->tokens[*index].value, L"定义常量") ==
                 0) {  // 定义变量：<id>,它的类型是：<id>|<kw>;
    if (*index >= tokens->count) {
      setError(ERR_NO_SYM_NAME, tokens->tokens[*index].line, NULL);
      return 255;
    }
    (*index)++;
    // var : <id>
    //     ^
    if (wcscmp(tokens->tokens[*index].value, L":") != 0 &&
        wcscmp(tokens->tokens[*index].value, L"：") != 0) {
      setError(ERR_DEF_VAR, tokens->tokens[*index].line, NULL);
      return 255;
    }
    if (*index >= tokens->count) {
      setError(ERR_NO_SYM_NAME, tokens->tokens[*index].line, NULL);
      return 255;
    }
    (*index)++;
    // wprintf(L"value:%ls\n", tokens->tokens[*index].value);
    // 变量名
    if (tokens->tokens[*index].type != TOK_ID) {
      setError(ERR_NO_SYM_NAME, tokens->tokens[*index].line, NULL);
      return 255;
    }
    sym->name = (wchar_t*)calloc(wcslen(tokens->tokens[*index].value) + 1,
                                 sizeof(wchar_t));
    if (!sym->name) return -1;
    wcscpy(sym->name, tokens->tokens[*index].value);
#ifdef HX_DEBUG
    fwprintf(logStream, L"\33[33m[DEB]已复制变量名(\"%ls\")\n\33[0m",
             sym->name);
#endif
    if (*index >= tokens->count) {
      setError(ERR_NO_END, tokens->tokens[*index].line,
               tokens->tokens[tokens->count - 1].value);
      return 255;
    }
    (*index)++;
    // 结束
    if (tokens->tokens[*index].type == TOK_END) {
      sym->type = NULL;
      return 0;
    } else if (wcscmp(tokens->tokens[*index].value, L",") == 0) {  // 分析类型
      if (*index >= tokens->count) {
        setError(ERR_NO_SYM_TYPE, tokens->tokens[*index].line,
                 tokens->tokens[tokens->count - 1].value);
        return 255;
      }
      (*index)++;
      if (wcscmp(tokens->tokens[*index].value, L"它的类型是") != 0) {
        setError(ERR_NO_SYM_TYPE, tokens->tokens[*index].line,
                 tokens->tokens[tokens->count - 1].value);
        return 255;
      }
      if (*index >= tokens->count) {
        setError(ERR_NO_SYM_TYPE, tokens->tokens[*index].line,
                 tokens->tokens[tokens->count - 1].value);
        return 255;
      }
      (*index)++;
      if (wcscmp(tokens->tokens[*index].value, L":") != 0 &&
          wcscmp(tokens->tokens[*index].value, L"：") != 0) {
        setError(ERR_NO_SYM_TYPE, tokens->tokens[*index].line,
                 tokens->tokens[tokens->count - 1].value);
        return 255;
      }
      if (*index >= tokens->count) {
        setError(ERR_NO_SYM_TYPE, tokens->tokens[*index].line,
                 tokens->tokens[tokens->count - 1].value);
        return 255;
      }
      (*index)++;
      if (tokens->tokens[*index].type != TOK_ID &&
          tokens->tokens[*index].type != TOK_KW) {
        setError(ERR_NO_SYM_TYPE, tokens->tokens[*index].line,
                 tokens->tokens[tokens->count - 1].value);
        return 255;
      }
      sym->type = (wchar_t*)calloc(wcslen(tokens->tokens[*index].value) + 1,
                                   sizeof(wchar_t));
      if (!sym->type) return -1;
      wcscpy(sym->type, tokens->tokens[*index].value);
#ifdef HX_DEBUG
      fwprintf(logStream, L"\33[33m[DEB]已复制变量类型(\"%ls\")\n\33[0m",
               sym->type);
#endif
      if (*index >= tokens->count) {
        setError(ERR_NO_END, tokens->tokens[tokens->count - 1].line,
                 tokens->tokens[tokens->count - 1].value);
        return 255;
      }
      (*index)++;
      // 数组
      if (wcscmp(tokens->tokens[*index].value, L"[") == 0 ||
          wcscmp(tokens->tokens[*index].value, L"【") == 0) {
        sym->array_num++;
        while (*index + 1 < tokens->count) {
          (*index)++;
          if (wcscmp(tokens->tokens[*index].value, L"]") != 0 &&
              wcscmp(tokens->tokens[*index].value, L"】") != 0) {
            setError(ERR_ARR_TYPE, tokens->tokens[*index].line, NULL);
            return 255;
          }
          if (*index + 1 >= tokens->count) {
            setError(ERR_NO_END, tokens->tokens[*index].line,
                     tokens->tokens[*index].value);
            return 255;
          }
          (*index)++;
          if (tokens->tokens[*index].type == TOK_END) break;
          if (wcscmp(tokens->tokens[*index].value, L"[") == 0 ||
              wcscmp(tokens->tokens[*index].value, L"【") == 0)
            sym->array_num++;
        }
      }
      if (tokens->tokens[*index].type != TOK_END) {
        setError(ERR_NO_END, tokens->tokens[*index - 1].line,
                 tokens->tokens[*index - 1].value);
        return 255;
      }
#ifdef HX_DEBUG
      fwprintf(logStream, L"\33[33m[DEB]数组维数%d\n\33[0m", sym->array_num);
#endif
    } else {
      setError(ERR_NO_END, tokens->tokens[*index].line,
               tokens->tokens[tokens->count - 2].value);
      return 255;
    }
  }
  sym->isOnlyRead = isOnlyRead;
  return 0;
}
// 分析函数定义
static int parseFunDef(Tokens* tokens, int* index, _Function* fun) {
  if (!tokens || !index || !fun) return -1;
#ifdef HX_DEBUG
  log(L"正在分析函数");
#endif
  //<fun><:><id><"("> ... <")"> [<"->"> <id>|<kw>] <"{"> ... <"}">
  // fun:main(void)->int { ... }
  //<"定义函数"><:><标识符><"(">...<")">
  //[<,><"它的返回类型是"><:><标识符>|<关键字>] <"{"> ... <"}">
  if (*index + 1 >= tokens->count) {
    setError(ERR_NO_FUN_NAME, tokens->tokens[*index].line, NULL);
    return 255;
  }
  (*index)++;
  if (wcscmp(tokens->tokens[*index].value, L":") != 0 &&
      wcscmp(tokens->tokens[*index].value, L"：") != 0) {
    setError(ERR_NO_FUN_NAME, tokens->tokens[*index].line, NULL);
    return 255;
  }
  if (*index + 1 >= tokens->count) {
    setError(ERR_NO_FUN_NAME, tokens->tokens[*index].line, NULL);
    return 255;
  }
  (*index)++;
  if (tokens->tokens[*index].type != TOK_ID) {
    setError(ERR_NO_FUN_NAME, tokens->tokens[*index].line, NULL);
    return 255;
  }
  fun->name = (wchar_t*)calloc(wcslen(tokens->tokens[*index].value) + 1,
                               sizeof(wchar_t));
  if (!fun->name) return -1;
  wcscpy(fun->name, tokens->tokens[*index].value);
#ifdef HX_DEBUG
  fwprintf(logStream, L"\33[33m[DEB]已复制函数名(\"%ls\")\n\33[0m", fun->name);
#endif
  // 参数表
  if (*index + 1 >= tokens->count) {
    setError(ERR_FUN, tokens->tokens[*index].line, NULL);
    return 255;
  }
  (*index)++;
  if (wcscmp(tokens->tokens[*index].value, L"(") != 0 &&
      wcscmp(tokens->tokens[*index].value, L"（") != 0) {
    setError(ERR_FUN, tokens->tokens[*index].line, NULL);
    return 255;
  }
  int args_start = *index;  // 含开括号
  while (*index < tokens->count - 1) {
    (*index)++;
    // wprintf(L"L357     %ls\n", tokens->tokens[*index].value);
    if (tokens->tokens[*index].value &&
        (wcscmp(tokens->tokens[*index].value, L")") == 0 ||
         wcscmp(tokens->tokens[*index].value, L"）") == 0)) {
      break;
    }
  }
  // 括号未闭合
  if (wcscmp(tokens->tokens[*index].value, L")") != 0 &&
      wcscmp(tokens->tokens[*index].value, L"）") != 0) {
    // wprintf(L"L357     %ls\n", tokens->tokens[*index].value);
    setError(ERR_FUN, tokens->tokens[args_start].line, NULL);
    return 255;
  }
  // 无参
  if (args_start + 1 == *index) {
#ifdef HX_DEBUG
    log(L"无参数");
#endif
    fun->args = NULL;
    fun->args_size = 0;
  } else if (wcscmp(tokens->tokens[args_start + 1].value, L"void") == 0 ||
             wcscmp(tokens->tokens[args_start + 1].value, L"无参数") == 0) {
#ifdef HX_DEBUG
    log(L"无参数");
#endif
    fun->args = NULL;
    fun->args_size = 0;
  } else {
    int i = args_start;
    i++;
    fun->args = (_Symbol*)calloc(1, sizeof(_Symbol));
    if (!(fun->args)) return -1;
    fun->args_size = 1;
    int args_index = 0;
    while (i < *index) {
      if (args_index >= fun->args_size) {
        fun->args_size = args_index + 1;
        void* temp = realloc(fun->args, (fun->args_size) * sizeof(_Symbol));
        if (!temp) return -1;
        fun->args = (_Symbol*)temp;
        memset(&(fun->args[args_index]), 0, sizeof(_Symbol));
      }
      if (tokens->tokens[i].type != TOK_ID) {
        setError(ERR_FUN_ARG, tokens->tokens[i].line, NULL);
        return 255;
      }
      fun->args[args_index].name = (wchar_t*)calloc(
          wcslen(tokens->tokens[i].value) + 1, sizeof(wchar_t));
      if (!(fun->args[args_index].name)) return -1;
      wcscpy(fun->args[args_index].name, tokens->tokens[i].value);
#ifdef HX_DEBUG
      fwprintf(logStream, L"\33[33m[DEB]已复制参数%d的名字(\"%ls\")\n\33[0m",
               args_index, fun->args[args_index].name);
#endif
      if (i + 1 >= *index) {
        setError(ERR_FUN_ARG, tokens->tokens[i].line, NULL);
        return 255;
      }
      i++;
      if (wcscmp(tokens->tokens[i].value, L":") != 0 &&
          wcscmp(tokens->tokens[i].value, L"：") != 0) {
        setError(ERR_FUN_ARG, tokens->tokens[i].line, NULL);
        return 255;
      }
      // 类型
      if (i + 1 >= *index) {
        setError(ERR_FUN_ARG, tokens->tokens[i].line, NULL);
        return 255;
      }
      i++;
      if (tokens->tokens[i].type != TOK_ID) {
        setError(ERR_FUN_ARG, tokens->tokens[i].line, NULL);
        return 255;
      }
      fun->args[args_index].type = (wchar_t*)calloc(
          wcslen(tokens->tokens[i].value) + 1, sizeof(wchar_t));
      if (!(fun->args[args_index].type)) return -1;
      wcscpy(fun->args[args_index].type, tokens->tokens[i].value);
#ifdef HX_DEBUG
      fwprintf(logStream, L"\33[33m[DEB]已复制参数%d的类型(\"%ls\")\n\33[0m",
               args_index, fun->args[args_index].type);
#endif
      // 可能是数组
      if (i + 1 < *index) {
        i++;
        if (wcscmp(tokens->tokens[i].value, L",") == 0 ||
            wcscmp(tokens->tokens[i].value, L"，") == 0) {
          i++;
          continue;
        } else if (wcscmp(tokens->tokens[i].value, L"[") == 0 ||
                   wcscmp(tokens->tokens[i].value, L"【") == 0) {  // 数组
          fun->args[args_index].array_num++;
          while (i + 1 < *index) {
            i++;
            if (wcscmp(tokens->tokens[i].value, L"]") != 0 &&
                wcscmp(tokens->tokens[i].value, L"】") != 0) {
              setError(ERR_ARR_TYPE, tokens->tokens[i].line, NULL);
              return 255;
            }
            if (i + 1 >= *index) break;
            i++;
            if (wcscmp(tokens->tokens[i].value, L"[") == 0 ||
                wcscmp(tokens->tokens[i].value, L"【") == 0) {
              fun->args[args_index].array_num++;
            } else if (wcscmp(tokens->tokens[i].value, L",") == 0 ||
                       wcscmp(tokens->tokens[i].value, L"，") == 0) {
              break;
            } else {
              setError(ERR_FUN_ARG, tokens->tokens[i].line, NULL);
              return 255;
            }
          }
        }
      }
#ifdef HX_DEBUG
      fwprintf(logStream, L"\33[33m[DEB]参数%d的类型(\"%ls\")的层数:%d\n\33[0m",
               args_index, fun->args[args_index].type,
               fun->args[args_index].array_num);
#endif
      args_index++;
      i++;
    }
    fun->args_size = args_index + 1;
  }
  // 分析返回值或函数体
  if (*index + 1 >= tokens->count) {
    setError(ERR_FUN, tokens->tokens[*index].line, NULL);
    return 255;
  }
  (*index)++;
  // 显式声明返回值
  if (wcscmp(tokens->tokens[*index].value, L"->") == 0 ||
      wcscmp(tokens->tokens[*index].value, L",") == 0 ||
      wcscmp(tokens->tokens[*index].value, L"，") == 0) {
    if (wcscmp(tokens->tokens[*index].value, L",") == 0 ||
        wcscmp(tokens->tokens[*index].value, L"，") == 0) {
      if (*index + 1 >= tokens->count) {
        setError(ERR_FUN, tokens->tokens[*index].line, NULL);
        return 255;
      }
      (*index)++;
      if (wcscmp(tokens->tokens[*index].value, L"它的返回类型是") != 0) {
        setError(ERR_FUN, tokens->tokens[*index].line, NULL);
        return 255;
      }
      if (*index + 1 >= tokens->count) {
        setError(ERR_FUN, tokens->tokens[*index].line, NULL);
        return 255;
      }
      (*index)++;
      if (wcscmp(tokens->tokens[*index].value, L":") != 0 &&
          wcscmp(tokens->tokens[*index].value, L"：") != 0) {
        setError(ERR_FUN, tokens->tokens[*index].line, NULL);
        return 255;
      }
    }
    if (*index + 1 >= tokens->count) {
      setError(ERR_FUN, tokens->tokens[*index].line, NULL);
      return 255;
    }
    (*index)++;
    // 复制返回类型
    fun->isRetTypeKnown = true;
    if (wcscmp(tokens->tokens[*index].value, L"void") == 0 ||
        wcscmp(tokens->tokens[*index].value, L"无返回值") == 0) {
      fun->ret_type = NULL;
#ifdef HX_DEBUG
      log(L"无返回值");
#endif
    } else {
      fun->ret_type = (wchar_t*)calloc(wcslen(tokens->tokens[*index].value) + 1,
                                       sizeof(wchar_t));
      if (!(fun->ret_type)) return -1;
      wcscpy(fun->ret_type, tokens->tokens[*index].value);
#ifdef HX_DEBUG
      fwprintf(logStream, L"\33[32m[DEB]\33[0m 返回%ls\n", fun->ret_type);
#endif
    }
    if (*index + 1 >= tokens->count) {
      setError(ERR_FUN, tokens->tokens[*index].line, NULL);
      return 255;
    }
    (*index)++;
    // 分析数组
    if (wcscmp(tokens->tokens[*index].value, L"{") != 0) {
      if (wcscmp(tokens->tokens[*index].value, L"[") == 0 ||
          wcscmp(tokens->tokens[*index].value, L"【") == 0) {
        while (*index + 1 < tokens->count) {
          fun->ret_type_arr_num++;
          (*index)++;
          if (wcscmp(tokens->tokens[*index].value, L"]") != 0 &&
              wcscmp(tokens->tokens[*index].value, L"】") != 0) {
            setError(ERR_ARR_TYPE, tokens->tokens[*index].line, NULL);
            return 255;
          }
          if (*index + 1 >= tokens->count) {
            setError(ERR_FUN, tokens->tokens[*index].line, NULL);
            return 255;
          }
          (*index)++;
          if (wcscmp(tokens->tokens[*index].value, L"{") == 0)
            break;
          else if (wcscmp(tokens->tokens[*index].value, L"[") == 0 ||
                   wcscmp(tokens->tokens[*index].value, L"【") == 0)
            continue;
          else {
            setError(ERR_ARR_TYPE, tokens->tokens[*index].line, NULL);
            return 255;
          }
        }
      }
    }
  } else if (wcscmp(tokens->tokens[*index].value, L"{") == 0) {
    fun->isRetTypeKnown = false;
  } else {
    setError(ERR_FUN, tokens->tokens[*index].line, NULL);
    return 255;
  }
  // 复制函数体
  int body_start = *index;
  int start = 1;
  int end = 0;
  while (*index + 1 < tokens->count) {
    (*index)++;
    if (wcscmp(tokens->tokens[*index].value, L"fun") == 0 ||
        wcscmp(tokens->tokens[*index].value, L"定义函数") == 0) {
      setError(ERR_FUN, tokens->tokens[*index].line, NULL);
      return 255;
    }
    if (wcscmp(tokens->tokens[*index].value, L"{") == 0) start++;
    if (wcscmp(tokens->tokens[*index].value, L"}") == 0) end++;
    if (start == end) break;
  }
  if (start != end) {
    wchar_t errCode[512] = {0};
    swprintf(errCode, 512, L"%ls %ls", tokens->tokens[body_start - 1].value,
             tokens->tokens[body_start].value);
    setError(ERR_HUAKUOHAO_NOT_CLOSE, tokens->tokens[body_start].line, errCode);
    return 255;
  }
  int body_size = *index - body_start - 1;  // 不复制花括号
  if (body_size == 0) {
    fun->body = NULL;
  } else {
    fun->body = (Tokens*)calloc(1, sizeof(Tokens));
    if (!(fun->body)) return -1;
    fun->body->size = fun->body->count = body_size;
    fun->body->tokens = (Token*)calloc(body_size, sizeof(Token));
    int body_index = 0;
    while (body_start + 1 < *index) {
      body_start++;
      fun->body->tokens[body_index].line = tokens->tokens[body_start].line;
      fun->body->tokens[body_index].mark = tokens->tokens[body_start].mark;
      fun->body->tokens[body_index].type = tokens->tokens[body_start].type;
      fun->body->tokens[body_index].value = (wchar_t*)calloc(
          wcslen(tokens->tokens[body_start].value) + 1, sizeof(wchar_t));
      if (!(fun->body->tokens[body_index].value)) return -1;
      wcscpy(fun->body->tokens[body_index].value,
             tokens->tokens[body_start].value);
      body_index++;
    }
  }
#ifdef HX_DEBUG
  log(L"已复制函数体");
#endif
  return 0;
}
// 辅助函数：检查类中的变/常量是否重复定义
static bool isSymDefinedInClass(wchar_t* name, _Class* class) {
  if (!name || !class) return false;
  if (class->publicMember) {
    if (class->publicMember->syms) {
      for (int i = 0; i < class->publicMember->syms_count; i++) {
        if (class->publicMember->syms[i].name) {
          if (wcscmp(class->publicMember->syms[i].name, name) == 0) return true;
        }
      }
    }
  }
  if (class->privateMember) {
    if (class->privateMember->syms) {
      for (int i = 0; i < class->privateMember->syms_count; i++) {
        if (class->privateMember->syms[i].name) {
          if (wcscmp(class->privateMember->syms[i].name, name) == 0)
            return true;
        }
      }
    }
  }
  if (class->protectedMember) {
    if (class->protectedMember->syms) {
      for (int i = 0; i < class->protectedMember->syms_count; i++) {
        if (class->protectedMember->syms[i].name) {
          if (wcscmp(class->protectedMember->syms[i].name, name) == 0)
            return true;
        }
      }
    }
  }
  return false;
}
// 分析类成员
static int parseClassMember(Tokens* tokens, int* index, int end_index,
                            _ClassMember* mem, _Class* class) {
  if (tokens == NULL || index == NULL || mem == NULL) return -1;
  while (*index < end_index) {
    // wprintf(L"L341   %ls\n", tokens->tokens[*index].value);
    if (wcscmp(tokens->tokens[*index].value, L"var") == 0 ||
        wcscmp(tokens->tokens[*index].value, L"定义变量") == 0 ||
        wcscmp(tokens->tokens[*index].value, L"con") == 0 ||
        wcscmp(tokens->tokens[*index].value, L"定义常量") == 0) {
      if (mem->syms == NULL) {
        mem->syms = (_Symbol*)calloc(1, sizeof(_Symbol));
        if (!(mem->syms)) return -1;
        mem->syms_size = 1;
        mem->syms_count = 0;
      }
      if (mem->syms_size <= mem->syms_count) {
        mem->syms_size = mem->syms_count + 1;
        void* temp = realloc(mem->syms, (mem->syms_size) * sizeof(_Symbol));
        if (!temp) return -1;
        mem->syms = (_Symbol*)temp;
        memset(&(mem->syms[mem->syms_count]), 0, sizeof(_Symbol));
      }
      int err = parseVarDef(tokens, index, &(mem->syms[mem->syms_count]));
      if (err) return err;
      if (isSymDefinedInClass(mem->syms[mem->syms_count].name, class)) {
        setError(ERR_DEF_CLASS_DOUBLE_DEFINED_SYM, tokens->tokens[*index].line,
                 mem->syms[mem->syms_count].name);
        return 255;
      }
      mem->syms_count++;
    } else if (wcscmp(tokens->tokens[*index].value, L"fun") == 0 ||
               wcscmp(tokens->tokens[*index].value, L"定义函数") == 0) {
      if (mem->funcs == NULL) {
        mem->funcs = (_Function*)calloc(1, sizeof(_Function));
        if (!(mem->funcs)) return -1;
        mem->funcs_size = 1;
        mem->funcs_count = 0;
      }
      if (mem->funcs_size <= mem->funcs_count) {
        mem->funcs_size = mem->funcs_count + 1;
        void* temp = realloc(mem->funcs, (mem->funcs_size) * sizeof(_Function));
        if (!temp) return -1;
        mem->funcs = (_Function*)temp;
        memset(&(mem->funcs[mem->funcs_count]), 0, sizeof(_Function));
      }

      int err = parseFunDef(tokens, index, &(mem->funcs[mem->funcs_count]));
      if (err) return err;
    }
    (*index)++;
  }
  return 0;
}
// 分析不带继承的类
static int parseClassNotExtend(Tokens* tokens, int* index, _Class* class) {
  if (tokens == NULL || index == NULL || class == NULL) return -1;
#ifdef HX_DEBUG
  log(L"分析类");
#endif
  int token_size = tokens->count;
  int huakuohao_start = 1, huakuohao_end = 0;
  int body_start = *index;
  while (!getNextToken(index, token_size)) {  // 找类体范围
    if (wcscmp(tokens->tokens[*index].value, L"{") == 0) huakuohao_start++;
    if (wcscmp(tokens->tokens[*index].value, L"}") == 0) huakuohao_end++;
    if (huakuohao_start == huakuohao_end) break;
  }
  if (huakuohao_end != huakuohao_start) {  // 花括号未闭合
    wchar_t* errCode =
        (wchar_t*)calloc(wcslen(tokens->tokens[body_start].value) +
                             wcslen(tokens->tokens[body_start - 1].value) + 3,
                         sizeof(wchar_t));
    if (!errCode) return -1;
    wcscpy(errCode, tokens->tokens[body_start - 1].value);
    wcscat(errCode, L" ");
    wcscat(errCode, tokens->tokens[body_start].value);
    setError(ERR_HUAKUOHAO_NOT_CLOSE, tokens->tokens[body_start].line, errCode);
    free(errCode);
    return 255;
  }
  // 此时index指向类体末尾的花括号
  int body_end = *index;
  body_start++;
  // wprintf(L"%ls\n", tokens->tokens[body_start].value);
  int p = body_start;
  while (p < body_end) {
    // wprintf(L"L398  %ls\n", tokens->tokens[p].value);
    if (wcscmp(tokens->tokens[p].value, L"public") == 0 ||
        wcscmp(tokens->tokens[p].value, L"公有成员") == 0) {
#ifdef HX_DEBUG
      log(L"分析公有成员");
#endif
      // 公有成员
      //  <"public"!"private"|"protected"> <"{"> ... <"}">
      if (p >= body_end) {
        setError(ERR_DEF_CLASS_ACCESS, tokens->tokens[p].line, NULL);
        return 255;
      }
      p++;
      if (wcscmp(tokens->tokens[p].value, L"{") != 0) {
        setError(ERR_DEF_CLASS_ACCESS, tokens->tokens[p].line, NULL);
        return 255;
      }
      int huakuohao_start = 0;
      int huakuohao_end = 0;
      int huakuohao_start_index = p;
      while (p < body_end) {
        if (wcscmp(tokens->tokens[p].value, L"{") == 0) huakuohao_start++;
        if (wcscmp(tokens->tokens[p].value, L"}") == 0) huakuohao_end++;
        if (huakuohao_start == huakuohao_end) break;
        p++;
      }
      if (huakuohao_start != huakuohao_end) {
        wchar_t* errCode = (wchar_t*)calloc(
            wcslen(tokens->tokens[huakuohao_start_index].value) +
                wcslen(tokens->tokens[huakuohao_start_index - 1].value) + 3,
            sizeof(wchar_t));
        if (!errCode) return -1;
        wcscpy(errCode, tokens->tokens[huakuohao_start_index - 1].value);
        wcscat(errCode, L" ");
        wcscat(errCode, tokens->tokens[huakuohao_start_index].value);
        setError(ERR_HUAKUOHAO_NOT_CLOSE,
                 tokens->tokens[huakuohao_start_index].line, errCode);
        free(errCode);
        return 255;
      }
      int end = p;
      if (end == huakuohao_start_index + 1) {
        p++;
        continue;
      }
      if (class->publicMember == NULL) {
        class->publicMember = (_ClassMember*)calloc(1, sizeof(_ClassMember));
        if (!(class->publicMember)) return -1;
      }
      // 分析成员
      int err = parseClassMember(tokens, &huakuohao_start_index, end,
                                 class->publicMember, class);
      if (err) return err;
    } else if (wcscmp(tokens->tokens[p].value, L"private") == 0 ||
               wcscmp(tokens->tokens[p].value, L"私有成员") == 0) {
#ifdef HX_DEBUG
      log(L"分析私有成员");
#endif
      // 私有成员
      if (p >= body_end) {
        setError(ERR_DEF_CLASS_ACCESS, tokens->tokens[p].line, NULL);
        return 255;
      }
      p++;
      if (wcscmp(tokens->tokens[p].value, L"{") != 0) {
        setError(ERR_DEF_CLASS_ACCESS, tokens->tokens[p].line, NULL);
        return 255;
      }
      int huakuohao_start = 0;
      int huakuohao_end = 0;
      int huakuohao_start_index = p;
      while (p < body_end) {
        if (wcscmp(tokens->tokens[p].value, L"{") == 0) huakuohao_start++;
        if (wcscmp(tokens->tokens[p].value, L"}") == 0) huakuohao_end++;
        if (huakuohao_start == huakuohao_end) break;
        p++;
      }
      if (huakuohao_start != huakuohao_end) {
        wchar_t* errCode = (wchar_t*)calloc(
            wcslen(tokens->tokens[huakuohao_start_index].value) +
                wcslen(tokens->tokens[huakuohao_start_index - 1].value) + 3,
            sizeof(wchar_t));
        if (!errCode) return -1;
        wcscpy(errCode, tokens->tokens[huakuohao_start_index - 1].value);
        wcscat(errCode, L" ");
        wcscat(errCode, tokens->tokens[huakuohao_start_index].value);
        setError(ERR_HUAKUOHAO_NOT_CLOSE,
                 tokens->tokens[huakuohao_start_index].line, errCode);
        free(errCode);
        return 255;
      }
      int end = p;
      if (end == huakuohao_start_index + 1) {
        p++;
        continue;
      }
      if (class->privateMember == NULL) {
        class->privateMember = (_ClassMember*)calloc(1, sizeof(_ClassMember));
        if (!(class->privateMember)) return -1;
      }
      // 分析成员
      int err = parseClassMember(tokens, &huakuohao_start_index, end,
                                 class->privateMember, class);
      if (err) return err;
    } else if (wcscmp(tokens->tokens[p].value, L"protected") == 0 ||
               wcscmp(tokens->tokens[p].value, L"受保护成员") == 0) {
#ifdef HX_DEBUG
      log(L"分析受保护成员");
#endif
      // 受保护成员
      if (p >= body_end) {
        setError(ERR_DEF_CLASS_ACCESS, tokens->tokens[p].line, NULL);
        return 255;
      }
      p++;
      if (wcscmp(tokens->tokens[p].value, L"{") != 0) {
        setError(ERR_DEF_CLASS_ACCESS, tokens->tokens[p].line, NULL);
        return 255;
      }
      int huakuohao_start = 0;
      int huakuohao_end = 0;
      int huakuohao_start_index = p;
      while (p < body_end) {
        if (wcscmp(tokens->tokens[p].value, L"{") == 0) huakuohao_start++;
        if (wcscmp(tokens->tokens[p].value, L"}") == 0) huakuohao_end++;
        if (huakuohao_start == huakuohao_end) break;
        p++;
      }
      if (huakuohao_start != huakuohao_end) {
        wchar_t* errCode = (wchar_t*)calloc(
            wcslen(tokens->tokens[huakuohao_start_index].value) +
                wcslen(tokens->tokens[huakuohao_start_index - 1].value) + 3,
            sizeof(wchar_t));
        if (!errCode) return -1;
        wcscpy(errCode, tokens->tokens[huakuohao_start_index - 1].value);
        wcscat(errCode, L" ");
        wcscat(errCode, tokens->tokens[huakuohao_start_index].value);
        setError(ERR_HUAKUOHAO_NOT_CLOSE,
                 tokens->tokens[huakuohao_start_index].line, errCode);
        free(errCode);
        return 255;
      }
      int end = p;
      if (end == huakuohao_start_index + 1) {
        p++;
        continue;
      }
      if (class->protectedMember == NULL) {
        class->protectedMember = (_ClassMember*)calloc(1, sizeof(_ClassMember));
        if (!(class->protectedMember)) return -1;
      }
      // 分析成员
      int err = parseClassMember(tokens, &huakuohao_start_index, end,
                                 class->protectedMember, class);
      if (err) return err;
    } else {
#ifdef HX_DEBUG
      // fwprintf(logStream, L"%ls\n", tokens->tokens[p].value);
      log(L"默认为私有成员");
#endif
      // 默认为私有成员
      if (wcscmp(tokens->tokens[p].value, L"var") == 0 ||
          wcscmp(tokens->tokens[p].value, L"定义变量") == 0 ||
          wcscmp(tokens->tokens[p].value, L"con") == 0 ||
          wcscmp(tokens->tokens[p].value, L"定义常量") == 0) {
        if (class->privateMember == NULL) {
          class->privateMember = (_ClassMember*)calloc(1, sizeof(_ClassMember));
          if (!(class->privateMember)) return -1;
        }
        if (class->privateMember->syms == NULL) {
          class->privateMember->syms = (_Symbol*)calloc(1, sizeof(_Symbol));
          if (!(class->privateMember->syms)) return -1;
          class->privateMember->syms_size = 1;
          class->privateMember->syms_count = 0;
        }
        if (class->privateMember->syms_size <=
            class->privateMember->syms_count) {
          class->privateMember->syms_size =
              class->privateMember->syms_count + 1;
          void* temp =
              realloc(class->privateMember->syms,
                      (class->privateMember->syms_size) * sizeof(_Symbol));
          if (!temp) return -1;
          class->privateMember->syms = (_Symbol*)temp;
          memset(
              &(class->privateMember->syms[class->privateMember->syms_count]),
              0, sizeof(_Symbol));
        }
        int err = parseVarDef(
            tokens, &p,
            &(class->privateMember->syms[class->privateMember->syms_count]));
        if (err) return err;
        if (isSymDefinedInClass(
                class->privateMember->syms[class->privateMember->syms_count]
                    .name,
                class)) {
          setError(ERR_DEF_CLASS_DOUBLE_DEFINED_SYM, tokens->tokens[p].line,
                   class->privateMember->syms[class->privateMember->syms_count]
                       .name);
          return 255;
        }
        class->privateMember->syms_count++;
      } else if (wcscmp(tokens->tokens[p].value, L"fun") == 0 ||
                 wcscmp(tokens->tokens[p].value, L"定义函数") == 0) {
        // printf("hello\n");
        if (class->privateMember->funcs == NULL) {
          class->privateMember->funcs =
              (_Function*)calloc(1, sizeof(_Function));
          if (!(class->privateMember->funcs)) return -1;
          class->privateMember->funcs_size = 1;
          class->privateMember->funcs_count = 0;
        }
        if (class->privateMember->funcs_size <=
            class->privateMember->funcs_count) {
          class->privateMember->funcs_size =
              class->privateMember->funcs_count + 1;
          void* temp =
              realloc(class->privateMember->funcs,
                      (class->privateMember->funcs_size) * sizeof(_Function));
          if (!temp) return -1;
          class->privateMember->funcs = (_Function*)temp;
          memset(
              &(class->privateMember->funcs[class->privateMember->funcs_count]),
              0, sizeof(_Function));
        }
        int err = parseFunDef(
            tokens, &p,
            &(class->privateMember->funcs[class->privateMember->funcs_count]));
        if (err) return err;
      }
    }
    p++;
  }
  return 0;
}
/*********清理部分************/
static void freeSymbol(_Symbol* sym) {
  if (!sym) return;
#ifdef HX_DEBUG
  fwprintf(logStream, L"\33[33m[DEB] 正在释放符号(%ls)\33[0m\n",
           sym->name ? sym->name : L"(null)");
#endif
  if (sym->name) free(sym->name);
  sym->name = NULL;
  if (sym->type) free(sym->type);
  sym->type = NULL;
  return;
}
static void freeFunction(_Function* fun) {
  if (!fun) return;
#ifdef HX_DEBUG
  fwprintf(logStream, L"\33[33m[DEB] 正在释放函数(%ls)\33[0m\n",
           fun->name ? fun->name : L"(null)");
#endif
  if (fun->name) free(fun->name);
  fun->name = NULL;
  if (fun->ret_type) free(fun->ret_type);
  fun->ret_type = 0;
  if (fun->body) {
    freeTokens(&(fun->body));
    free(fun->body);
    fun->body = NULL;
  }
  if (fun->args) {
    for (int i = 0; i < fun->args_size; i++) {
      freeSymbol(&(fun->args[i]));
    }
    free(fun->args);
    fun->args = NULL;
  }
  return;
}
static void freeClass(_Class* class) {
  if (!class) return;
#ifdef HX_DEBUG
  fwprintf(logStream, L"\33[33m[DEB] 正在释放类(%ls)\33[0m\n",
           class->name ? class->name : L"(null)");
#endif
  if (class->name) free(class->name);
  class->name = NULL;
  if (class->parent_name) free(class->parent_name);
  class->parent_name = NULL;
  if (class->publicMember) {
#ifdef HX_DEBUG
    log(L"释放公有成员");
#endif
    if (class->publicMember->syms) {
      for (int i = 0; i < class->publicMember->syms_size; i++) {
        freeSymbol(&(class->publicMember->syms[i]));
      }
      free(class->publicMember->syms);
      class->publicMember->syms = NULL;
    }
    if (class->publicMember->funcs) {
      for (int i = 0; i < class->publicMember->funcs_size; i++) {
        freeFunction(&(class->publicMember->funcs[i]));
      }
      free(class->publicMember->funcs);
      class->publicMember->funcs = NULL;
    }
    free(class->publicMember);
    class->publicMember = NULL;
  }
  if (class->privateMember) {
#ifdef HX_DEBUG
    log(L"释放私有成员");
#endif
    if (class->privateMember->syms) {
      for (int i = 0; i < class->privateMember->syms_size; i++) {
        freeSymbol(&(class->privateMember->syms[i]));
      }
      free(class->privateMember->syms);
      class->privateMember->syms = NULL;
    }
    if (class->privateMember->funcs) {
      for (int i = 0; i < class->privateMember->funcs_size; i++) {
        freeFunction(&(class->privateMember->funcs[i]));
      }
      free(class->privateMember->funcs);
      class->privateMember->funcs = NULL;
    }
    free(class->privateMember);
    class->privateMember = NULL;
  }
  if (class->protectedMember) {
#ifdef HX_DEBUG
    log(L"释放受保护成员");
#endif
    if (class->protectedMember->syms) {
      for (int i = 0; i < class->protectedMember->syms_size; i++) {
        freeSymbol(&(class->protectedMember->syms[i]));
      }
      free(class->protectedMember->syms);
      class->protectedMember->syms = NULL;
    }
    if (class->protectedMember->funcs) {
      for (int i = 0; i < class->protectedMember->funcs_size; i++) {
        freeFunction(&(class->protectedMember->funcs[i]));
      }
      free(class->protectedMember->funcs);
      class->protectedMember->funcs = NULL;
    }
    free(class->protectedMember);
    class->protectedMember = NULL;
  }
}
void freeIR_1(IR_1** ir) {
  if (!ir) return;
  if (!(*ir)) return;
  if ((*ir)->classes) {
    for (int i = 0; i < (*ir)->class_size; i++) {
      freeClass(&((*ir)->classes[i]));
    }
    free((*ir)->classes);
    (*ir)->classes = NULL;
  }
  if ((*ir)->funcs) {
    for (int i = 0; i < (*ir)->funcs_size; i++) {
      freeFunction(&((*ir)->funcs[i]));
    }
    free((*ir)->funcs);
    (*ir)->funcs = NULL;
  }
  free(*ir);
  *ir = NULL;
#ifdef HX_DEBUG
  log(L"已释放IR_1");
#endif
  return;
}
#endif