#ifndef HXLANG_SRC_HXVM_OBJECT_READER_H
#define HXLANG_SRC_HXVM_OBJECT_READER_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include <string>
#include <vector>
typedef enum {
    OP_NOP,
    OP_LOAD_CONST,  // 加载常量至栈顶 OP_LOAD_CONST <param_value> | OP_LOAD_CONST
    // <const_index>
    OP_LOAD_VAR,   // 加载变量至栈顶
    OP_POP,
    OP_STORE_VAR,  // 将栈顶值存入变量
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_JMP,
    OP_JMP_CONDITION,
    OP_CAL,  // CAL <proc_index>(u32) <paramCount>(u32)
    OP_RET,
    OP_PRINT_STRING,
    //类型转换
    OP_CHAR_TO_INT,
    OP_INT_TO_CHAR,
    OP_INT_TO_FLOAT,
    OP_CHAR_TO_FLOAT,
    OP_CHAR_TO_STRING,
    OP_FLOAT_TO_INT,
    OP_INT_TO_STRING,
    //连接字符串
    OP_STRING_CONCAT
} Opcode;
enum ParamType {
    PARAM_TYPE_INT,
    PARAM_TYPE_FLOAT,  // double
    PARAM_TYPE_CHAR,
    PARAM_TYPE_BOOL,
    PARAM_TYPE_STRING,
    PARAM_TYPE_ADDRESS,
    PARAM_TYPE_INDEX  // uint32_t 索引常量池或过程表
};
typedef struct Param {
    ParamType type;  // char
    uint8_t size;
    char value[8];
    uint32_t offest;  // 偏移量
} Param;
// 指令
typedef struct Instruction {
    Opcode opcode;  // char
    Param params[3];
} Instruction;
// 过程,用索引访问
typedef struct Procedure {
    uint32_t instructionSize;
    std::vector<Instruction> instructions;
    uint32_t stackSize;     // 栈大小
    uint32_t localVarSize;  // 局部变量数量
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
    uint32_t size;
    Constant* constants;
} ConstantPool;
//----------------------------------
typedef struct ObjectCodeHeader {
    char magic[4];  // 魔数 "HXOC"
} ObjectCodeHeader;
//--------------------------------------
typedef struct ObjectCode {
    ObjectCodeHeader header;
    ConstantPool constantPool;
    uint32_t procedureSize;
    std::vector<Procedure> procedures;
    int32_t start;  // 入口索引
} ObjectCode;

static wchar_t* readWstring(FILE* file) {
    uint32_t byteLen;  // 字节长度（包含末尾 \0 的字节）
    if (fread(&byteLen, sizeof(uint32_t), 1, file) != 1) return nullptr;

    uint32_t charCount = byteLen / sizeof(uint16_t);
    if (charCount == 0) return nullptr;

    // 1. 先读取原始的 u16 数据
    uint16_t* u16Buf = (uint16_t*)malloc(byteLen);
    if (!u16Buf) return nullptr;

    if (fread(u16Buf, 1, byteLen, file) != byteLen) {
        free(u16Buf);
        return nullptr;
    }
    // 转换为当前平台的 wchar_t (处理 sizeof(wchar_t) 可能为 4 的情况)
    wchar_t* wstr = (wchar_t*)malloc(charCount * sizeof(wchar_t));
    if (!wstr) {
        free(u16Buf);
        return nullptr;
    }
    for (uint32_t i = 0; i < charCount; i++) {
        wstr[i] = (wchar_t)u16Buf[i];
    }
    free(u16Buf);
    return wstr;
}
// 读取指令
static int readInstruction(Instruction& instr, FILE* file) {
    // 读取 Opcode (写入时是 char 类型)
    char opcodeChar;
    if (fread(&opcodeChar, sizeof(char), 1, file) != 1) return -1;
    instr.opcode = (Opcode)opcodeChar;
    // 读取 3 个参数
    for (int i = 0; i < 3; i++) {
        char typeChar;
        if (fread(&typeChar, sizeof(char), 1, file) != 1) return -1;
        instr.params[i].type = (ParamType)typeChar;

        if (fread(&(instr.params[i].size), sizeof(uint8_t), 1, file) != 1)
            return -1;
        if (fread(instr.params[i].value, 1, 8, file) != 8) return -1;
        if (fread(&(instr.params[i].offest), sizeof(uint32_t), 1, file) != 1)
            return -1;
    }
    return 0;
}
int readObjectCode(std::string path, ObjectCode& obj) {
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) return -1;

    // 1. 验证头信息 "HXOC"
    char header[4];
    if (fread(header, 1, 4, file) != 4) {
        fclose(file);
        return -1;
    }
    if (header[0] != 'H' || header[1] != 'X' || header[2] != 'O' ||
            header[3] != 'C') {
        fclose(file);
        return -1;
    }
    // 2.读取常量池
    if (fread(&(obj.constantPool.size), sizeof(uint32_t), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    obj.constantPool.constants =
        (Constant*)malloc(sizeof(Constant) * obj.constantPool.size);
    for (uint32_t i = 0; i < obj.constantPool.size; i++) {
        char typeChar;
        if (fread(&typeChar, sizeof(char), 1, file) != 1) {
            fclose(file);
            return -1;
        }
        obj.constantPool.constants[i].type = (ConstantType)typeChar;
        if (obj.constantPool.constants[i].type == CONST_STRING) {
            // 这里将 u16 序列读入并转为 wchar_t*
            obj.constantPool.constants[i].value.string_value = readWstring(file);
            // 重新记录当前平台的真实字节大小（可选）
            if (obj.constantPool.constants[i].value.string_value) {
                obj.constantPool.constants[i].size =
                    (uint32_t)(wcslen(
                                   obj.constantPool.constants[i].value.string_value) *
                               sizeof(wchar_t));
            }
        }
    }
    // 3.读取过程
    uint32_t procCount;
    if (fread(&procCount, sizeof(uint32_t), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    obj.procedureSize = procCount;
    for (uint32_t i = 0; i < procCount; i++) {
        Procedure proc;
        if (fread(&(proc.instructionSize), sizeof(uint32_t), 1, file) != 1) {
            fclose(file);
            return -1;
        }
        for (uint32_t j = 0; j < proc.instructionSize; j++) {
            Instruction instr;
            if (readInstruction(instr, file) != 0) {
                fclose(file);
                return -1;
            }
            proc.instructions.push_back(instr);
        }
        // 读取运行栈和局部变量配置
        if (fread(&(proc.stackSize), sizeof(uint32_t), 1, file) != 1) break;
        if (fread(&(proc.localVarSize), sizeof(uint32_t), 1, file) != 1) break;

        obj.procedures.push_back(proc);
    }
    // 4.读取入口
    if (fread(&(obj.start), sizeof(uint32_t), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}
#endif