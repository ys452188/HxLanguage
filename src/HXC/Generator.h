// 目标代码生成
#ifndef HXLANG_GENERATOR_H
#define HXLANG_GENERATOR_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "Error.h"
#include "Lexer.h"
#include "Parser.h"
#include "Pass.h"
typedef enum OpCode {  // 指令码
  ADD,                 // 加
  SUB,                 // 减
  MUL,                 // 乘
  DIV,                 // 除
  MOVE,                // 赋值
  PUSH,                // 压栈
  CALL,                // 调用函数
  LOAD,                // 加载变量到栈       LOAD <size>
} OpCode;
typedef struct OpArgument {  // 操作数
  enum {
    INT,
    CHAR,
    STRING,
    FLOAT,
    DOUBLE,
    INDEX,
    TYPE,
  } type;
  union {
    uint32_t index;
    int32_t i32_val;
    wchar_t* string;
    wchar_t ch;
    float float_val;
    double double_val;
    char only_type;  // 仅用于表示类型
  } value;
} OpArgument;
typedef struct Command {  // 指令
  OpCode op;
  OpArgument* args;
  uint8_t args_size;
} Command;
typedef struct Symbol {
  wchar_t* name;
  wchar_t* type;
  int type_arr_num;
  bool isOnlyRead;
  bool isUsed;
  int sp;         // 栈中的位置
  int cmd_index;  // 加载它的指令的位置,确定变量类型后记得改它
} Symbol;
typedef struct Function {
  bool isUseless;  // 是否无用

  wchar_t* name;
  wchar_t* ret_type;
  uint32_t ret_type_arr_num;
  Symbol* args;
  uint32_t args_size;

  Command* body;
  uint32_t body_size;
} Function;
typedef struct HxCode {
  Function* funs;
  uint32_t funs_size;
} HxCode;
static int genFunction(_Function* fun, Function* obj, Function* table,
                       int table_size);
