#ifndef HXLANG_SRC_HXC_GENERATOR_H
#define HXLANG_SRC_HXC_GENERATOR_H
#include "Error.h"
#include "IR.h"
#include "Lexer.h"
#include "ObjectCode.h"
#include "Parser.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

typedef uint16_t wchar;
/*
 * 生成目标代码
 * @param program: 中间表示
 * @param err: 错误码指针，发生错误时会设置为相应错误码
 * @return 生成的目标代码对象指针，发生错误时返回NULL
 */
extern ObjectCode *generateObjectCode(IR_Program *program, int *err);
/*
 * 生成函数的目标代码
 * @param function: 中间表示的函数
 * @param err: 错误码指针，发生错误时会设置为相应错误码
 * @return 生成的过程对象指针，发生错误时返回NULL
 */
static Procedure *generateFunction(IR_Function *function,
                                   ConstantPool *constantPool, int *err);
extern void freeObjectCode(ObjectCode **objCode);
static int getMainFunctionIndex(IR_Program *program) {
  if (!program || !program->functions)
    return -1;
  int index = -1;
  for (int i = 0; i < program->function_count; i++) {
    if ((program->functions[i] && program->functions[i]->name &&
         wcscmp(program->functions[i]->name, L"main") == 0) ||
        (program->functions[i] && program->functions[i]->name &&
         wcscmp(program->functions[i]->name, L"主函数") == 0)) {
      if (index != -1) {
        // 已经找到过main函数，说明有重定义
        index = -255;
        setError(ERR_MAIN, program->functions[i]->body_tokens[0].line,
                 program->functions[i]->name);
        break;
      }
      index = i;
    }
  }
  return index;
}
#ifdef HX_DEBUG
static void listObjectCode_Proc(Procedure *proc) {
  if (!proc)
    return;
  fwprintf(logStream, L"[DEB] 过程指令数: %u\n", proc->instructionSize);
  for (uint32_t i = 0; i < proc->instructionSize; i++) {
    Instruction ins = proc->instructions[i];
    switch (ins.opcode) {
    case OP_LOAD_CONST: {
      fwprintf(logStream, L"\t%03u: OP_LOAD_CONST", i);
      // print param type and value for common types
      if (ins.params[0].type == PARAM_TYPE_INT) {
        int32_t v;
        memcpy(&v, ins.params[0].value, sizeof(int32_t));
        fwprintf(logStream, L" INT=%d", v);
      } else if (ins.params[0].type == PARAM_TYPE_FLOAT) {
        float f;
        memcpy(&f, ins.params[0].value, sizeof(float));
        fwprintf(logStream, L" FLOAT=%f", f);
      } else if (ins.params[0].type == PARAM_TYPE_CHAR) {
        wchar_t c;
        memcpy(&c, ins.params[0].value, sizeof(wchar_t));
        fwprintf(logStream, L" CHAR=%lc", c);
      } else if (ins.params[0].type == PARAM_TYPE_INDEX) {
        uint32_t idx = 0;
        memcpy(&idx, ins.params[0].value, sizeof(uint32_t));
        fwprintf(logStream, L" INDEX=%u", idx);
      }
      fwprintf(logStream, L"\n");
      break;
    }
    case OP_LOAD_VAR:
      fwprintf(logStream, L"\t%03u: OP_LOAD_VAR index=%d\n", i,
               (int)ins.params[0].value[0]);
      break;
    case OP_STORE_VAR:
      fwprintf(logStream, L"\t%03u: OP_STORE_VAR\n", i);
      break;
    case OP_ADD:
      fwprintf(logStream, L"\t%03u: OP_ADD\n", i);
      break;
    case OP_SUB:
      fwprintf(logStream, L"\t%03u: OP_SUB\n", i);
      break;
    case OP_MUL:
      fwprintf(logStream, L"\t%03u: OP_MUL\n", i);
      break;
    case OP_DIV:
      fwprintf(logStream, L"\t%03u: OP_DIV\n", i);
      break;
    case OP_CAL:
      fwprintf(logStream, L"\t%03u: OP_CAL\n", i);
      break;
    case OP_RET:
      fwprintf(logStream, L"\t%03u: OP_RET\n", i);
      break;
    default:
      fwprintf(logStream, L"\t%03u: OP_NOP\n", i);
    }
  }
}
#endif
//----------------------------------------------------------------------------
ObjectCode *generateObjectCode(IR_Program *program, int *err) {
  if (!program || !err) {
    if (err)
      *err = -1;
    return NULL;
  }
  // 找入口函数
  int mainIndex = getMainFunctionIndex(program);
  if (mainIndex == -255) {
    *err = 255;
    return NULL;
  } else if (mainIndex == -1) {
    setError(ERR_NO_MAIN, 0, NULL);
    *err = 255;
    return NULL;
  }
  ObjectCode *objCode = (ObjectCode *)calloc(1, sizeof(ObjectCode));
  if (!objCode) {
    if (err)
      *err = -1;
    return NULL;
  }
  // 初始化目标代码对象
  objCode->procedures = (Procedure **)calloc(1, sizeof(Procedure *));
  if (!objCode->procedures) {
    free(objCode);
    if (err)
      *err = -1;
    return NULL;
  }
  objCode->procedureSize = 1;

// 生成main函数的目标代码
#ifdef HX_DEBUG
  log(L"生成main函数的目标代码...");
#endif
  objCode->procedures[0] = generateFunction(program->functions[mainIndex],
                                            &(objCode->constantPool), err);
  if (*err != 0) {
    // freeObjectCode(&objCode);
    return NULL;
  }

#ifdef HX_DEBUG
  // 打印生成的目标代码（调试）
  if (objCode->procedures && objCode->procedures[0]) {
    fwprintf(logStream, L"[DEB] 列出生成的目标代码:\n");
    listObjectCode_Proc(objCode->procedures[0]);
  }
#endif

  return objCode;
}
static int getClassIndexByName(wchar_t *name, IR_Class **class_table,
                               int class_table_size) {
  if (!name || !class_table)
    return -1;
  for (int i = 0; i < class_table_size; i++) {
    if (!class_table[i])
      continue;
    if (wcscmp(name, class_table[i]->name) == 0) {
      return i;
    }
  }
  return -1;
}
static int getVarSize(IR_DataType type, IR_Class **class_table,
                      int class_table_size) {
  if (!class_table)
    return 0;
  switch (type.kind) {
  case IR_DT_INT:
    return sizeof(int32_t);
  case IR_DT_FLOAT:
    return sizeof(float);
  case IR_DT_CHAR:
    return sizeof(wchar_t);
  case IR_DT_BOOL:
    return sizeof(bool);
  case IR_DT_CUSTOM:
    return (class_table)[getClassIndexByName(type.custom_type_name, class_table,
                                             class_table_size)]
        ->size;
  default:
    return 0;
  }
}
Procedure *generateFunction(IR_Function *function, ConstantPool *constantPool,
                            int *err) {
  if (!function || !err) {
    if (err)
      *err = -1;
    return NULL;
  }
  initLocale();
  Procedure *proc = (Procedure *)calloc(1, sizeof(Procedure));
  if (!proc) {
    *err = -1;
    return NULL;
  }
  if (function->body_token_count == 0 || !function->body_tokens) {
    // 空函数体
    proc->instructions = NULL;
    proc->instructionSize = 0;
    proc->localVarSize = 0;
    proc->stackSize = 0;
    return proc;
  }
  int index = 0;
  SymbolTable localeSymbolTable = {0};
  proc->instructions = NULL;
  proc->instructionSize = 0;
  while (index < function->body_token_count) {
    Token currentToken = function->body_tokens[index];
    // 返回::= ret:exp
    // 返回::= 返回:exp
    // PUSH ...
    // OP_RET
    //wprintf(L"%ls\n", currentToken.value);
    if (wcscmp(currentToken.value, L"ret") == 0 ||
        wcscmp(currentToken.value, L"返回") == 0) {
#ifdef HX_DEBUG
      log(L"解析到返回语句\n");
#endif
      /* 语法：ret : <expression> ;
       */
      if (index + 1 >= function->body_token_count) {
        setError(ERR_RET, currentToken.line, NULL);
        *err = 255;
        free(proc);
        return NULL;
      }
      index++; // 指向冒号
      if (function->body_tokens[index].type == TOK_END) {
        if (function->return_type.kind != IR_DT_VOID &&
            function->isReturnTypeKnown) {
          setError(ERR_RET_VAL, currentToken.line, NULL);
          *err = 255;
          free(proc);
          return NULL;
        }
        // 空返回
        proc->instructions = (Instruction *)realloc(
            proc->instructions,
            sizeof(Instruction) * (proc->instructionSize + 1));
#ifdef HX_DEBUG
        log(L"生成空返回指令\n");
#endif
        if (!proc->instructions) {
          *err = -1;
          free(proc);
          return NULL;
        }
        proc->instructions[proc->instructionSize].opcode = OP_RET;
        proc->instructionSize++;
        continue;
      }
      //:
      if (function->body_tokens[index].type != TOK_OPR_COLON) {
        setError(ERR_RET, currentToken.line, NULL);
        *err = 255;
        free(proc);
        return NULL;
      }
      index++; // 指向表达式起始位置
      wprintf(L"%ls", function->body_tokens[index].value);
      ASTNode *expNode =
          parseExpression(function->body_tokens, &index,
                          function->body_token_count, &localeSymbolTable, err);
      if (*err != 0 || !expNode) {
        free(proc);
        return NULL;
      }
      index++;
      if(function->body_tokens[index].type != TOK_END) {
        setError(ERR_RET, currentToken.line, NULL);
        *err = 255;
        freeAST(expNode);
        free(proc);
        return NULL;
      }
    }
    index++;
  }
  return proc;
}
#endif