#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <pthread.h>
#define STACK_SPACE_MAX 2048                //栈空间
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
typedef struct {
    pthread_t thread;
    wchar_t* name;
} Thread;
typedef struct HXVM {
    Stack stack;      //虚拟机栈
    Thread* threads;  //线程
} HXVM;
void initHXVM(HXVM*);                                 //初始化
void closeHXVM(HXVM*);                                //关闭虚拟机
int pushIntoStack(Stack*, StackDataType);             //入栈
int main(int argc, char** argv) {
    HXVM vm;
    initHXVM(&vm);
    return 0;
}
int pushIntoStack(Stack* stack, StackDataType value) {
    if (stack->top >= &stack->stack[STACK_SPACE_MAX]) {
        fprintf(stderr,"\33[31m[ERROR]虚拟机栈空间已满！\033[0m\n");
        return 255;
    }
    *(stack->top) = value;
    stack->top++;
    return 0;
}
void initHXVM(HXVM* vm) {
    if(vm==NULL) return;
    vm-> stack.top = &(vm-> stack.stack[0]);
    return;
}
void closeHXVM(HXVM* vm) {
    if(vm == NULL) return;
    vm-> stack.top = NULL;
    while((vm->threads) != NULL) {             //释放线程
        int err = pthread_attr_destroy(vm->threads->thread);
        if(err != 0) {
            fprintf(stderr,"\33[31m[ERROR]虚拟机线程销毁失败！\n(错误码：%d)\33[0m\n",err);
        }
        free(vm->threads->name);
    }
}