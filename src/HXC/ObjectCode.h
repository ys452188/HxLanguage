#ifndef HXLANG_SRC_HXC_OBJECTCODE_H
#define HXLANG_SRC_HXC_OBJECTCODE_H
#include <wchar.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
typedef enum {
  OP_NOP,
  OP_LOAD_CONST,  //加载常量至栈顶
  OP_LOAD_VAR,    //加载变量至栈顶
  OP_STORE_VAR,   //将栈顶值存入变量
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_JMP,
  OP_JMP_CONDITION,
  OP_CAL,
  OP_RET,
} Opcode;
typedef struct Param {
    enum ParamType {
        PARAM_TYPE_INT,
        PARAM_TYPE_FLOAT,
        PARAM_TYPE_CHAR,
        PARAM_TYPE_BOOL,
        PARAM_TYPE_ADDRESS,
        PARAM_TYPE_INDEX
    } type;
    uint8_t size;
    char value[8];
} Param;
//指令
typedef struct instruction {
  Opcode opcode;
  Param params[3];
} instruction;
//过程,用索引访问
typedef struct Procedure {
  instruction *instructions;
  uint32_t stackSize;    //栈大小
  uint32_t localVarSize; //局部变量数量
  uint32_t instructionSize;
} Procedure;
//------------------------------------
//常量池
typedef struct Constant {
  enum ConstantType {
    CONST_INT,
    CONST_FLOAT,
    CONST_CHAR,
    CONST_BOOL,
    CONST_STRING,
  } type;
  union {
    int int_value;
    float float_value;
    wchar_t char_value;
    bool bool_value;
    wchar_t *string_value;
  } value;
  uint16_t size;
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
  Procedure** procedures;
  uint32_t procedureSize;
} ObjectCode;
//--------------------------------------
#endif