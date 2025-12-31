#ifndef HXLANG_SRC_HXC_IR_H
#define HXLANG_SRC_HXC_IR_H
#include "Error.h"
#include "Lexer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <wchar.h>

typedef enum IR_DataTypeKind {
  IR_DT_INT,
  IR_DT_FLOAT,
  IR_DT_STRING,
  IR_DT_CHAR,
  IR_DT_BOOL,
  IR_DT_VOID,
  IR_DT_CUSTOM
} IR_DataTypeKind;

typedef struct IR_DataType {
  IR_DataTypeKind kind;
  wchar_t *custom_type_name; // 当kind为IR_DT_CUSTOM 时使用
} IR_DataType;
typedef struct IR_FunctionParam {
  wchar_t *name;
  IR_DataType type;
} IR_FunctionParam;
typedef struct IR_Function {
  wchar_t *name;
  IR_FunctionParam *params;
  int param_count;
  bool isReturnTypeKnown;
  IR_DataType return_type;
  Token *body_tokens; // 函数体的Token流
  int body_token_count;
} IR_Function;
typedef struct IR_Variable {
  wchar_t *name;
  IR_DataType type;
  bool isOnlyRead;
  int address; // 变量在符号号表中的地址
} IR_Variable;
typedef union IR_ClassMember {
  IR_Variable variable;
  IR_Function function;
} IR_ClassMember;
typedef struct IR_Class {
  wchar_t *name;
  int fatherIndex; // 父类在类表中的索引，-1表示无父类

  IR_ClassMember *publicMembers;
  int public_member_count;
  IR_ClassMember *privateMembers;
  int private_member_count;
  IR_ClassMember *protectedMembers;
  int protected_member_count;
} IR_Class;

typedef struct IR_Program {
  IR_Variable *global_variables;
  int global_variable_count;
  IR_Function **functions;
  int function_count;
  IR_Class *classes;
  int class_count;
} IR_Program;

#ifdef HX_DEBUG
void showFunctionInfo(IR_Function *function);
#endif
/**
 * 解析函数定义
 * @param tokens 词法分析得到的Token流
 * @param index 当前解析到的Token索引，解析完成后会更新为下一个未解析的Token索引
 * @param err 错误码指针，发生错误时会设置为相应错误码
 */
IR_Function *parseFunction(Tokens *tokens, int *index, int *err);
/**
 * 解析类定义
 * @param tokens 词法分析得到的Token流
 * @param index 当前解析到的Token索引，解析完成后会更新为下一个未解析的Token索引
 * @param err 错误码指针，发生错误时会设置为相应错误码
 */
IR_Class *parseClass(Tokens *tokens, int *index, int *err);
/** 生成中间表示
 * @param tokens 词法分析得到的Token流
 * @param err 错误码指针，发生错误时会设置为相应错误码
 */
IR_Program *generateIR(Tokens *tokens, int *err); // 生成中间表示

