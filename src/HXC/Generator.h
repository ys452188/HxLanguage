#ifndef HXLANG_SRC_HXC_GENERATOR_H
#define HXLANG_SRC_HXC_GENERATOR_H
#include "IR.h"
#include <wchar.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "Error.h"
#include "Lexer.h"
#include <stdio.h>
#include <string.h>
#include "ObjectCode.h"
typedef uint16_t wchar;
/*
* 生成目标代码
* @param program: 中间表示
* @param err: 错误码指针，发生错误时会设置为相应错误码
* @return 生成的目标代码对象指针，发生错误时返回NULL
*/
extern ObjectCode* generateObjectCode(IR_Program *program, int *err);
/*
* 生成函数的目标代码
* @param function: 中间表示的函数
* @param err: 错误码指针，发生错误时会设置为相应错误码
* @return 生成的过程对象指针，发生错误时返回NULL
*/
static Procedure* generateFunction(IR_Function* function, int* err);
extern void freeObjectCode(ObjectCode **objCode);
static int getMainFunctionIndex(IR_Program *program) {
    if (!program || !program->functions) return -1;
    int index = -1;
    for (int i = 0; i < program->function_count; i++) {
        if ((program->functions[i] && program->functions[i]->name &&
            wcscmp(program->functions[i]->name, L"main") == 0) ||
            (program->functions[i] && program->functions[i]->name &&
            wcscmp(program->functions[i]->name, L"主函数") == 0)) {
                if(index != -1) {
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
//----------------------------------------------------------------------------
ObjectCode* generateObjectCode(IR_Program *program, int *err) {
    if (!program || !err) {
        if (err) *err = -1;
        return NULL;
    }
    //找入口函数
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
        if (err) *err = -1;
        return NULL;
    }
    // 初始化目标代码对象
    objCode->procedures = (Procedure**)calloc(1, sizeof(Procedure*));
    if(!objCode->procedures) {
        free(objCode);
        if (err) *err = -1;
        return NULL;
    }
    objCode->procedureSize = 1;

    return objCode;
}
static int getClassIndexByName(wchar_t* name, IR_Class** class_table, int class_table_size) {
    if(!name || !class_table) return -1;
    for(int i = 0; i < class_table_size; i++) {
        if(!class_table[i]) continue;
        if(wcscmp(name, class_table[i]->name) == 0) {
            return i;
        }
    }
    return -1;
}
static int getVarSize(IR_DataType type, IR_Class** class_table, int class_table_size) {
    if(!class_table) return 0;
    switch(type.kind) {
        case IR_DT_INT:
            return sizeof(int32_t);
        case IR_DT_FLOAT:
            return sizeof(float);
        case IR_DT_CHAR:
            return sizeof(wchar_t);
        case IR_DT_BOOL:
            return sizeof(bool);
        case IR_DT_CUSTOM:
            return (class_table)[getClassIndexByName(type.custom_type_name, class_table, class_table_size)]->size;
        default:
            return 0;
    }
}
Procedure* generateFunction(IR_Function* function, int* err) {
    if(!function || !err) {
        if(err) *err = -1;
        return NULL;
    }
    Procedure* proc = (Procedure*)calloc(1, sizeof(Procedure));
    if(!proc) {
        *err = -1;
        return NULL;
    }
    proc->paramSize = (uint16_t)(function->param_count);
}
#endif