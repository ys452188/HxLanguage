#ifndef HXLANG_SRC_HXC_GENERATOR_H
#define HXLANG_SRC_HXC_GENERATOR_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <vector>

#include "Error.h"
#include "IR.h"
#include "Lexer.h"
typedef uint16_t wchar;
class FunCallPitch {  // 回填CALL指令,被指向
public:
    FunCallPitch(IR_Function* ir_fun) : fun(ir_fun) {}
    IR_Function* fun;
    int index;
};
class FunCallPitchTable {
    std::vector<FunCallPitch*> pitches;

public:
    FunCallPitch* enter(IR_Function* fun) {
        for (int i = 0; i < pitches.size(); i++) {
            if (pitches.at(i)->fun == fun) return pitches.at(i);
        }
        FunCallPitch* pitch = new FunCallPitch(fun);
        pitches.push_back(pitch);
        return pitch;
    }
#ifdef HX_DEBUG
    void list() {
        fwprintf(logStream, L"回填函数列表：\n");
        for (int i = 0; i < pitches.size(); i++) {
            fwprintf(logStream, L"\t%03u\33[1;32m%ls\33[0m index:%d\n", i,
                     pitches.at(i)->fun->name, pitches.at(i)->index);
        }
    }
#endif
    ~FunCallPitchTable() {
        for (int i = 0; i < pitches.size(); i++) {
#ifdef HX_DEBUG
            fwprintf(logStream, L"释放：%ls\n", pitches.at(i)->fun->name);
#endif
            if (pitches.at(i) != NULL) delete pitches.at(i);
            pitches.at(i) = NULL;
        }
    }
};
#include "ObjectCode.h"
#include "Parser.h"
/*
 * 生成目标代码
 * @param program: 中间表示
 * @param err: 错误码指针，发生错误时会设置为相应错误码
 * @return 生成的目标代码对象指针，发生错误时返回NULL
 */
extern ObjectCode* generateObjectCode(IR_Program* program, int* err);
extern void freeObjectCode(ObjectCode** obj);
static void generateInstructionsFromAST(std::vector<Instruction>& instructions,
                                        int* inst_index, int* inst_size,
                                        ASTNode* node,
                                        ConstantPool* constantPool, int* err);
/*
 * 生成函数的目标代码
 * @param function: 中间表示的函数
 * @param err: 错误码指针，发生错误时会设置为相应错误码
 * @return 生成的过程对象指针，发生错误时返回NULL
 */
static Procedure* generateFunction(IR_Function* function,
                                   FunCallPitchTable& pitchTable,
                                   ConstantPool* constantPool,
                                   IR_Function** all_functions,
                                   int all_function_count, int* err);
/**
 * 生成定义变量的目标代码
 */
