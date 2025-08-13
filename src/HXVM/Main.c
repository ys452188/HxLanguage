//版本 1
#pragma pack(1)     //取消结构体对齐
#define HX_DEBUG                     //定义这个宏显示调示信息
#define SHOW_HX_DEBUG_DETAIL         //定义这个宏显示调示细节
#define PUT_STRING_DISPLAY_NULL 1    //定义这个宏设置显示空字符
#include <stdio.h>
#include <time.h>
#include "HXVM.h"
#include "hxLocale.h"
#include "interpreter.h"
int main(int argc, char** argv) {
#ifdef HX_DEBUG
    clock_t start, end;
    start = clock();
#endif
    initLocale();
    if(loadObjectFile("test.hxe")) {
        wprintf(L"\33[31m[E]加载目标文件时发生错误！\33[0m\n");
        exit(EXIT_FAILURE);
    }
    vm.top_StackFrame = 0;
    vm.top_stack = 0;
    if(pushFunIntoStackFrame(&(hsmCode.obj_fun[hsmCode.start_fun])) == -1) {
        exit(EXIT_FAILURE);
    }
    interprete();
    popFunOutOfStackFrame();
    freeObjectCode(&hsmCode);
#ifdef HX_DEBUG
    end = clock();
    wprintf(L"\n\33[33m[DEG]\33[0m解释结束\n共解释%ld条指令, 耗时%lfs(%.3fms)\n", interprete_commands_size,(double)(end - start) / CLOCKS_PER_SEC, (float)(((double)(end - start) / CLOCKS_PER_SEC)*1000));
#endif
    return 0;
}