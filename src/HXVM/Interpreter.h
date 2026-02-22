#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include "ObjectReader.h"
static thread_local int callDepth = 0;
#define OP_STACK_SIZE 512     // 操作数栈大小
typedef enum OpStackType {
    TYPE_INT,
    TYPE_FLOAT,    //double
    TYPE_CHAR,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_ADDRESS,
} StackType;
typedef struct _OpStack {
    OpStackType type;
    int size;
    char value[8];
} _OpStack;
typedef struct OpStack {
    _OpStack opStack[OP_STACK_SIZE];
    int top;
} OpStack;
typedef struct Symbol {
    OpStackType type;
    void* address;
} Symbol;

static int interpretProcedure(Procedure& proc, OpStack& opStack, ObjectCode& obj, std::vector<char>& stack, int& usedStackSize, std::vector<Symbol>& localSymbol);
static int interpretInstruction(Instruction& inst, OpStack& opStack, std::vector<char>& stack, int& usedStackSize, ObjectCode& obj);
int interpret(ObjectCode& obj, int& err) {
#ifdef HX_DEBUG
    wprintf(LOG_LABEL L"开始解释\n");
#endif
    //找入口
    if (obj.procedures.empty() || obj.start >= obj.procedures.size()) {
        fwprintf(errorStream, ERR_LABEL L"无效的程序入口索引 %d\n", obj.start);
        err = -1;
        return -1;
    }
    Procedure& entry = obj.procedures.at(obj.start);
    std::vector<char> stack(entry.stackSize);
    int usedStackSize = 0;
    std::vector<Symbol> localSymbol(entry.localVarSize);
    OpStack opStack = {};
    if(interpretProcedure(entry, opStack, obj, stack, usedStackSize, localSymbol)) {
        err = -1;
        return -1;
    }
    return 0;
}

