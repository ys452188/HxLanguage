//版本 1
#pragma pack(1)     //取消结构体对齐
#define HX_DEBUG   //定义这个宏显示调示信息
#include <stdio.h>
#include "HXVM.h"
#include "hxLocale.h"
#include "interpreter.h"
int main(int argc, char** argv) {
    initLocale();
    if(loadObjectFile("test.hxe")) {
        wprintf(L"\33[31m[E]加载目标文件时发生错误！\33[0m\n");
    }
    vm.top_StackFrame = 0;
    if(pushFunIntoStackFrame(&(hsmCode.obj_fun[hsmCode.start_fun])) == -1) {
        exit(EXIT_FAILURE);
    }
    interprete();
    popFunOutOfStackFrame();
    freeObjectCode(&hsmCode);
#ifdef HX_DEBUG
    printf("\n\33[33m[DEG]\33[0m解释结束\n");
#endif
    return 0;
}