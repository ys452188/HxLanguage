#ifndef HXVM_H
#define HXVM_H
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <pthread.h>
#define STACK_SPACE_MAX 2048                //局部变量栈空间
#define FUNCTION_CALL_STACK_MAX 128         //函数调用栈空间
typedef union {
    int i;
    float f;
    double lf;
    char c;
    wchar_t wc;
    long int ld;
    void* p;
} StackDataType;
typedef struct {
    StackDataType stack[STACK_SPACE_MAX];
    StackDataType* top;
} Stack;
typedef struct LocalSymbol {       //局部变量及常量
    wchar_t* name;
    wchar_t* type;
    void* address;
} LocalSymbol;
typedef struct StackFrame {
    Stack stack;
    LocalSymbol* localSymbol;
    void* ret_address;
} StackFrame;
typedef struct HXVMStack {
    StackFrame stackFrame[FUNCTION_CALL_STACK_MAX];
    StackFrame* top;
} HXVMStack;
typedef struct Thread {
    pthread_t thread;
    wchar_t* name;
} Thread;
typedef struct HXVM {
    HXVMStack stack;  //虚拟机栈
    Thread* threads;  //线程
    int threadLength;
} HXVM;
void initHXVM(HXVM*);                                 //初始化
void closeHXVM(HXVM*);                                //关闭虚拟机
int pushIntoStack(Stack*, StackDataType);             //入栈
int popStack(Stack*);                                 //出栈
#endif