int interpretProcedure(Procedure& proc, OpStack& opStack, ObjectCode& obj, std::vector<char>& stack, int& usedStackSize, std::vector<Symbol>& localSymbol) {
    callDepth++;
    if(callDepth > CALL_DEPTH_MAX) {
        fwprintf(errorStream, ERR_LABEL L"递归调用过多导致的栈溢出\n");
        return -1;
    }
#ifdef HX_DEBUG
    wprintf(LOG_LABEL L"解释过程\n");
#endif
    for(int i = 0; i < proc.instructionSize; i++) {
#ifdef HX_DEBUG
        wprintf(LOG_LABEL L"解释第%d指令\n", i);
#endif
        if(interpretInstruction(proc.instructions.at(i), opStack, stack, usedStackSize, obj)) return -1;
    }
    return 0;
}
static inline int promoteNumeric(_OpStack& a, _OpStack& b) {
    // 只处理 int / float，其它类型交给上层报错
    if ((a.type != TYPE_INT && a.type != TYPE_FLOAT) ||
            (b.type != TYPE_INT && b.type != TYPE_FLOAT)) {
        return -1;
    }

    // 只要有一个是 float，就全部提升为 float
    if (a.type == TYPE_FLOAT || b.type == TYPE_FLOAT) {
        if (a.type == TYPE_INT) {
            int32_t v = *(int32_t*)a.value;
            double d = (double)v;
            memcpy(a.value, &d, sizeof(double));
            a.type = TYPE_FLOAT;
            a.size = sizeof(double);
        }
        if (b.type == TYPE_INT) {
            int32_t v = *(int32_t*)b.value;
            double d = (double)v;
            memcpy(b.value, &d, sizeof(double));
            b.type = TYPE_FLOAT;
            b.size = sizeof(double);
        }
    }
    return 0;
}
int interpretInstruction(Instruction& inst, OpStack& opStack, std::vector<char>& stack, int& usedStackSize, ObjectCode& obj) {
#ifdef HX_DEBUG
    wprintf(LOG_LABEL L"解释指令\n");
#endif
    switch (inst.opcode) {
    case OP_LOAD_CONST: {
#ifdef HX_DEBUG
        wprintf(LOG_LABEL L"加载常量到操作数栈)\n");
#endif
        if(opStack.top >= OP_STACK_SIZE) {
            fwprintf(errorStream, ERR_LABEL L"栈溢出\n");
            return -1;
        }
        switch(inst.params[0].type) {
        case PARAM_TYPE_INT:
            opStack.opStack[opStack.top].type = TYPE_INT;
            opStack.opStack[opStack.top].size = sizeof(int32_t);
            break;
        case PARAM_TYPE_FLOAT:
            opStack.opStack[opStack.top].type = TYPE_FLOAT;
            opStack.opStack[opStack.top].size = sizeof(double);
            break;
        case PARAM_TYPE_CHAR:
            opStack.opStack[opStack.top].type = TYPE_CHAR;
            opStack.opStack[opStack.top].size = sizeof(uint16_t);
            break;
        case PARAM_TYPE_BOOL:
            opStack.opStack[opStack.top].type = TYPE_BOOL;
            opStack.opStack[opStack.top].size = sizeof(char);
            break;
        case PARAM_TYPE_STRING:
            opStack.opStack[opStack.top].type = TYPE_STRING;
            opStack.opStack[opStack.top].size = sizeof(uint16_t*);
            break;
        case PARAM_TYPE_ADDRESS:
            opStack.opStack[opStack.top].type = TYPE_ADDRESS;
            opStack.opStack[opStack.top].size = sizeof(void*);
            break;
        default:
            fwprintf(errorStream, ERR_LABEL L"非法指令格式\n");
            return -1;
            break;
        }
        if(inst.params[0].type==PARAM_TYPE_STRING) {
            if(inst.params[1].type != PARAM_TYPE_INDEX) {
                fwprintf(errorStream, ERR_LABEL L"非法指令格式\n");
                return -1;
            }
            uint32_t* index = (uint32_t*)(&(inst.params[1].value));
            if(obj.constantPool.constants[*index].type != CONST_STRING) {
                fwprintf(errorStream, ERR_LABEL L"非法指令格式\n");
                return -1;
            }
            wchar_t* wstr = obj.constantPool.constants[*index].value.string_value;
            memcpy(opStack.opStack[opStack.top].value, wstr, sizeof(wchar_t*));
        } else {
            if(inst.params[0].type == PARAM_TYPE_INDEX) {
                fwprintf(errorStream, ERR_LABEL L"非法指令格式\n");
                return -1;
            }
            memcpy(opStack.opStack[opStack.top].value, inst.params[0].value, 8);
#ifdef HX_DEBUG
            switch(opStack.opStack[opStack.top].type) {
            case TYPE_INT:
                wprintf(LOG_LABEL L"%d\n", *((int32_t*)opStack.opStack[opStack.top].value));
                break;
            case TYPE_FLOAT:
                wprintf(LOG_LABEL L"%lf\n", *((double*)opStack.opStack[opStack.top].value));
                break;
            }
#endif
        }
        opStack.top++;
        break;
    }
    case OP_ADD: {
#ifdef HX_DEBUG
        wprintf(LOG_LABEL L"相加\n");
#endif
        if (opStack.top < 2) {
            fwprintf(errorStream, ERR_LABEL L"栈中操作数不够\n");
            return -1;
        }
        _OpStack rhs = opStack.opStack[--opStack.top];
        _OpStack lhs = opStack.opStack[--opStack.top];
        if (promoteNumeric(lhs, rhs) != 0) {
            fwprintf(errorStream, ERR_LABEL L"不支持的加法操作数类型\n");
            return -1;
        }
        _OpStack result = {};
        if (lhs.type == TYPE_INT) {
            int32_t res = *(int32_t*)lhs.value + *(int32_t*)rhs.value;
            result.type = TYPE_INT;
            result.size = sizeof(int32_t);
            memcpy(result.value, &res, sizeof(int32_t));
#ifdef HX_DEBUG
            wprintf(LOG_LABEL L"结果：%d\n", res);
#endif
        } else {
            double res = *(double*)lhs.value + *(double*)rhs.value;
            result.type = TYPE_FLOAT;
            result.size = sizeof(double);
            memcpy(result.value, &res, sizeof(double));
#ifdef HX_DEBUG
            wprintf(LOG_LABEL L"结果：%lf\n", res);
#endif
        }
        if (opStack.top >= OP_STACK_SIZE) {
            fwprintf(errorStream, ERR_LABEL L"操作数栈溢出\n");
            return -1;
        }

        opStack.opStack[opStack.top++] = result;
        break;
    }
    case OP_SUB: {
        if (opStack.top < 2) {
            fwprintf(errorStream, ERR_LABEL L"栈中操作数不够\n");
            return -1;
        }
        _OpStack rhs = opStack.opStack[--opStack.top];
        _OpStack lhs = opStack.opStack[--opStack.top];

        if (promoteNumeric(lhs, rhs) != 0) {
            fwprintf(errorStream, ERR_LABEL L"不支持的减法操作数类型\n");
            return -1;
        }

        _OpStack result = {};

        if (lhs.type == TYPE_INT) {
            int32_t res = *(int32_t*)lhs.value - *(int32_t*)rhs.value;
            result.type = TYPE_INT;
            result.size = sizeof(int32_t);
            memcpy(result.value, &res, sizeof(int32_t));
        } else {
            double res = *(double*)lhs.value - *(double*)rhs.value;
            result.type = TYPE_FLOAT;
            result.size = sizeof(double);
            memcpy(result.value, &res, sizeof(double));
        }

        opStack.opStack[opStack.top++] = result;
        break;
    }
    case OP_MUL: {
        if (opStack.top < 2) {
            fwprintf(errorStream, ERR_LABEL L"栈中操作数不够\n");
            return -1;
        }
        _OpStack rhs = opStack.opStack[--opStack.top];
        _OpStack lhs = opStack.opStack[--opStack.top];

        if (promoteNumeric(lhs, rhs) != 0) {
            fwprintf(errorStream, ERR_LABEL L"不支持的乘法操作数类型\n");
            return -1;
        }

        _OpStack result = {};

        if (lhs.type == TYPE_INT) {
            int32_t res = (*(int32_t*)lhs.value)*(*(int32_t*)rhs.value);
            result.type = TYPE_INT;
            result.size = sizeof(int32_t);
            memcpy(result.value, &res, sizeof(int32_t));
        } else {
            double res = (*(double*)lhs.value)*(*(double*)rhs.value);
            result.type = TYPE_FLOAT;
            result.size = sizeof(double);
            memcpy(result.value, &res, sizeof(double));
        }

        opStack.opStack[opStack.top++] = result;
        break;
    }
    case OP_CAL: {   //CAL <proc_index>(u32) <paramCount>(u32)
#ifdef HX_DEBUG
        wprintf(LOG_LABEL L"调用\n");
#endif
        if(inst.params[0].type != PARAM_TYPE_INDEX) {
            fwprintf(errorStream, ERR_LABEL L"非法指令格式\n");
            return -1;
        }
        if(inst.params[1].type != PARAM_TYPE_INT) {
            fwprintf(errorStream, ERR_LABEL L"非法指令格式\n");
            return -1;
        }
        int32_t argCount = *((uint32_t*)inst.params[1].value);
        Procedure& proc = (obj.procedures.at(*((int32_t*)inst.params[0].value)));
        std::vector<char> localStack(proc.stackSize);
        std::vector<Symbol> localSymbol(proc.localVarSize);
        if(opStack.top < argCount) {
            fwprintf(errorStream, ERR_LABEL L"参数不够\n");
            return -1;
        }
        int localSymbolTop = argCount-1;
        int usedStackSize = 0;
        for(int i = opStack.top - 1; i >= opStack.top - argCount; i--) {
            localSymbol[localSymbolTop--].type = opStack.opStack[i].type;
            usedStackSize += opStack.opStack[i].size;
        }
        if(interpretProcedure(proc, opStack, obj, localStack, usedStackSize, localSymbol)) return -1;
        break;
    }
    case OP_PRINT_STRING: {
        wprintf(L"%ls", obj.constantPool.constants[*((uint32_t*)inst.params[0].value)].value.string_value);
        break;
    }
    case OP_RET: {
#ifdef HX_DEBUG
        wprintf(LOG_LABEL L"返回\n");
#endif
        return 0;
        break;
    }
    }
    return 0;
}