#ifndef HXHLANG_SRC_HXC_OBJECTCODE_H
#define HXHLANG_SRC_HXC_OBJECTCODE_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>

#include <cstdio>
#include <string>

#pragma pack(push, 1)  // 强制 1 字节对齐
typedef uint8_t Opcode;
enum {
    OP_NOP = 0,
    OP_LOAD_CONST,  // 加载常量至栈顶 OP_LOAD_CONST <paramType> <paramValue> |
    // OP_LOAD_CONST <constantIndex>
    OP_LOAD_VAR,   // 加载变量至栈顶
    OP_POP,        // 弹出
    OP_STORE_VAR,  // 将栈顶值存入变量
    OP_DEF_VAR,    // 为变量开辟内存空间 OP_DEF_VAR <memorySize(u32)>
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_JMP,
    OP_JMP_CONDITION,  // JMP_CONDITION <栈顶为真时跳转的地址>
    // <为假时跳转的地址(>size时跳转至末尾)>
    OP_CAL,            // CAL <procIndex>(u32) <paramCount>(u32)
    OP_RET,
    OP_PRINT_STRING,
    // 类型转换
    OP_CHAR_TO_INT,
    OP_INT_TO_CHAR,
    OP_INT_TO_FLOAT,
    OP_CHAR_TO_FLOAT,
    OP_CHAR_TO_STRING,
    OP_FLOAT_TO_INT,
    OP_INT_TO_STRING,
    // 连接字符串
    OP_STRING_CONCAT,
};
typedef uint8_t ParamType;
enum {
    PARAM_TYPE_INT = 0,
    PARAM_TYPE_FLOAT,  // double
    PARAM_TYPE_CHAR,
    PARAM_TYPE_BOOL,
    PARAM_TYPE_STRING,
    PARAM_TYPE_ADDRESS,
    PARAM_TYPE_INDEX,  // uint32_t 索引常量池或过程表
    PARAM_TYPE_SIZE    // u32
};
typedef struct Param {
    ParamType type;  // char
    uint8_t size;
    char value[8];
    uint32_t offest;  // 偏移量
} Param;
// 指令
typedef struct Instruction {
    bool isNotUsed;  // 为true的指令将不会写入
    Opcode opcode;   // char
    Param params[3];
    FunCallPitch* pitch;  // 回填，仅OP_CAL使用,不写入文件
} Instruction;
// 过程,用索引访问
typedef struct Procedure {
    bool isUsed = false;      // 这个变量不会写入
    IR_Function* fun = NULL;  // 这个变量也不会写入

    uint32_t instructionSize = 0;
    std::vector<Instruction> instructions;
    uint32_t stackSize = 0;     // 栈大小
    uint32_t localVarSize = 0;  // 局部变量数量
} Procedure;
//------------------------------------
// 常量池
enum ConstantType {
    CONST_STRING,
};
typedef struct Constant {
    ConstantType type;  // char, 1字节
    uint32_t size;      // 真实大小，不是字符串长度
    union {
        wchar_t* string_value;
    } value;
} Constant;
typedef struct ConstantPool {
    uint32_t size = 0;
    Constant* constants = NULL;
} ConstantPool;
//----------------------------------
typedef struct ObjectCodeHeader {
    char magic[4];  // 魔数 "HXOC"
    float version = 0.0f;
    uint8_t isInDebugMode = 0;
} ObjectCodeHeader;
//--------------------------------------
typedef struct ObjectCode {
    ObjectCodeHeader header;
    ConstantPool constantPool;
    uint32_t procedureSize = 0;
    std::vector<Procedure*> procedures;
    int32_t start = 0;  // 入口索引
} ObjectCode;
//--------------------------------------
// 写入目标代码
extern int writeObjectCode(FILE* objFile, ObjectCode& obj) noexcept;

