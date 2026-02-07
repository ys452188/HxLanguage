#ifndef HXLANG_SRC_HXC_OBJECTCODE_H
#define HXLANG_SRC_HXC_OBJECTCODE_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>
typedef enum {
  OP_NOP,
  OP_LOAD_CONST, // 加载常量至栈顶 OP_LOAD_CONST <param_type> <param_value> | OP_LOAD_CONST <const_index>
  OP_LOAD_VAR,   // 加载变量至栈顶
  OP_STORE_VAR,  // 将栈顶值存入变量
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_JMP,
  OP_JMP_CONDITION,
  OP_CAL,        //CAL <proc_index>(u32) <param_count>(u32) 
  OP_RET,
} Opcode;
typedef struct Param {
  enum ParamType {
    PARAM_TYPE_INT,
    PARAM_TYPE_FLOAT,    //double
    PARAM_TYPE_CHAR,
    PARAM_TYPE_BOOL,
    PARAM_TYPE_STRING, 
    PARAM_TYPE_ADDRESS,
    PARAM_TYPE_INDEX   //uint32_t 索引常量池或过程表
  } type;
  uint8_t size;
  char value[8];
} Param;
// 指令
typedef struct Instruction {
  Opcode opcode;
  Param params[3];
} Instruction;
// 过程,用索引访问
typedef struct Procedure {
  Instruction *instructions;
  uint32_t stackSize;    // 栈大小
  uint32_t localVarSize; // 局部变量数量
  uint32_t instructionSize;
} Procedure;
//------------------------------------
// 常量池
typedef struct Constant {
  enum ConstantType {
    CONST_STRING,
  } type;
  union {
    uint16_t *string_value;
  } value;
  uint16_t size;  // 不是字符串长度
} Constant;
typedef struct ConstantPool {
  uint32_t size;
  Constant *constants;
} ConstantPool;
//----------------------------------
typedef struct ObjectCodeHeader {
  char magic[4]; // 魔数 "HXOC"
} ObjectCodeHeader;
//--------------------------------------
typedef struct ObjectCode {
  ObjectCodeHeader header;
  ConstantPool constantPool;
  Procedure **procedures;
  uint32_t procedureSize;
} ObjectCode;
//--------------------------------------
#endif