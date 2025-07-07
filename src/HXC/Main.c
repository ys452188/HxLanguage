#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include "lexer.h"
#include "scanner.h"
#include "compiler.h"
int main(void) {
#ifdef _WIN32
    setlocale(LC_ALL,"zh_CN.UTF-8");
#endif
    wchar_t* src = getData("main.hxl");
    if(src == NULL) {
        return 255;
    }
    TokenStream t = getToken(src);
    free(src);
    ObjectCode obj = {0};
    int err = compile(&t,&obj);
    cleanupToken(&t);
    if(err) {
#ifndef _WIN32
        printf("\33[31m出现错误,编译失败。\33[0m\n");
#else
        wprintf(L"\33[31m出现错误,编译失败。\33[0m\n");
#endif
        freeObjectCode(&obj);
        return -1;
    }
    printf("\33[32m编译完成。\33[0m\n");
    freeObjectCode(&obj);
    printf("Hello,world!\n");
    return 0;
}