IR_Program *generateIR(Tokens *tokens, int *err) {
  if (!tokens) {
    if (err)
      *err = -1;
    return NULL;
  }
  if (tokens->count == 0) {
    *err = 225;
    setError(ERR_NO_MAIN, 0, NULL);
    return NULL;
  }
  IR_Program *program = (IR_Program *)calloc(1, sizeof(IR_Program));
  if (!program) {
    if (err)
      *err = -1;
    return NULL;
  }
  int index = 0;
  while (index < tokens->count) {
    // 解析全局变量、函数或类定义
    if (tokens->tokens[index].type == TOK_KW) {
      if (wcscmp(tokens->tokens[index].value, L"定义函数") == 0 ||
          wcscmp(tokens->tokens[index].value, L"function") == 0) {
#ifdef HX_DEBUG
        log(L"解析到一个函数定义");
#endif
        IR_Function *func = parseFunction(tokens, &index, err);
        if (!func) {
          // 解析函数出错
          free(program);
          return NULL;
        }
        program->function_count++;
        program->functions = (IR_Function **)realloc(program->functions,
                                                     program->function_count *
                                                         sizeof(IR_Function *));
        if (!program->functions) {
          if (err)
            *err = -1;
          free(func);
          free(program);
          return NULL;
        }
        program->functions[program->function_count - 1] = func;
      } else if (wcscmp(tokens->tokens[index].value, L"定义类") == 0 ||
                 wcscmp(tokens->tokens[index].value, L"class") == 0) {
      } else {
      }
    } else {
      // 未知的全局定义
      setError(ERR_GLOBAL_UNKOWN, tokens->tokens[index].line,
               tokens->tokens[index].value);
      if (err)
        *err = 255;
      free(program);
      return NULL;
  }
  }
  return program;
}
// 根据字符串解析数据类型
static IR_DataType parseDataType(wchar_t *typeStr) {
  IR_DataType dt;
  if (wcscmp(typeStr, L"int") == 0) {
    dt.kind = IR_DT_INT;
  } else if (wcscmp(typeStr, L"float") == 0) {
    dt.kind = IR_DT_FLOAT;
  } else if (wcscmp(typeStr, L"string") == 0) {
    dt.kind = IR_DT_STRING;
  } else if (wcscmp(typeStr, L"char") == 0) {
    dt.kind = IR_DT_CHAR;
  } else if (wcscmp(typeStr, L"bool") == 0) {
    dt.kind = IR_DT_BOOL;
  } else if (wcscmp(typeStr, L"void") == 0) {
    dt.kind = IR_DT_VOID;
  } else if (wcscmp(typeStr, L"无参数") == 0) {
    dt.kind = IR_DT_VOID;
  } else if (wcscmp(typeStr, L"整型") == 0) {
    dt.kind = IR_DT_INT;
  } else if (wcscmp(typeStr, L"浮点型") == 0) {
    dt.kind = IR_DT_FLOAT;
  } else if (wcscmp(typeStr, L"字符串") == 0) {
    dt.kind = IR_DT_STRING;
  } else if (wcscmp(typeStr, L"字符型") == 0) {
    dt.kind = IR_DT_CHAR;
  } else if (wcscmp(typeStr, L"布尔型") == 0) {
    dt.kind = IR_DT_BOOL;
  } else {
    dt.kind = IR_DT_CUSTOM;
    dt.custom_type_name =
        (wchar_t *)calloc(wcslen(typeStr) + 1, sizeof(wchar_t));
    if (dt.custom_type_name) {
      wcscpy(dt.custom_type_name, typeStr);
    }
  }
  return dt;
}
// 解析函数参数列表
// 参数::= 标识符：数据类型
static IR_FunctionParam *parseFunctionParams(Tokens *tokens, int *index,
                                             int *param_count, int *err) {
  *param_count = 0;
  IR_FunctionParam *params = NULL;
  if (tokens->tokens[*index].type == TOK_OPR_RQUOTE) {
    return NULL;
  }
  while (*index < tokens->count &&
         tokens->tokens[*index].type != TOK_OPR_RQUOTE) {
    if (wcscmp(tokens->tokens[*index].value, L"无参数") == 0 ||
        wcscmp(tokens->tokens[*index].value, L"void") == 0) {
      (*index)++;
      break;
    }
    if (tokens->tokens[*index].type != TOK_ID) {
      *err = 255; // 语法错误
      setError(ERR_FUN_ARG, tokens->tokens[*index].line, NULL);
      return NULL;
    }
    (*param_count)++;
    params = (IR_FunctionParam *)realloc(params, (*param_count) *
                                                     sizeof(IR_FunctionParam));
    if (!params) {
      *err = -1;
      return NULL;
    }
    params[(*param_count) - 1].name = (wchar_t *)calloc(
        wcslen(tokens->tokens[*index].value) + 1, sizeof(wchar_t));
    if (!params[(*param_count) - 1].name) {
      *err = -1;
      free(params);
      return NULL;
    }
    wcscpy(params[(*param_count) - 1].name, tokens->tokens[*index].value);
    if ((*index + 1) >= tokens->count) {
      *err = 255; // 语法错误
      setError(ERR_FUN_ARG, tokens->tokens[*index].line, NULL);
      // 释放已分配的内存
      for (int i = 0; i < (*param_count); i++) {
        free(params[i].name);
      }
      free(params);
      return NULL;
    }
    (*index)++;
    // 解析冒号
    if (tokens->tokens[*index].type != TOK_OPR_COLON) {
      *err = 255; // 语法错误
      setError(ERR_FUN_ARG, tokens->tokens[*index].line, NULL);
      // 释放已分配的内存
      for (int i = 0; i < (*param_count); i++) {
        free(params[i].name);
      }
      free(params);
      return NULL;
    }
    if ((*index + 1) >= tokens->count) {
      *err = 255; // 语法错误
      setError(ERR_FUN_ARG, tokens->tokens[*index].line, NULL);
      // 释放已分配的内存
      for (int i = 0; i < (*param_count); i++) {
        free(params[i].name);
      }
      free(params);
      return NULL;
    }
    (*index)++;
    // 解析数据类型
    if (tokens->tokens[*index].type != TOK_ID) {
      *err = 255; // 语法错误
      setError(ERR_FUN_ARG, tokens->tokens[*index].line, NULL);
      // 释放已分配的内存
      for (int i = 0; i < (*param_count); i++) {
        free(params[i].name);
      }
      free(params);
      return NULL;
    }
    params[(*param_count) - 1].type =
        parseDataType(tokens->tokens[*index].value);
    // 下一参数或结束
    if ((*index + 1) >= tokens->count) {
      *err = 255; // 语法错误
      setError(ERR_FUN, tokens->tokens[*index].line, NULL);
      // 释放已分配的内存
      for (int i = 0; i < (*param_count); i++) {
        free(params[i].name);
      }
      free(params);
      return NULL;
    }
    (*index)++;
    if (tokens->tokens[*index].type == TOK_OPR_COMMA) {
      (*index)++;
    } else if (tokens->tokens[*index].type == TOK_OPR_RQUOTE) {
      break;
    } else {
      *err = 255; // 语法错误
      setError(ERR_FUN_ARG, tokens->tokens[*index].line, NULL);
      // 释放已分配的内存
      for (int i = 0; i < (*param_count); i++) {
        free(params[i].name);
      }
      free(params);
      return NULL;
    }
  }
  return params;
}
static IR_Function *parseFunction_CN(Tokens *tokens, int *index, int *err) {
  if ((*index + 1) >= tokens->count) {
    *err = 255; // 语法错误
    setError(ERR_FUN, tokens->tokens[*index].line, NULL);
    return NULL;
  }
  IR_Function *function = (IR_Function *)calloc(1, sizeof(IR_Function));
  if (!function) {
    *err = -1;
    return NULL;
  }
  (*index)++;

  // 解析函数名
  //:|：
  if (tokens->tokens[*index].type != TOK_OPR_COLON) {
    setError(ERR_FUN, tokens->tokens[*index].line, NULL);
    *err = 255; // 语法错误
    free(function);
    return NULL;
  }
  if ((*index + 1) >= tokens->count) {
    setError(ERR_FUN, tokens->tokens[*index].line, NULL);
    *err = 255; // 语法错误
    free(function);
    return NULL;
  }
  (*index)++;
  // ID
  if (tokens->tokens[*index].type != TOK_ID) {
    setError(ERR_FUN, tokens->tokens[*index].line, NULL);
    *err = 255; // 语法错误
    free(function);
    return NULL;
  }
  function->name = (wchar_t *)calloc(wcslen(tokens->tokens[*index].value) + 1,
                                     sizeof(wchar_t));
  if (!function->name) {
    *err = -1;
    free(function);
    return NULL;
  }
  wcscpy(function->name, tokens->tokens[*index].value);
#ifdef HX_DEBUG
  log(L"解析到函数名：%ls", function->name);
#endif

  if ((*index + 1) >= tokens->count) {
    setError(ERR_FUN, tokens->tokens[*index].line, NULL);
    *err = 255; // 语法错误
    free(function->name);
    free(function);
    return NULL;
  }
  (*index)++;
  // (
  if (tokens->tokens[*index].type != TOK_OPR_LQUOTE) {
    setError(ERR_FUN, tokens->tokens[*index].line, NULL);
    *err = 255; // 语法错误
    free(function->name);
    free(function);
    return NULL;
  }
  if ((*index + 1) >= tokens->count) {
    setError(ERR_FUN, tokens->tokens[*index].line, NULL);
    *err = 255; // 语法错误
    free(function->name);
    free(function);
    return NULL;
  }
  (*index)++;
  // 解析参数列表
  function->params =
      parseFunctionParams(tokens, index, &(function->param_count), err);
  if (*err != 0) {
    free(function->name);
    free(function);
    return NULL;
  }
  // 分析返回类型或复制函数体
  if ((*index + 1) >= tokens->count) {
    setError(ERR_FUN, tokens->tokens[*index].line, NULL);
    *err = 255; // 语法错误
    // 释放已分配的内存
    for (int i = 0; i < function->param_count; i++) {
      free(function->params[i].name);
    }
    free(function->params);
    free(function->name);
    free(function);
    return NULL;
  }
  (*index)++;
  function->isReturnTypeKnown = false;
  if (tokens->tokens[*index].type == TOK_OPR_COMMA) {
    // ,
    if ((*index + 1) >= tokens->count) {
      setError(ERR_FUN, tokens->tokens[*index].line, NULL);
      *err = 255; // 语法错误
      // 释放已分配的内存
      for (int i = 0; i < function->param_count; i++) {
        free(function->params[i].name);
      }
      free(function->params);
      free(function->name);
      free(function);
      return NULL;
    }
    (*index)++;
    // 它的返回类型是
    if (wcscmp(tokens->tokens[*index].value, L"它的返回类型是") == 0) {
      // 冒号
      if ((*index + 1) >= tokens->count) {
        setError(ERR_FUN, tokens->tokens[*index].line, NULL);
        *err = 255; // 语法错误
        // 释放已分配的内存
        for (int i = 0; i < function->param_count; i++) {
          free(function->params[i].name);
        }
        free(function->params);
        free(function->name);
        free(function);
        return NULL;
      }
      (*index)++;
      if (tokens->tokens[*index].type != TOK_OPR_COLON) {
        setError(ERR_FUN, tokens->tokens[*index].line, NULL);
        *err = 255; // 语法错误
        // 释放已分配的内存
        for (int i = 0; i < function->param_count; i++) {
          free(function->params[i].name);
        }
        free(function->params);
        free(function->name);
        free(function);
        return NULL;
      }
      // 数据类型
      if ((*index + 1) >= tokens->count) {
        setError(ERR_FUN, tokens->tokens[*index].line, NULL);
        *err = 255; // 语法错误
        // 释放已分配的内存
        for (int i = 0; i < function->param_count; i++) {
          free(function->params[i].name);
        }
        free(function->params);
        free(function->name);
        free(function);
        return NULL;
      }
      (*index)++;
      if (tokens->tokens[*index].type != TOK_ID) {
        setError(ERR_FUN, tokens->tokens[*index].line, NULL);
        *err = 255; // 语法错误
        // 释放已分配的内存
        for (int i = 0; i < function->param_count; i++) {
          free(function->params[i].name);
        }
        free(function->params);
        free(function->name);
        free(function);
        return NULL;
      }
#ifdef HX_DEBUG
      log(L"解析到函数返回类型：%ls", tokens->tokens[*index].value);
#endif
      function->return_type = parseDataType(tokens->tokens[*index].value);
      function->isReturnTypeKnown = true;
      if ((*index + 1) >= tokens->count) {
        setError(ERR_FUN, tokens->tokens[*index].line, NULL);
        *err = 255; // 语法错误
        // 释放已分配的内存
        for (int i = 0; i < function->param_count; i++) {
          free(function->params[i].name);
        }
        free(function->params);
        free(function->name);
        free(function);
        return NULL;
      }
      (*index)++;
      //它没有返回类型
    } else if(wcscmp(tokens->tokens[*index].value, L"它没有返回类型") == 0) {
      function->isReturnTypeKnown = true;
      function->return_type.kind = IR_DT_VOID;
      if ((*index + 1) >= tokens->count) {
        setError(ERR_FUN, tokens->tokens[*index].line, NULL);
        *err = 255; // 语法错误
        // 释放已分配的内存
        for (int i = 0; i < function->param_count; i++) {
          free(function->params[i].name);
        }
        free(function->params);
        free(function->name);
        free(function);
        return NULL;
      }
      (*index)++;
    } else {
      setError(ERR_FUN, tokens->tokens[*index].line, NULL);
      *err = 255; // 语法错误
      // 释放已分配的内存
      for (int i = 0; i < function->param_count; i++) {
        free(function->params[i].name);
      }
      free(function->params);
      free(function->name);
      free(function);
      return NULL;
    }
  }
  // 函数体
  if (tokens->tokens[*index].type != TOK_OPR_LBRACE) {
    setError(ERR_FUN, tokens->tokens[*index].line, NULL);
    *err = 255; // 语法错误
    // 释放已分配的内存
    for (int i = 0; i < function->param_count; i++) {
      free(function->params[i].name);
    }
    free(function->params);
    free(function->name);
    free(function);
    return NULL;
  }
  // 复制函数体Token流
  int body_start_index = *index;
  int brace_count = 1;
  while (*index + 1 < tokens->count && brace_count > 0) {
    (*index)++;
    if (tokens->tokens[*index].type == TOK_OPR_LBRACE) {
      brace_count++;
    } else if (tokens->tokens[*index].type == TOK_OPR_RBRACE) {
      brace_count--;
    }

    if (brace_count == 0) {
      break;
    }
  }
  int end = *index;
  if (brace_count != 0) {
    setError(ERR_HUAKUOHAO_NOT_CLOSE, tokens->tokens[*index].line, tokens->tokens[body_start_index].value);
    *err = 255; // 语法错误
    // 释放已分配的内存
    for (int i = 0; i < function->param_count; i++) {
      free(function->params[i].name);
    }
    free(function->params);
    free(function->name);
    free(function);
    return NULL;
  }
  function->body_token_count = end - body_start_index + 1;
  function->body_tokens =
      (Token *)calloc(function->body_token_count, sizeof(Token));
  if (!function->body_tokens) {
    *err = -1;
    // 释放已分配的内存
    for (int i = 0; i < function->param_count; i++) {
      free(function->params[i].name);
    }
    free(function->params);
    free(function->name);
    free(function);
    return NULL;
  }
  for (int i = 0; i < function->body_token_count; i++) {
    function->body_tokens[i] = tokens->tokens[body_start_index + i];
  }
  (*index)++;
#ifdef HX_DEBUG
  showFunctionInfo(function);
#endif
  return function;
}
static IR_Function *parseFunction_EN(Tokens *tokens, int *index, int *err) {}
// 定义函数::= fun:标识符(参数列表)->数据类型 { 函数体 }
// 定义函数::= 定义函数:标识符(参数列表)
// [，它的返回类型是：数据类型]|[，它没有返回类型] { 函数体 }
IR_Function *parseFunction(Tokens *tokens, int *index, int *err) {
  if (!tokens || !index) {
    if (err)
      *err = -1;
    return NULL;
  }
  IR_Function *function = NULL;
  if (wcscmp(tokens->tokens[*index].value, L"fun") == 0) {

  } else if (wcscmp(tokens->tokens[*index].value, L"定义函数") == 0) {
    function = parseFunction_CN(tokens, index, err);
    if (*err != 0) {
      return NULL;
    }
  } else {
    *err = 255; // 语法错误
    return NULL;
  }

  return function;
}

#ifdef HX_DEBUG
void showFunctionInfo(IR_Function *function) {
  if (!function)
    return;
  initLocale();
  fwprintf(logStream, L"函数名: %ls\n", function->name);
  fwprintf(logStream, L"参数个数: %d\n", function->param_count);
  for (int i = 0; i < function->param_count; i++) {
    fwprintf(logStream, L"\t参数%d: 名字=%ls, 类型=", i + 1,
             function->params[i].name);
    switch (function->params[i].type.kind) {
    case IR_DT_INT:
      fwprintf(logStream, L"int\n");
      break;
    case IR_DT_FLOAT:
      fwprintf(logStream, L"float\n");
      break;
    case IR_DT_STRING:
      fwprintf(logStream, L"string\n");
      break;
    case IR_DT_CHAR:
      fwprintf(logStream, L"char\n");
      break;
    case IR_DT_BOOL:
      fwprintf(logStream, L"bool\n");
      break;
    case IR_DT_VOID:
      fwprintf(logStream, L"void\n");
      break;
    case IR_DT_CUSTOM:
      fwprintf(logStream, L"custom(%ls)\n",
               function->params[i].type.custom_type_name);
      break;
    }
  }
}
#endif

#endif