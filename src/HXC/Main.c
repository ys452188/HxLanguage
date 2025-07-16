//用于处理GNU C Library字符转换子系统（iconv）分配的内部缓冲区
#ifdef __GLIBC__
#include <gnu/libc-version.h>
#include <stdlib.h>
void __libc_freeres(void);
#endif
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include "lexer.h"
#include "scanner.h"
#include "compiler.h"
int hxCompiler(void);
int main(void);
int main(void) {
    clock_t start,end;
    start = clock();
    int err = hxCompiler();
    end = clock();
    printf("\33[33m运行结束,历时 %lf s, 返回码为 %d.\n\33[0m",((double)(end-start)/CLOCKS_PER_SEC),err);
#ifdef __GLIBC__
    __libc_freeres();
#endif
    return err;
}
int hxCompiler(void) {
#ifdef _WIN32
    setlocale(LC_ALL,"zh_CN.UTF-8");
#endif
    printf("\33[32m输入源文件名>>\33[0m");
    char fname[1024];
    scanf("%s",fname);
    wchar_t* src = getData(fname);
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
        freeSymTable();
        //freeObjectCode(&obj);
        return -1;
    }
    freeSymTable();
    //freeObjectCode(&obj);
#ifndef _WIN32
    printf("\33[32m编译完成。\33[0m\n");
#else
    wprintf(L"\33[32m编译完成。\33[0m\n");
#endif
    return 0;
}