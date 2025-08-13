#ifndef HXLANG_HXVM_H
#define HXLANG_HXVM_H
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdbool.h>
#include <stdint.h>
#include "hxLocale.h"
#include "hsmLoader.h"
#define STACK_SIZE_MAX 128
#define STACK_FRAME_SIZE_MAX 1024

typedef enum HXVMErrorType {
    ERR_STACK_OVERFLOW,         //栈溢出
    ERR_NULL_PTR,               //空指针
} HXVMErrorType;
typedef struct Symbol {
    wchar* name;
    wchar* type;
    bool isOnlyRead;
    void* address;       //地址
} Symbol;
typedef struct StackFrame {  //栈帧
    ObjFunction* func;    //指向函数表中的函数
    Symbol* localeSymbol;    //局部变量
    int localeSymbolSize;
} StackFrame;
typedef struct StackType {
    void* value;
    enum {
        STR = 1,
        INT,
        FLOAT,
        DOUBLE,
        CHAR,
        WCHAR,
    } type;
} StackType;
typedef struct HXVM {
    StackFrame stackFrame[STACK_FRAME_SIZE_MAX];
    StackType  stack[STACK_SIZE_MAX];
    int top_stack;
    int top_StackFrame;
} HXVM;

extern ObjectCode hsmCode;
extern HXVM vm;
HXVM vm = {0};

void HXVMError(HXVMErrorType errType);
int pushValueIntoStack(StackType*);           //表面值入栈
void popValueOutOfStack(void);                //表面值出栈
void popFunOutOfStackFrame(void);
int pushFunIntoStackFrame(ObjFunction*);      //函数入栈
int pushFunIntoStackFrame(ObjFunction* fun) {
    if(vm.top_StackFrame >= STACK_FRAME_SIZE_MAX) {
        HXVMError(ERR_STACK_OVERFLOW);
        return -1;
    }
    vm.stackFrame[vm.top_StackFrame].func = fun;
    vm.stackFrame[vm.top_StackFrame].localeSymbol = NULL;
    vm.stackFrame[vm.top_StackFrame].localeSymbolSize = 0;
#ifdef SHOW_HX_DEBUG_DETAIL
    printf("\33[33m[DEG]\33[0m函数%ls已入栈(%p).\n", vm.stackFrame[vm.top_StackFrame].func->name, &(vm.stackFrame[vm.top_StackFrame]));
#endif
    vm.top_StackFrame++;
    return 0;
}
void popFunOutOfStackFrame(void) {
    if(vm.top_StackFrame > 0) { // 确保栈帧不为空
        vm.top_StackFrame--; // 先将索引移到正确的位置
#ifdef SHOW_HX_DEBUG_DETAIL
        printf("\33[33m[DEG]\33[0m正在弹出函数%ls...\n",vm.stackFrame[vm.top_StackFrame].func->name);
#endif
        vm.stackFrame[vm.top_StackFrame].func = NULL;
        if(vm.stackFrame[vm.top_StackFrame].localeSymbol) {
            for(int i = 0; i < vm.stackFrame[vm.top_StackFrame].localeSymbolSize; i++) {
                if(vm.stackFrame[vm.top_StackFrame].localeSymbol[i].name) {
                    free(vm.stackFrame[vm.top_StackFrame].localeSymbol[i].name);
                    vm.stackFrame[vm.top_StackFrame].localeSymbol[i].name = NULL;
                }
                if(vm.stackFrame[vm.top_StackFrame].localeSymbol[i].type) {
                    free(vm.stackFrame[vm.top_StackFrame].localeSymbol[i].type);
                    vm.stackFrame[vm.top_StackFrame].localeSymbol[i].type = NULL;
                }
                if(vm.stackFrame[vm.top_StackFrame].localeSymbol[i].address) {
                    free(vm.stackFrame[vm.top_StackFrame].localeSymbol[i].address);
                    vm.stackFrame[vm.top_StackFrame].localeSymbol[i].address = NULL;
                }
            }
            free(vm.stackFrame[vm.top_StackFrame].localeSymbol);
            vm.stackFrame[vm.top_StackFrame].localeSymbol = NULL;
        }
    }
    return;
}
int pushValueIntoStack(StackType* symbol) {
    if(vm.top_stack >= STACK_SIZE_MAX) {
        HXVMError(ERR_STACK_OVERFLOW);
        return -1;
    }
    vm.stack[vm.top_stack].value = symbol->value;
    vm.stack[vm.top_stack].type = symbol->type;
#ifdef SHOW_HX_DEBUG_DETAIL
    printf("\33[33m[DEG]\33[0m表面值(%p)已入栈(%p).\n",symbol->value, &(vm.stack[vm.top_stack]));
#endif
    vm.top_stack++;
    return 0;
}
void popValueOutOfStack(void) {
    if(vm.top_stack > 0) { // 确保栈帧不为空
        vm.top_stack--; // 先将索引移到正确的位置
#ifdef SHOW_HX_DEBUG_DETAIL
        printf("\33[33m[DEG]\33[0m正在弹出表面值(%p)...\n",vm.stack[vm.top_stack].value);
#endif
    }
    return;
}
void HXVMError(HXVMErrorType errType) {
    initLocale();
    switch(errType) {
    case ERR_STACK_OVERFLOW: {
        wprintf(L"\33[31m[E]栈空间不足！\33[0m\n");
    }
    break;

    case ERR_NULL_PTR: {
        wprintf(L"\33[31m[E]无效的内存地址！\33[0m\n");
    }
    break;
    }
    return;
}
#endif