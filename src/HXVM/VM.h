#ifndef HXVM_VM_H
#define HXVM_VM_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
typedef struct OpStack {
  enum {
    INT,
    CHAR,
    STRING,
    FLOAT,
    DOUBLE,
    VOID_POINTER,  // void*指针,指向堆内存中的对象
  } type;
  union {
    int32_t int_value;
    wchar_t char_value;
    wchar_t* string;
    float float_value;
    double double_value;
    void* void_pointer;
  } value;
} OpStackType;
typedef struct StackFrame {
  OpStackType opStack[OP_STACK_SIZE];  // 操作数栈
  byte stack[STACK_SIZE];
  int commandSize;  // 指令数量
} StackFrame;
typedef struct Thread {
  StackFrame frames[STACK_FRAME_SIZE];
} Thread;
typedef struct VM {
  // 线程
  Thread* threads;
  int threadsSize;
  // 分配的堆内存,最后要释放
  void** heapMemory;
  int heapMemory_size;
} VM;
extern int startVM(VM* vm) {
  if (vm == NULL) return -1;
  if (vm->threads == NULL || vm->threadsSize == 0) {
    fwprintf(errorStream, L"\33[31m[E]\33[0m没有可用的线程！\n");
    return -1;
  }
  return 0;
}
#endif