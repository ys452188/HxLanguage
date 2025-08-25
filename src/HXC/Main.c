#define HX_DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "output.h"
#include "lexer.h"
#include "scanner.h"
#include "theFirstPass.h"
#include "compiler.h"
#include "writer.h"
/*C语言好习惯：
*变量初始化
*分配的堆内存要释放
*结束后指针置为NULL
*/
int main(int argc, char** argv) {
    if(argc == 1||argv == NULL) {
        initLocale();
        wprintf(L"\33[36m[INFO]HxCompiler(hxc)----HxLanguage的编译器\33[0m\n");
        wprintf(L"\tBy\33[37m硫酸铜非常好吃\33[0m\n");
        wprintf(L"\t版本：\33[37mv1.0\33[0m\n");
        wprintf(L"\tGit：\33[37mhttps://github.com/ys452188/HxLanguage.git\33[0m\n");
        return 0;
    }
    char* fileName = argv[1];
    initLocale();
    //准备
    clock_t start;
    start = clock();
    time_t now_time;
    time(&now_time); // 获取当前时间的秒数
    wprintf(L"\33[36m[INFO]当前时间为：%s\n\33[0m", ctime(&now_time));
    wprintf(L"\33[36m[I]开始编译……\33[0m\n");
    wchar* data = getData(fileName);
    if(data == NULL) {
        exit(EXIT_FAILURE);
    }
    setSrc(data);
    wprintf(L"\33[36m[I]源文件读取完成\33[0m\n");
    free(data);
    data = NULL;
    int lexErr = setTokens();
    freeSrc();
    if(lexErr == -1) {
        wprintf(L"\33[31m[E]内存分配失败！\33[0m\n");
        freeTokens();
        exit(EXIT_FAILURE);
        return -1;
    } else if(lexErr == 255) {
        wprintf(L"\33[31m[E]出现了词法错误, 编译失败！\33[0m\n");
        freeTokens();
        exit(EXIT_FAILURE);
        return 255;
    }
    wprintf(L"\33[36m[I]词法分析完成\33[0m\n");
    int syntaxErr = 0;
    syntaxErr = firstPass();
    freeTokens();
    if(syntaxErr == 255) {
        wprintf(L"\33[31m[E]出现了语法错误, 编译失败1！\33[0m\n");
        freeCheckerOutput();
        exit(EXIT_FAILURE);
        return 255;
    }
    if(syntaxErr == -1) {
        wprintf(L"\33[31m[E]内存分配失败！\33[0m\n");
        freeCheckerOutput();
        exit(EXIT_FAILURE);
        return -1;
    }
    int compileErr = compile(&checkerOutput);
    if(compileErr == 255) {
        wprintf(L"\33[31m[E]出现了语法错误, 编译失败！\33[0m\n");
        freeCheckerOutput();
        exit(EXIT_FAILURE);
        return 255;
    }
    if(compileErr==-1) {
        wprintf(L"\33[31m[E]内存分配失败！\33[0m\n");
        freeCheckerOutput();
        exit(EXIT_FAILURE);
        return -1;
    }
    fflush(stdout);
    wprintf(L"\33[36m[I]编译完成\33[0m\n");
    freeCheckerOutput();
#ifdef HX_DEBUG
    wprintf(L"==========目标代码==========\n");
    wprintf(L"函数数量：%d\n",objCode.obj_fun_size);
    for(int i = 0; i < objCode.obj_fun_size; i++) {
        wprintf(L"函数%d {\n", i);
        wprintf(L"\t函数名：%ls\n", objCode.obj_fun[i].name);
        wprintf(L"\t指令数量：%d\n", objCode.obj_fun[i].body_size);
        wprintf(L"}\n");
        wprintf(L"   -----------------------------------------   \n");
    }
    wprintf(L"============================\n");
#endif
    writeObjectFile("test.hxe");
    wprintf(L"\33[32m[I]已生成目标文件, 本次编译耗时%lfs\33[0m\n",(double)(clock() - start) / CLOCKS_PER_SEC);
    //printf("%ls\n", (wchar*)objCode.obj_fun[0].body[0].op_value[0].value.ptr_val);
    freeObjectCode(&objCode);
    //printf("%zd\n", sizeof(ObjectCode));
    return 0;
}