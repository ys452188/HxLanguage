#include "vm.h"
int popStack(Stack* stack) {
    if(stack->top == NULL) {
        printf("\33[31m[ERROR]虚拟机栈中无元素！\33[31m\n");
        return 255;
    }
    if(stack->top == &(stack->stack[0])) {
        printf("元素正在出栈......\t地址：%p\n",stack->top);
        stack->top = NULL;
    } else {
        printf("元素正在出栈......\t地址：%p\n",stack->top-1);
        stack->top--;
    }
    return 0;
}
int pushIntoStack(Stack* stack, StackDataType value) {
    if(stack->top == NULL) {
        stack->top = &(stack->stack[0]);
    }
    if (stack->top >= &stack->stack[STACK_SPACE_MAX]) {
        fprintf(stderr,"\33[31m[ERROR]虚拟机栈空间已满！\033[0m\n");
        return 255;
    }
    *(stack->top) = value;
    printf("元素已入栈\t地址：%p\n",stack->top);
    stack->top++;
    return 0;
}
void initHXVM(HXVM* vm) {
    if(vm==NULL) return;
    vm->stack.top = NULL;
    vm->threads = NULL;
    vm->threadLength = 0;
    return;
}
void closeHXVM(HXVM* vm) {
    if(vm == NULL) return;
    vm-> stack.top = NULL;
    //printf("已将虚拟机栈指针设置为NULL.");
    if((vm->threads) != NULL) {             //释放线程
        for(int i = 0; i < vm->threadLength; i++) {
            int err = pthread_attr_destroy(vm->threads[i].thread);
            if(err != 0) {
                fprintf(stderr,"\33[31m[ERROR]虚拟机线程销毁失败！\n(错误码：%d)\33[0m\n",err);
            }
            free(vm->threads[i].name);
            printf("freed %p\n",vm->threads[i].name);
            vm->threads[i].name = NULL;
        }
        free(vm->threads);
    }
}