extern int gen(IR_1* ir, HxCode* obj) {
  if (!ir | !obj) return -1;
  if (ir->funcs == NULL || ir->funcs_size == 0) {
    setError(ERR_NO_MAIN, 0, NULL);
    return 255;
  }
  // 编译函数
  obj->funs = (Function*)calloc(ir->funcs_size, sizeof(Function));
  if (!(obj->funs)) return -1;
  // 先复制声明
#ifdef HX_DEBUG
  log(L"复制函数声明");
#endif
  for (int i = 0; i < ir->funcs_size; i++) {
    // 名
    obj->funs[i].name =
        (wchar_t*)calloc(wcslen(ir->funcs[i].name) + 1, sizeof(wchar_t));
    if (!(obj->funs[i].name)) return -1;
    wcscpy(obj->funs[i].name, ir->funcs[i].name);
#ifdef HX_DEBUG
    fwprintf(logStream, L"\33[33m[DEG]复制函数声明 %ls\n\33[0m",
             obj->funs[i].name);
#endif
    // 返
    obj->funs[i].ret_type = NULL;
    if (ir->funcs[i].ret_type != NULL) {
      obj->funs[i].ret_type =
          (wchar_t*)calloc(wcslen(ir->funcs[i].ret_type) + 1, sizeof(wchar_t));
      if (!(obj->funs[i].ret_type)) {
        return -1;
      }
      wcscpy(obj->funs[i].ret_type, ir->funcs[i].ret_type);
    }
    obj->funs[i].ret_type_arr_num = ir->funcs[i].ret_type_arr_num;
    // 参
    obj->funs[i].args = NULL;
    obj->funs[i].args_size = ir->funcs[i].args_size;
    if (ir->funcs[i].args != NULL) {
      obj->funs[i].args =
          (Symbol*)calloc(obj->funs[i].args_size, sizeof(Symbol));
      if (!(obj->funs[i].args)) {
        return -1;
      }
      for (int j = 0; j < obj->funs[i].args_size; j++) {
        obj->funs[i].args[j].name = (wchar_t*)calloc(
            wcslen(ir->funcs[i].args[j].name) + 1, sizeof(wchar_t));
        if (!(obj->funs[i].args[j].name)) {
          return -1;
        }
        wcscpy(obj->funs[i].args[j].name, ir->funcs[i].args[j].name);
        obj->funs[i].args[j].type = (wchar_t*)calloc(
            wcslen(ir->funcs[i].args[j].type) + 1, sizeof(wchar_t));
        if (!(obj->funs[i].args[j].type)) {
          return -1;
        }
        wcscpy(obj->funs[i].args[j].type, ir->funcs[i].args[j].type);
        obj->funs[i].args[j].type_arr_num = ir->funcs[i].args[j].array_num;
      }
    }
  }
#ifdef HX_DEBUG
  log(L"生成函数体的目标代码");
#endif
  for (int i = 0; i < ir->funcs_size; i++) {
    int err = genFunction(&(ir->funcs[i]), &(obj->funs[i]), obj->funs,
                          obj->funs_size);
    if (err) return err;
  }
  return 0;
}
static void freeSymbolTable(Symbol* table, int size) {
  if (!table) return;
  for (int i = 0; i < size; i++) {
    if (table[i].name) {
      free(table[i].name);
      table[i].name = NULL;
    }
    if (table[i].type) {
      free(table[i].type);
      table[i].type = NULL;
    }
  }
  free(table);
  table = NULL;
  return;
}
static int genVarDef(Tokens* tokens, int* index, Command** cmd, int* cmd_index,
                     int* cmd_size, Symbol* sym) {
  if (tokens == NULL || index == NULL || !cmd_index || !cmd_size) return -1;
#ifdef HX_DEBUG
  log(L"分析变量的定义");
#endif
  sym->isOnlyRead = false;
  // var:<id> "->" <id>|<kw>   var:v1->int
  bool isOnlyRead = false;
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
      // 生成load指令  (    load <size>     )
      if (*cmd == NULL) {
        *cmd = (Command*)calloc(1, sizeof(Command));
        if (!(*cmd)) return -1;
        *cmd_size = 1;
      }
      if (*cmd_size <= *cmd_index) {
        *cmd_size = *cmd_index + 1;
        void* temp = realloc(*cmd, sizeof(Command) * (*cmd_size));
        if (!temp) return -1;
        *cmd = (Command*)temp;
        memset(&(*cmd[*cmd_index]), 0, sizeof(Command));
      }
      (*cmd)[*cmd_index].args = (OpArgument*)calloc(1, sizeof(OpArgument));
      if (!((*cmd)[*cmd_index].args)) return -1;
      (*cmd)[*cmd_index].args_size = 1;
      (*cmd)[*cmd_index].args[0].type = INT;
      // 确定变量大小
      // 类型未知,暂不处理
      sym->cmd_index = *cmd_index;
#ifdef HX_DEBUG
      fwprintf(logStream, L"\33[33m[DEB]已生成指令load %d(类型未知)\n\33[0m",
               (int)((*cmd)[*cmd_index].args[0].value.i32_val));
#endif
      (*cmd_index)++;
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
        sym->type_arr_num++;
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
            sym->type_arr_num++;
        }
      }
      if (tokens->tokens[*index].type != TOK_END &&
          !(wcscmp(tokens->tokens[*index].value, L"=") == 0)) {
        setError(ERR_NO_END, tokens->tokens[*index - 1].line,
                 tokens->tokens[*index - 1].value);
        return 255;
      }
#ifdef HX_DEBUG
      fwprintf(logStream, L"\33[33m[DEB]数组维数%d\n\33[0m", sym->type_arr_num);