static Procedure* generateVariable();
static int getMainFunctionIndex(IR_Program* program) {
    if (!program || !program->functions) return -1;
    int index = -1;
    for (int i = 0; i < program->function_count; i++) {
        if ((program->functions[i] && program->functions[i]->name &&
                wcscmp(program->functions[i]->name, L"main") == 0) ||
                (program->functions[i] && program->functions[i]->name &&
                 wcscmp(program->functions[i]->name, L"主函数") == 0)) {
            if (index != -1) {
                // 已经找到过main函数，说明有重定义
                index = -255;
                setError(ERR_MAIN, program->functions[i]->bodyTokens[0].line,
                         program->functions[i]->name);
                break;
            }
            index = i;
        }
    }
    return index;
}
#ifdef HX_DEBUG
static void listObjectCode_Proc(Procedure* proc) {
    if (!proc) return;
    fwprintf(logStream, L"[DEB] 过程指令数: %u\n", proc->instructionSize);
    for (uint32_t i = 0; i < proc->instructionSize; i++) {
        Instruction ins = proc->instructions[i];
        switch (ins.opcode) {
        case OP_LOAD_CONST: {
            fwprintf(logStream, L"\t%03u: \33[1;34mOP_LOAD_CONST\33[0m", i);
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
                fwprintf(logStream, L" CHAR=\'%lc\'", c);
            } else if (ins.params[0].type == PARAM_TYPE_INDEX) {
                uint32_t idx = 0;
                memcpy(&idx, ins.params[0].value, sizeof(uint32_t));
                fwprintf(logStream, L" INDEX=%u", idx);
            }
            fwprintf(logStream, L"\n");
            break;
        }
        case OP_LOAD_VAR:
            fwprintf(logStream, L"\t%03u: \33[1;34mOP_LOAD_VAR\33[0m index=%d\n", i,
                     (int)ins.params[0].value[0]);
            break;
        case OP_STORE_VAR:
            fwprintf(logStream, L"\t%03u: \33[1;34mOP_STORE_VAR\33[0m\n", i);
            break;
        case OP_ADD:
            fwprintf(logStream, L"\t%03u: \33[1;34mOP_ADD\33[0m\n", i);
            break;
        case OP_SUB:
            fwprintf(logStream, L"\t%03u: \33[1;34mOP_SUB\33[0m\n", i);
            break;
        case OP_MUL:
            fwprintf(logStream, L"\t%03u: \33[1;34mOP_MUL\33[0m\n", i);
            break;
        case OP_DIV:
            fwprintf(logStream, L"\t%03u: \33[1;34mOP_DIV\33[0m\n", i);
            break;
        case OP_CAL:
            fwprintf(
                logStream,
                L"\t%03u: \33[1;34mOP_CAL\33[0m %ls(funName), %u(paramCount)\n", i,
                ins.pitch == NULL ? L" " : ins.pitch->fun->name,
                *((uint32_t*)ins.params[1].value));
            break;
        case OP_RET:
            fwprintf(logStream, L"\t%03u: \33[1;34mOP_RET\33[0m\n", i);
            break;
        case OP_PRINT_STRING:
            fwprintf(logStream, L"\t%03u: \33[1;34mOP_PRINT_STRING\33[0m\n", i);
            break;
        case OP_CHAR_TO_INT:
            fwprintf(logStream, L"\t%03u: \33[1;34m OP_CHAR_TO_INT\33[0m\n", i);
            break;
        case OP_INT_TO_CHAR:
            fwprintf(logStream, L"\t%03u: \33[1;34m OP_INT_TO_CHAR\33[0m\n", i);
            break;
        case OP_INT_TO_FLOAT:
            fwprintf(logStream, L"\t%03u: \33[1;34m OP_INT_TO_CHAR\33[0m\n", i);
            break;
        case OP_CHAR_TO_FLOAT:
            fwprintf(logStream, L"\t%03u: \33[1;34m OP_CHAR_TO_FLOAT\33[0m\n", i);
            break;
        case OP_CHAR_TO_STRING:
            (logStream, L"\t%03u: \33[1;34m OP_CHAR_TO_STRING\33[0m\n", i);
            break;
        case OP_FLOAT_TO_INT:
            (logStream, L"\t%03u: \33[1;34m OP_FLOAT_TO_INT\33[0m\n", i);
            break;
        case OP_INT_TO_STRING:
            fwprintf(logStream, L"\t%03u: \33[1;34m OP_INT_TO_STRING\33[0m\n", i);
            break;
        case OP_POP:
            fwprintf(logStream, L"\t%03u: \33[1;34m OP_POP\33[0m\n", i);
            break;
        default:
            fwprintf(logStream, L"\t%03u: \33[1;31mOP_NOP\33[0m\n", i);
        }
    }
}
#endif
static void markUsedFun(Procedure* fun, std::vector<Procedure*>& objFun) {
    // 防空指针且防止循环调用（A调B，B调A）导致的爆栈
    if (fun == nullptr || fun->isUsed) {
        return;
    }
    fun->isUsed = true;
    // 遍历当前过程的所有指令，寻找函数调用
    for (size_t i = 0; i < fun->instructions.size(); i++) {
        Instruction& instr = fun->instructions.at(i);
        if (instr.opcode == OP_CAL) {
            // 安全检查，确保 pitch 和 fun 都存在
            if (instr.pitch != nullptr && instr.pitch->fun != nullptr) {
                IR_Function* targetFun = instr.pitch->fun;
                Procedure* targetProc = targetFun->proc;

                if (targetProc != nullptr) {
                    targetFun->isUsed = true;  // 标记 IR 层的函数
#ifdef HX_DEBUG
                    log(L"标记函数: %ls", targetFun->name);
#endif
                    // 递归追踪被调用的函数
                    markUsedFun(targetProc, objFun);
                }
            }
        }
    }
}
//----------------------------------------------------------------------------
ObjectCode* generateObjectCode(IR_Program* program, int* err) {
    if (!program || !err) {
        if (err) *err = -1;
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
    ObjectCode* objCode = new ObjectCode;
    objCode->constantPool.size = 0;
    objCode->constantPool.constants = NULL;
    objCode->procedureSize = 0;
    if (!objCode) {
        if (err) *err = -1;
        return NULL;
    }
    objCode->procedureSize = 0;
// 生成main函数的目标代码
#ifdef HX_DEBUG
    log(L"生成函数的目标代码...");
#endif
    FunCallPitchTable pitchTable;
    std::vector<Procedure*> objFun;
    for (int i = 0; i < program->function_count; i++) {
#ifdef HX_DEBUG
        fwprintf(logStream, L"编译函数 %ls\n", program->functions[i]->name);
#endif
        objFun.push_back(generateFunction(
                             program->functions[i], pitchTable, &(objCode->constantPool),
                             program->functions, program->function_count, err));
        objFun.at(objFun.size() - 1)->fun = program->functions[i];
        if (*err != 0) return NULL;
#ifdef HX_DEBUG
        listObjectCode_Proc(objFun.at(i));
#endif
    }
    if (*err != 0) {
        // freeObjectCode(&objCode);
        return NULL;
    }

    // main在目标中索引等于中间表示中的索引
    markUsedFun(objFun.at(mainIndex), objFun);
    // 写入函数
    for (int i = 0; i < objFun.size(); i++) {
        if (objFun.at(i)->isUsed == true) {
#ifdef HX_DEBUG
            log(L"写入函数%ls", objFun.at(i)->fun->name);
#endif
            objCode->procedures.push_back(objFun.at(i));
            // objFun.at(i)->fun->pitch->index = objCode->procedureSize;
            (pitchTable.enter(objFun.at(i)->fun))->index = objCode->procedureSize;
            if (i == mainIndex) {
                objCode->start = objCode->procedureSize;
#ifdef HX_DEBUG
                log(L"设置入口函数%ls[%d]",
                    objCode->procedures.at(objCode->start)->fun->name, objCode->start);
#endif
            }
            objCode->procedureSize++;
        }
    }

#ifdef HX_DEBUG
    pitchTable.list();
#endif

    // 回填
    for (int i = 0; i < objCode->procedureSize; i++) {
        std::vector<Instruction>& inst = objCode->procedures.at(i)->instructions;
        for (int instIndex = 0; instIndex < inst.size(); instIndex++) {
            if (inst.at(instIndex).opcode == OP_CAL) {
                memcpy((inst.at(instIndex).params[0].value),
                       &(inst.at(instIndex).pitch->index), sizeof(uint32_t));
#ifdef HX_DEBUG
                uint32_t* index = (uint32_t*)(inst.at(instIndex).params[0].value);
                log(L"设置OP_CALL %d", *index);
#endif
            }
        }
    }
    return objCode;
}
static int getClassIndexByName(wchar_t* name, IR_Class** class_table,
                               int class_table_size) {
    if (!name || !class_table) return -1;
    for (int i = 0; i < class_table_size; i++) {
        if (!class_table[i]) continue;
        if (wcscmp(name, class_table[i]->name) == 0) {
            return i;
        }
    }
    return -1;
}
static int getVarSize(IR_DataType type, IR_Class** class_table,
                      int class_table_size) {
    if (!class_table) return 0;
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
        return (class_table)[getClassIndexByName(type.customTypeName, class_table,
                                                                      class_table_size)]
               ->size;
    default:
        return 0;
    }
}
Procedure* generateFunction(IR_Function* function,
                            FunCallPitchTable& pitchTable,
                            ConstantPool* constantPool,
                            IR_Function** all_functions, int all_function_count,
                            int* err) {
    if (!function || !err) {
        if (err) *err = -1;
        return NULL;
    }
    initLocale();
    Procedure* proc = new Procedure;
    if (!proc) {
        *err = -1;
        return NULL;
    }
    proc->fun = function;
    if (function->body_token_count == 0 || !function->bodyTokens) {
        // 空函数体
        proc->instructionSize = 0;
        proc->localVarSize = 0;
        proc->stackSize = 0;
        return proc;
    }
    int index = 0;
    SymbolTable localeSymbolTable = {0};
    // 填充函数表以便解析器能解析函数调用
    localeSymbolTable.fun = all_functions;
    localeSymbolTable.fun_size = all_function_count;
    proc->instructionSize = 0;
    while (index < function->body_token_count) {
        Token currentToken = function->bodyTokens[index];
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
                delete (proc);
                return NULL;
            }
            index++;  // 指向冒号
            if (function->bodyTokens[index].type == TOK_END) {
                if (function->returnType.kind != IR_DT_VOID &&
                        function->isReturnTypeKnown) {
                    setError(ERR_RET_VAL, currentToken.line, NULL);
                    *err = 255;
                    delete (proc);
                    return NULL;
                }
                // 空返回
#ifdef HX_DEBUG
                log(L"生成空返回指令\n");
#endif
                Instruction newInst = {};
                newInst.opcode = OP_RET;
                proc->instructions.push_back(newInst);
                proc->instructionSize++;
                continue;
            }
            //:
            if (function->bodyTokens[index].type != TOK_OPR_COLON) {
                setError(ERR_RET, currentToken.line, NULL);
                *err = 255;
                delete (proc);
                return NULL;
            }
            index++;  // 指向表达式起始位置
            // wprintf(L"%ls", function->bodyTokens[index].value);
            ASTNode* expNode = parseExpression(function->bodyTokens, &index,
                                               function->body_token_count, pitchTable,
                                               &localeSymbolTable, err);
            if (*err != 0 || !expNode) {
                delete (proc);
                return NULL;
            }
#ifdef HX_DEBUG
            log(L"生成返回值表达式指令...\n");
#endif
            // 生成表达式指令
            int inst_index = proc->instructionSize;
            int inst_size = proc->instructionSize;
            generateInstructionsFromAST(proc->instructions, &inst_index, &inst_size,
                                        expNode, constantPool, err);
            if (*err != 0) {
                freeAST(expNode);
                return NULL; /* 保留 generateInstructionsFromAST 设置的错误码 */
            }
            proc->instructionSize = inst_index;
            freeAST(expNode);
            // 生成返回指令
            Instruction newInst = {};
            newInst.opcode = OP_RET;
            proc->instructions.push_back(newInst);
            proc->instructionSize++;
            index++;
            if (function->bodyTokens[index].type != TOK_END) {
                setError(ERR_RET, currentToken.line, NULL);
                *err = 255;
                freeAST(expNode);
                delete (proc);
                return NULL;
            }
        } else if (wcscmp(currentToken.value, L"__hx_write_string__") == 0) {
            if (index + 1 >= function->body_token_count) {
                continue;
            }
            index++;
            if ((function->bodyTokens[index].type != TOK_VAL) ||
                    (function->bodyTokens[index].mark != STR))
                continue;
            Instruction newInst = {};
            newInst.opcode = OP_PRINT_STRING;

            constantPool->constants = (Constant*)realloc(
                                          constantPool->constants, sizeof(Constant) * (constantPool->size + 1));
            if (!constantPool->constants) {
                *err = -1;
                return NULL;
            }
            constantPool->constants[constantPool->size].type = CONST_STRING;
            constantPool->constants[constantPool->size].value.string_value =
                (wchar_t*)calloc(wcslen(function->bodyTokens[index].value) + 1,
                                 sizeof(wchar_t));
            if (!constantPool->constants[constantPool->size].value.string_value) {
                *err = -1;
                return NULL;
            }
            wcscpy(constantPool->constants[constantPool->size].value.string_value,
                   function->bodyTokens[index].value);
            constantPool->constants[constantPool->size].size =
                (uint16_t)wcslen(function->bodyTokens[index].value) *
                sizeof(uint16_t);
            constantPool->size += 1;
            newInst.params[0].type = PARAM_TYPE_INDEX;
            uint32_t strIndex = constantPool->size - 1;
            memcpy(newInst.params[0].value, &(strIndex), sizeof(uint32_t));
            newInst.params[0].size = sizeof(uint32_t);

            proc->instructions.push_back(newInst);
            proc->instructionSize++;
        } else if (currentToken.type == TOK_ID) {  // 赋值或调用函数
            ASTNode* expNode = parseExpression(function->bodyTokens, &index,
                                               function->body_token_count, pitchTable,
                                               &localeSymbolTable, err);
            if (*err != 0 || !expNode) {
                delete (proc);
                return NULL;
            }
            // 生成表达式指令
            int inst_index = proc->instructionSize;
            int inst_size = proc->instructionSize;
            generateInstructionsFromAST(proc->instructions, &inst_index, &inst_size,
                                        expNode, constantPool, err);
            if(expNode->kind == NODE_FUN_CALL) {
                //有返回值要手动弹出
                if(expNode->data.funCall.ret_type.kind != IR_DT_VOID) {
                    Instruction popInst = {};
                    popInst.opcode = OP_POP;
                    proc->instructions.push_back(popInst);
                    inst_index++;
                    inst_size++;
                }
            }
            if (*err != 0) {
                freeAST(expNode);
                return NULL; /* 保留 generateInstructionsFromAST 设置的错误码 */
            }
            proc->instructionSize = inst_index;
            freeAST(expNode);
        } else if (wcscmp(currentToken.value, L"var") ==
                   0) {  // var:id[->type][=exp];
            if (index + 1 >= function->body_token_count) {
                setError(ERR_DEF_VAR, currentToken.line, NULL);
                *err = 255;
                delete (proc);
                return NULL;
            }
            index++;  // 指向冒号
            if (function->bodyTokens[index].type != TOK_OPR_COLON) {
                setError(ERR_DEF_VAR, currentToken.line, NULL);
                *err = 255;
                delete (proc);
                return NULL;
            }
            if (index + 1 >= function->body_token_count) {
                setError(ERR_DEF_VAR, currentToken.line, NULL);
                *err = 255;
                delete (proc);
                return NULL;
            }
            index++;  // 指向标识符
            if (function->bodyTokens[index].type != TOK_ID) {
                setError(ERR_DEF_VAR, currentToken.line, NULL);
                *err = 255;
                delete (proc);
                return NULL;
            }
            if (index + 1 >= function->body_token_count) {
                setError(ERR_DEF_VAR, currentToken.line, NULL);
                *err = 255;
                delete (proc);
                return NULL;
            }
            index++;  // 指向结束标志或->或=
            if (function->bodyTokens[index].type == TOK_OPR_POINT) {
            } else if (function->bodyTokens[index].type == TOK_OPR_SET) {
            } else if (function->bodyTokens[index].type == TOK_END) {
            } else {
                setError(ERR_DEF_VAR, currentToken.line, NULL);
                *err = 255;
                delete (proc);
                return NULL;
            }
        }
        index++;
    }
    function->proc = proc;
    return proc;
}
void generateInstructionsFromAST(std::vector<Instruction>& instructions,
                                 int* inst_index, int* inst_size, ASTNode* node,
                                 ConstantPool* constantPool, int* err) {
    if (!inst_index || !node || !inst_size || !err || !constantPool) {
        if (err) *err = -1;
        return;
    }
    Instruction newInst = {};
    if (node->kind == NODE_VALUE) {
        newInst.opcode = OP_LOAD_CONST;
        // 设置参数
        if (node->data.value.type.kind == IR_DT_INT) {
            newInst.params[0].type = PARAM_TYPE_INT;
            memcpy(newInst.params[0].value, &(node->data.value.val.i),
                   sizeof(int32_t));
            newInst.params[0].size = sizeof(int32_t);
            newInst.params[1].type = PARAM_TYPE_INT;
        } else if (node->data.value.type.kind == IR_DT_FLOAT) {
            newInst.params[0].type = PARAM_TYPE_FLOAT;
            memcpy(newInst.params[0].value, &(node->data.value.val.f),
                   sizeof(double));
            newInst.params[0].size = sizeof(double);
            newInst.params[1].type = PARAM_TYPE_FLOAT;

        } else if (node->data.value.type.kind == IR_DT_CHAR) {
            newInst.params[0].type = PARAM_TYPE_CHAR;
            memcpy(newInst.params[0].value, &(node->data.value.val.c),
                   sizeof(uint16_t));
            newInst.params[0].size = sizeof(uint16_t);
        } else if (node->data.value.type.kind == IR_DT_STRING) {
            // 加入常量池
            constantPool->constants = (Constant*)realloc(
                                          constantPool->constants, sizeof(Constant) * (constantPool->size + 1));
            if (!constantPool->constants) {
                *err = -1;
                return;
            }
            constantPool->constants[constantPool->size].type = CONST_STRING;
            constantPool->constants[constantPool->size].value.string_value =
                (wchar_t*)calloc(wcslen(node->data.value.val.s) + 1, sizeof(wchar_t));
            if (!constantPool->constants[constantPool->size].value.string_value) {
                *err = -1;
                return;
            }
            wcscpy(constantPool->constants[constantPool->size].value.string_value,
                   node->data.value.val.s);
            constantPool->constants[constantPool->size].size =
                (uint16_t)wcslen(node->data.value.val.s) * sizeof(uint16_t);
            constantPool->size += 1;
            newInst.params[0].type = PARAM_TYPE_INDEX;
            uint32_t strIndex = constantPool->size - 1;
            memcpy(newInst.params[0].value, &(strIndex), sizeof(uint32_t));
            newInst.params[0].size = sizeof(uint32_t);
        } else {
            *err = -1;
            return;
        }
        (*inst_index)++;
        //类型转换
        /*if(node->typeCast != OP_NOP) {
            Instruction typeCastInst = {};
            typeCastInst.opcode = node->typeCast;
            instructions.push_back(typeCastInst);
            (*inst_index)++;
            (*inst_size)++;
        }*/
    } else if (node->kind == NODE_BINARY) {
        // 生成左子树指令
        generateInstructionsFromAST(instructions, inst_index, inst_size, node->left,
                                    constantPool, err);
        if (*err != 0) {
            return;
        }
        if(node->left->typeCast != OP_NOP) {
            Instruction typeCastInst = {};
            typeCastInst.opcode = node->left->typeCast;
            instructions.push_back(typeCastInst);
            (*inst_index)++;
            (*inst_size)++;
        }
        // 生成右子树指令
        generateInstructionsFromAST(instructions, inst_index, inst_size,
                                    node->right, constantPool, err);
        if (*err != 0) {
            return;
        }
        if(node->right->typeCast != OP_NOP) {
            Instruction typeCastInst = {};
            typeCastInst.opcode = node->right->typeCast;
            instructions.push_back(typeCastInst);
            (*inst_index)++;
            (*inst_size)++;
        }
        // 生成二元运算指令
        switch (node->data.binary.op) {
        case 0:  // ADD
            newInst.opcode = OP_ADD;
            break;
        case 1:  // SUB
            newInst.opcode = OP_SUB;
            break;
        case 2:  // MUL
            newInst.opcode = OP_MUL;
            break;
        case 3:  // DIV
            newInst.opcode = OP_DIV;
            break;
        case 4: {  //SET
            if (node->left->kind != NODE_VAR && node->left->kind != NODE_BINARY &&
                    node->left->data.binary.op != 4) {
                *err = 255;
                setError(ERR_EXP, node->token->line, NULL);
                return;
            }

            break;
        }
        case 5: {   //STRING_CONCAT
            newInst.opcode = OP_STRING_CONCAT;
            break;
        }
        default:
            *err = -1;
            return;
        }
        (*inst_index)++;
    } else if (node->kind == NODE_VAR) {
        newInst.opcode = OP_LOAD_VAR;
        newInst.params[0].type = PARAM_TYPE_INDEX;
        memcpy(newInst.params[0].value, &(node->data.var.index), sizeof(uint32_t));
        newInst.params[0].size = sizeof(uint32_t);
        switch (node->data.var.type.kind) {
        case IR_DT_INT:
            newInst.params[1].type = PARAM_TYPE_INT;
            break;
        case IR_DT_FLOAT:
            newInst.params[1].type = PARAM_TYPE_FLOAT;
            break;
        case IR_DT_CHAR:
            newInst.params[1].type = PARAM_TYPE_CHAR;
            break;
        case IR_DT_STRING:
            newInst.params[1].type = PARAM_TYPE_STRING;
            break;
        }
        (*inst_index)++;
    } else if (node->kind == NODE_FUN_CALL) {
        // 未解析到函数索引则报错（未能在符号表中解析函数）
        /*if (node->data.funCall.index < 0) {
            setError(ERR_CANNOT_FIND_SYMBOL, node->token ? node->token->line : 0,
                     node->data.funCall.name);
            *err = 255;
            free(instructions);
            return NULL;
        }*/
        if (node->data.funCall.arg_count == 0) {
        } else {
            // 生成参数指令
            for (uint32_t i = 0; i < node->data.funCall.arg_count; i++) {
                generateInstructionsFromAST(instructions, inst_index, inst_size,
                                            node->data.funCall.args[i], constantPool,
                                            err);
                if (*err != 0) {
                    return;
                }
            }
        }
        // 生成调用指令
        newInst.opcode = OP_CAL;
        newInst.params[0].type = PARAM_TYPE_INDEX;
        /*******(*********)***********************************************
         *                  先设置pitch，后面再回填(pitch在Parser已添加))
         *********************************-*****--**********************/
        newInst.params[0].size = sizeof(uint32_t);
        newInst.pitch = node->data.funCall.pitch;

        newInst.params[1].type = PARAM_TYPE_INT;
        memcpy(newInst.params[1].value, &(node->data.funCall.arg_count),
               sizeof(uint32_t));
        newInst.params[1].size = sizeof(uint32_t);
        (*inst_index)++;
    } else {
        *err = -1;
        return;
    }
    instructions.push_back(newInst);
    (*inst_size)++;
    return;
}
extern void freeObjectCode(ObjectCode** obj) {
    if (!obj || !(*obj)) return;
    // 释放常量池
    if ((*obj)->constantPool.constants) {
        for (int i = 0; i < (*obj)->constantPool.size; i++) {
            if ((*obj)->constantPool.constants[i].value.string_value) {
                free((*obj)->constantPool.constants[i].value.string_value);
                (*obj)->constantPool.constants[i].value.string_value = NULL;
            }
        }
        free((*obj)->constantPool.constants);
        (*obj)->constantPool.constants = NULL;
    }
    for (int i = 0; i < (*obj)->procedures.size(); i++) {
        if ((*obj)->procedures.at(i)) {
            delete (*obj)->procedures.at(i);
            (*obj)->procedures.at(i) = NULL;
        }
    }
    delete *obj;
    *obj = NULL;
}
#endif