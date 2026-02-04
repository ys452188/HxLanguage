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
static Instruction *generateInstructionsFromAST(Instruction *instructions,
                                                int *inst_index, int *inst_size,
                                                ASTNode *node,
                                                ConstantPool *constantPool,
                                                int *err);
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
        double d;
        memcpy(&d, ins.params[0].value, sizeof(double));
        fwprintf(logStream, L" FLOAT=%f", d);
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
    // wprintf(L"%ls\n", currentToken.value);
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
      //wprintf(L"%ls", function->body_tokens[index].value);
      ASTNode *expNode =
          parseExpression(function->body_tokens, &index,
                          function->body_token_count, &localeSymbolTable, err);
      if (*err != 0 || !expNode) {
        free(proc);
        return NULL;
      }
      #ifdef HX_DEBUG
      log(L"生成返回值表达式指令...\n");
      #endif
      // 生成表达式指令
      int inst_index = proc->instructionSize;
      int inst_size = proc->instructionSize; 
      proc->instructions = generateInstructionsFromAST(
          proc->instructions, &inst_index, &inst_size, expNode, constantPool,
          err);
      if (!proc->instructions || *err != 0) {
        freeAST(expNode);
        *err = -1;
        return NULL;
      }
      proc->instructionSize = inst_index;
      freeAST(expNode);
      // 生成返回指令
    proc->instructions = (Instruction *)realloc(
      proc->instructions,
      sizeof(Instruction) * (proc->instructionSize + 1));
      if (!proc->instructions) {
        *err = -1;
        freeAST(expNode);
        free(proc);
        return NULL;
      }
      proc->instructions[proc->instructionSize].opcode = OP_RET;
      proc->instructionSize++;
      index++;
      if (function->body_tokens[index].type != TOK_END) {
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



Instruction *generateInstructionsFromAST(Instruction *instructions,
                                         int *inst_index, int *inst_size,
                                         ASTNode *node,
                                         ConstantPool *constantPool, int *err) {
  if (!inst_index || !node || !inst_size || !err || !constantPool) {
    if (err)
      *err = -1;
    return NULL;
  }
  if (*inst_size <= *inst_index) {
    instructions = (Instruction *)realloc(instructions, sizeof(Instruction) *
                                                            (*inst_size + 1));
    if (!instructions) {
      *err = -1;
      return NULL;
    }
    *inst_size += 1;
    memset(&instructions[*inst_index], 0, sizeof(Instruction));
  }
  if (node->kind == NODE_VALUE) {
    instructions[*inst_index].opcode = OP_LOAD_CONST;
    // 设置参数
    if (node->data.value.type.kind == IR_DT_INT) {
      instructions[*inst_index].params[0].type = PARAM_TYPE_INT;
      memcpy(instructions[*inst_index].params[0].value,
        &(node->data.value.val.i), sizeof(int32_t));
      instructions[*inst_index].params[0].size = sizeof(int32_t);
      instructions[*inst_index].params[1].type = PARAM_TYPE_INT;


    } else if (node->data.value.type.kind == IR_DT_FLOAT) {
      instructions[*inst_index].params[0].type = PARAM_TYPE_FLOAT;
      memcpy(instructions[*inst_index].params[0].value,
        &(node->data.value.val.f), sizeof(double));
      instructions[*inst_index].params[0].size = sizeof(double);
      instructions[*inst_index].params[1].type = PARAM_TYPE_FLOAT;

    } else if (node->data.value.type.kind == IR_DT_CHAR) {
      instructions[*inst_index].params[0].type = PARAM_TYPE_CHAR;
      memcpy(instructions[*inst_index].params[0].value,
             &(node->data.value.val.c), sizeof(uint16_t));
      instructions[*inst_index].params[0].size = sizeof(uint16_t);

    } else if (node->data.value.type.kind == IR_DT_STRING) {
      // 加入常量池
      constantPool->constants = (Constant *)realloc(
          constantPool->constants, sizeof(Constant) * (constantPool->size + 1));
      if (!constantPool->constants) {
        *err = -1;
        free(instructions);
        return NULL;
      }
      constantPool->constants[constantPool->size].type = CONST_STRING;
      constantPool->constants[constantPool->size].value.string_value =
          (uint16_t *)calloc(wcslen(node->data.value.val.s) + 1,
                             sizeof(uint16_t));
      if (!constantPool->constants[constantPool->size].value.string_value) {
        *err = -1;
        free(instructions);
        return NULL;
      }
      for (int i = 0; i < wcslen(node->data.value.val.s); i++) {
        constantPool->constants[constantPool->size].value.string_value[i] =
            (uint16_t)node->data.value.val.s[i];
      }
      constantPool->constants[constantPool->size].size =
          (uint16_t)wcslen(node->data.value.val.s) * sizeof(uint16_t);
      constantPool->size += 1;
      instructions[*inst_index].params[0].type = PARAM_TYPE_INDEX;
      uint32_t strIndex = constantPool->size - 1;
      memcpy(instructions[*inst_index].params[0].value, &(strIndex),
             sizeof(uint32_t));
      instructions[*inst_index].params[0].size = sizeof(uint32_t);
    } else {
      
      *err = -1;
      free(instructions);
      return NULL;
    }
    (*inst_index)++;
  } else if(node->kind == NODE_BINARY) {
    // 生成左子树指令
    instructions = generateInstructionsFromAST(instructions, inst_index,
                                               inst_size, node->left,
                                               constantPool, err);
    if (*err != 0) {
      free(instructions);
      return NULL;
    }
    // 生成右子树指令
    instructions = generateInstructionsFromAST(instructions, inst_index,
                                               inst_size, node->right,
                                               constantPool, err);
    if (*err != 0) {
      free(instructions);
      return NULL;
    }
    // 生成二元运算指令
    if (*inst_size <= *inst_index) {
      instructions = (Instruction *)realloc(instructions, sizeof(Instruction) *
                                                              (*inst_size + 1));
      if (!instructions) {
        *err = -1;
        return NULL;
      }
      *inst_size += 1;
      memset(&instructions[*inst_index], 0, sizeof(Instruction));
    }
    switch (node->data.binary.op) {
    case 0: // ADD
      instructions[*inst_index].opcode = OP_ADD;
      break;
    case 1: // SUB
      instructions[*inst_index].opcode = OP_SUB;
      break;
    case 2: // MUL
      instructions[*inst_index].opcode = OP_MUL;
      break;
    case 3: // DIV
      instructions[*inst_index].opcode = OP_DIV;
      break;
    default:
      *err = -1;
      free(instructions);
      return NULL;
    }
    (*inst_index)++;
  } else if (node->kind == NODE_VAR) {
    instructions[*inst_index].opcode = OP_LOAD_VAR;
    instructions[*inst_index].params[0].type = PARAM_TYPE_INDEX;
    memcpy(instructions[*inst_index].params[0].value,
           &(node->data.var.index), sizeof(uint32_t));
    instructions[*inst_index].params[0].size = sizeof(uint32_t);
    switch (node->data.var.type.kind) {
    case IR_DT_INT: 
      instructions[*inst_index].params[1].type = PARAM_TYPE_INT;
      break;
    case IR_DT_FLOAT:
      instructions[*inst_index].params[1].type = PARAM_TYPE_FLOAT;
      break;
    case IR_DT_CHAR:
      instructions[*inst_index].params[1].type = PARAM_TYPE_CHAR;
      break;
    case IR_DT_STRING:
      instructions[*inst_index].params[1].type = PARAM_TYPE_STRING;
      break;
    }
    (*inst_index)++;
  } else {
    *err = -1;
    free(instructions);
    return NULL;
  }
  return instructions;
}
#endif