#endif
    } else if (tokens->tokens[*index].type != TOK_END &&
               !(wcscmp(tokens->tokens[*index].value, L"=") == 0)) {
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
      // 生成load指令  (    load <size>     )
      if (*cmd == NULL) {
        *cmd = (Command*)calloc(1, sizeof(Command));
        if (!(*cmd)) return -1;
        *cmd_size = 1;
      }
      if (*cmd_size <= *cmd_index) {
        *cmd_size = *cmd_index + 1;
        void* temp = realloc(*cmd, sizeof(Command) * (*cmd_size));
        if (!temp) return -1;
        *cmd = (Command*)temp;
        memset(&(*cmd[*cmd_index]), 0, sizeof(Command));
      }
      (*cmd)[*cmd_index].args = (OpArgument*)calloc(1, sizeof(OpArgument));
      if (!((*cmd)[*cmd_index].args)) return -1;
      (*cmd)[*cmd_index].args_size = 1;
      (*cmd)[*cmd_index].args[0].type = INT;
      // 确定变量大小
      // 类型未知,暂不处理
      sym->cmd_index = *cmd_index;
#ifdef HX_DEBUG
      fwprintf(logStream, L"\33[33m[DEB]已生成指令load %d(类型未知)\n\33[0m",
               (int)((*cmd)[*cmd_index].args[0].value.i32_val));
#endif
      (*cmd_index)++;
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
        sym->type_arr_num++;
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
            sym->type_arr_num++;
        }
      }
      if (tokens->tokens[*index].type != TOK_END &&
          !(wcscmp(tokens->tokens[*index].value, L"=") == 0)) {
        setError(ERR_NO_END, tokens->tokens[*index - 1].line,
                 tokens->tokens[*index - 1].value);
        return 255;
      }
#ifdef HX_DEBUG
      fwprintf(logStream, L"\33[33m[DEB]数组维数%d\n\33[0m", sym->type_arr_num);
#endif
    } else if (tokens->tokens[*index].type != TOK_END &&
               !(wcscmp(tokens->tokens[*index].value, L"=") == 0)) {
      setError(ERR_NO_END, tokens->tokens[*index].line,
               tokens->tokens[tokens->count - 2].value);
      return 255;
    }
  }
  // 生成load指令  (    load <size>     )
#ifdef HX_DEBUG
  log(L"生成load指令");
#endif
  if (*cmd == NULL) {
    *cmd = (Command*)calloc(1, sizeof(Command));
    if (!(*cmd)) return -1;
    *cmd_size = 1;
  }
  if (*cmd_size <= *cmd_index) {
    *cmd_size = *cmd_index + 1;
    void* temp = realloc(*cmd, sizeof(Command) * (*cmd_size));
    if (!temp) return -1;
    *cmd = (Command*)temp;
    memset(&(*cmd[*cmd_index]), 0, sizeof(Command));
  }
  (*cmd)[*cmd_index].args = (OpArgument*)calloc(1, sizeof(OpArgument));
  if (!((*cmd)[*cmd_index].args)) return -1;
  (*cmd)[*cmd_index].args_size = 1;
  (*cmd)[*cmd_index].args[0].type = INT;

  // 确定变量大小
  sym->cmd_index = *cmd_index;
  if (sym->type != NULL) {
    if (wcscmp(sym->type, L"int") == 0 || wcscmp(sym->type, L"整型") == 0) {
      (*cmd)[*cmd_index].args[0].value.i32_val = sizeof(int32_t);
    } else if (wcscmp(sym->type, L"char") == 0 ||
               wcscmp(sym->type, L"字符型") == 0) {
      (*cmd)[*cmd_index].args[0].value.i32_val = sizeof(uint16_t);
    } else if (wcscmp(sym->type, L"str") == 0 ||
               wcscmp(sym->type, L"字符串型") == 0) {
      (*cmd)[*cmd_index].args[0].value.string = NULL;
    }
  }

#ifdef HX_DEBUG
  fwprintf(logStream, L"\33[33m[DEB]已生成指令load %d\n\33[0m",
           (int)((*cmd)[*cmd_index].args[0].value.i32_val));