static int writeHeader(FILE* file) noexcept {
#ifdef HX_DEBUG
    log(L"写入文件头");
#endif
    ObjectCodeHeader header = {};
    header.magic[0] = 'H';
    header.magic[1] = 'X';
    header.magic[2] = 'O';
    header.magic[3] = 'C';
    header.version = HXC_VERSION;
    header.isInDebugMode = (uint8_t)isInDebugMode;
    if (fwrite(&(header.magic), sizeof(header.magic), 1, file) != 1) return -1;
    if (fwrite(&(header.version), sizeof(header.version), 1, file) != 1)
        return -1;
    if (fwrite(&(header.isInDebugMode), sizeof(header.isInDebugMode), 1, file) !=
            1)
        return -1;
    return 0;
}
// 存的是真实大小
/************************
    |------------------|
    |     size(u32     |
    |------------------|
    |  value[0](u16)   |
    |------------------|
    |  value[1](u16)   |
    ..................
*************************/
static int writeWstring(const wchar_t* wstr, FILE* file) noexcept {
    if (!wstr) {
        uint32_t byteLen = 0;
        return fwrite(&byteLen, sizeof(byteLen), 1, file) == 1 ? 0 : -1;
    }

    uint32_t len = wcslen(wstr);
    uint32_t byteLen = (len + 1) * sizeof(uint32_t);  // 含 \0
    if (fwrite(&byteLen, sizeof(byteLen), 1, file) != 1) return -1;

    for (uint32_t i = 0; i <= len; i++) {
        uint32_t cp = (uint32_t)wstr[i];
        if (fwrite(&cp, sizeof(cp), 1, file) != 1) return -1;
    }
    return 0;
}
static int writeParam(Param& param, FILE* file) noexcept {
    // 写type
    char type = (char)(param.type);
    if (fwrite(&(type), sizeof(char), 1, file) != 1) return -1;
    // size
    if (fwrite(&(param.size), sizeof(uint8_t), 1, file) != 1) return -1;
    // value
#ifdef HX_DEBUG
    // log(L"%d", *((int32_t*)(param.value)));
#endif
    if (fwrite(&(param.value), sizeof(param.value), 1, file) != 1) return -1;
    // 偏移量
    if (fwrite(&(param.offest), sizeof(uint32_t), 1, file) != 1) return -1;
    return 0;
}
static int writeInstruction(Instruction& inst, FILE* file) {
    if (inst.isNotUsed) {
#ifdef HX_DEBUG
        log(L"指令无用,跳过");
#endif
        return 0;
    }
#ifdef HX_DEBUG
    fwprintf(logStream, L"写入指令");
    switch (inst.opcode) {
    case OP_LOAD_CONST: {
        fwprintf(logStream, L"\33[1;34mOP_LOAD_CONST\33[0m)\n");
        break;
    }
    case OP_PRINT_STRING:
        fwprintf(logStream, L"\33[1;34mOP_PRINT_STRING\33[0m\n");
        break;
    case OP_DEF_VAR:
        fwprintf(logStream, L"\33[1;34mOP_DEF_VAR\33[0m)\n");
        break;
    case OP_LOAD_VAR:
        fwprintf(logStream, L"\33[1;34mOP_LOAD_VAR\33[0m)\n");
        break;
    case OP_STORE_VAR:
        fwprintf(logStream, L"\33[1;34mOP_STORE_VAR\33[0m)\n");
        break;
    case OP_ADD:
        fwprintf(logStream, L"\33[1;34mOP_ADD\33[0m)\n");
        break;
    case OP_SUB:
        fwprintf(logStream, L"\33[1;34mOP_SUB\33[0m)\n");
        break;
    case OP_MUL:
        fwprintf(logStream, L"\33[1;34mOP_MUL\33[0m)\n");
        break;
    case OP_DIV:
        fwprintf(logStream, L"\33[1;34mOP_DIV\33[0m)\n");
        break;
    case OP_CAL:
        fwprintf(logStream, L"\33[1;34mOP_CAL\33[0m)\n");
        break;
    case OP_RET:
        fwprintf(logStream, L"\33[1;34mOP_RET\33[0m)\n");
        break;
    case OP_CHAR_TO_INT:
        fwprintf(logStream, L"\33[1;34m OP_CHAR_TO_INT\33[0m\n");
        break;
    case OP_INT_TO_CHAR:
        fwprintf(logStream, L"\33[1;34m OP_INT_TO_CHAR\33[0m\n");
        break;
    case OP_INT_TO_FLOAT:
        fwprintf(logStream, L"\33[1;34m OP_INT_TO_CHAR\33[0m\n");
        break;
    case OP_CHAR_TO_FLOAT:
        fwprintf(logStream, L"\33[1;34m OP_CHAR_TO_FLOAT\33[0m\n");
        break;
    case OP_CHAR_TO_STRING:
        (logStream, L"\33[1;34m OP_CHAR_TO_STRING\33[0m\n");
        break;
    case OP_FLOAT_TO_INT:
        (logStream, L"\33[1;34m OP_FLOAT_TO_INT\33[0m\n");
        break;
    case OP_INT_TO_STRING:
        fwprintf(logStream, L"\33[1;34m OP_INT_TO_STRING\33[0m\n");
        break;
    case OP_POP:
        fwprintf(logStream, L"\33[1;34m OP_POP\33[0m\n");
        break;
    case OP_JMP:
        fwprintf(logStream, L"\33[1;34m OP_JMP\33[0m\n");
        break;
    default:
        fwprintf(logStream, L"\33[1;31mOP_NOP\33[0m)\n");
    }
#endif
    // 写opcode
    if (fwrite(&(inst.opcode), sizeof(Opcode), 1, file) != 1) return -1;
    // param
    for (int i = 0; i < 3; i++) {
        if (writeParam((inst.params[i]), file)) return -1;
    }
    return 0;
}
static int writeProcedure(Procedure& proc, FILE* file) noexcept {
    for(int i = 0; i < proc.instructionSize; i++) {
        if(proc.instructions.at(i).isNotUsed) {
            proc.instructionSize--;
        }    
    }
#ifdef HX_DEBUG
    log(L"算得指令数为%d", proc.instructionSize);
#endif
    // 写instructionSize
    #ifdef HX_DEBUG
    log(L"写instructionSize:%d",proc.instructionSize);
#endif
    if (fwrite(&(proc.instructionSize), sizeof(uint32_t), 1, file) != 1)
        return -1;
    // 写instructions
    #ifdef HX_DEBUG
    log(L"写instructions");
#endif
    for (int i = 0; i < proc.instructions.size(); i++) {
        if (writeInstruction(proc.instructions.at(i), file)) return -1;
    }
    // stackSize
    #ifdef HX_DEBUG
    log(L"写stackSize:%d",proc.stackSize);
#endif
    if (fwrite(&(proc.stackSize), sizeof(uint32_t), 1, file) != 1) return -1;
    // localVarSize
    #ifdef HX_DEBUG
    log(L"写localVarSize:%d",proc.localVarSize);
#endif
    if (fwrite(&(proc.localVarSize), sizeof(uint32_t), 1, file) != 1) return -1;
    return 0;
}
int writeObjectCode(FILE* objFile, ObjectCode& obj) noexcept {
    if (!objFile) return -1;
    if (writeHeader(objFile)) return -1;
    // 写ConstantPoolSize
    if (fwrite(&(obj.constantPool.size), sizeof(uint32_t), 1, objFile) != 1)
        return -1;
    // 写ConstantPool.constants
    for (int i = 0; i < obj.constantPool.size; i++) {
        // 写tyoe
        char type = (char)(obj.constantPool.constants[i].type);
        if (fwrite(&(type), sizeof(char), 1, objFile) != 1) return -1;
        if (obj.constantPool.constants[i].type == CONST_STRING) {
            if (writeWstring(obj.constantPool.constants[i].value.string_value,
                             objFile))
                return -1;
        }
    }
    // ProcedureSize
    if (fwrite(&(obj.procedureSize), sizeof(uint32_t), 1, objFile) != 1)
        return -1;
    // procedures
    for (int i = 0; i < obj.procedureSize; i++) {
#ifdef HX_DEBUG
        log(L"写入过程%d", i);
#endif
        if (writeProcedure(*(obj.procedures.at(i)), objFile)) return -1;
    }
    // 入口索引
    if (fwrite(&(obj.start), sizeof(uint32_t), 1, objFile) != 1) return -1;
    fclose(objFile);
    return 0;
}
#endif