#endif
  (*cmd_index)++;
  sym->isOnlyRead = isOnlyRead;
  return 0;
}
// 生成函数的目标代码
static int genFunction(_Function* fun, Function* obj, Function* table,
                       int table_size) {
  if (!obj) return -1;
  if (fun == NULL) return -1;
#ifdef HX_DEBUG
  fwprintf(logStream, L"\33[33m[DEB]=========分析函数(%ls)=========\n\33[0m",
           fun->name);
#endif
  Symbol* localeTable = NULL;
  int localeTable_size = 0;
  int localeTable_index = 0;
  // 将参数加入局部符号表
  if (fun->args != NULL) {
    localeTable_size = fun->args_size;
    localeTable = (Symbol*)calloc(localeTable_size, sizeof(Symbol));
    if (!localeTable) return -1;
    for (localeTable_index = 0; localeTable_index < localeTable_size;
         localeTable_index++) {
      localeTable[localeTable_index].name = (wchar_t*)calloc(
          wcslen(fun->args[localeTable_index].name) + 1, sizeof(wchar_t));
      if (!localeTable[localeTable_index].name) {
        freeSymbolTable(localeTable, localeTable_size);
        return -1;
      }
      wcscpy(localeTable[localeTable_index].name,
             fun->args[localeTable_index].name);
      localeTable[localeTable_index].type = (wchar_t*)calloc(
          wcslen(fun->args[localeTable_index].type) + 1, sizeof(wchar_t));
      if (!localeTable[localeTable_index].type) {
        freeSymbolTable(localeTable, localeTable_size);
        return -1;
      }
      wcscpy(localeTable[localeTable_index].type,
             fun->args[localeTable_index].type);
      localeTable[localeTable_index].type_arr_num =
          fun->args[localeTable_index].array_num;
    }
  } else {
    localeTable = (Symbol*)calloc(1, sizeof(Symbol));
    if (!localeTable) return -1;
    localeTable_size = 1;
  }
  if (fun->body == NULL) {
    obj->isUseless = true;
  } else {
    int body_index = 0;
    int obj_body_index = 0;
    while (body_index < fun->body->count) {
      if (wcscmp(fun->body->tokens[body_index].value, L"var") == 0 ||
          wcscmp(fun->body->tokens[body_index].value, L"con") == 0 ||
          wcscmp(fun->body->tokens[body_index].value, L"定义变量") == 0 ||
          wcscmp(fun->body->tokens[body_index].value, L"定义常量") == 0) {
        if (localeTable_size <= localeTable_index) {
          void* temp =
              realloc(localeTable, (localeTable_index + 1) * sizeof(Symbol));
          if (!temp) {
            freeSymbolTable(localeTable, localeTable_size);
            return -1;
          }
          localeTable_size = localeTable_index + 1;
          localeTable = (Symbol*)temp;
          memset(&(localeTable[localeTable_index]), 0, sizeof(Symbol));
        }
        int err =
            genVarDef(fun->body, &body_index, &(obj->body), &obj_body_index,
                      &(obj->body_size), &(localeTable[localeTable_index]));
        if (err) return err;
        localeTable_index++;
      } else if (fun->body->tokens[body_index].type == TOK_ID) {
        if (body_index + 1 >= fun->body->count) {
          setError(ERR_NO_END, fun->body->tokens[body_index].line,
                   fun->body->tokens[body_index].value);
          return 255;
        }
        body_index++;
        if (fun->body->tokens[body_index].type == TOK_END) {  // ID;  啥用也没
          body_index++;
          continue;
        }
      } else {
        body_index++;
      }
    }
  }
  freeSymbolTable(localeTable, localeTable_size);
  return 0;
}
void freeObject(HxCode** obj) {
  if (!obj) return;
  if (*obj) {
    free(*obj);
  }
  *obj = NULL;
  return;
}
#endif