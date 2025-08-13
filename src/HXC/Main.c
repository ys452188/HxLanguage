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
#ifdef HX_DEBUG
    clock_t start, end;
    start = clock();
#endif
    initLocale();
    wprintf(L"\33[36m[I]开始编译……\33[0m\n");
    wchar* data = getData("test.hxl");
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
    wprintf(L"\33[36m[I]编译完成\33[0m\n");
    freeCheckerOutput();
#ifdef HX_DEBUG
    printf("==========目标代码==========\n");
    printf("函数数量：%d\n",objCode.obj_fun_size);
    for(int i = 0; i < objCode.obj_fun_size; i++) {
        printf("函数%d {\n", i);
        printf("\t函数名：%ls\n", objCode.obj_fun[i].name);
        printf("\t指令数量：%d\n", objCode.obj_fun[i].body_size);
        printf("}\n");
    }
#endif
    writeObjectFile("test.hxe");
    wprintf(L"\33[36m[I]已生成目标文件\33[0m\n");
    //printf("%ls\n", (wchar*)objCode.obj_fun[0].body[0].op_value[0].value.ptr_val);
    freeObjectCode(&objCode);
    //printf("%zd\n", sizeof(ObjectCode));
    wprintf(L"\33[32m[I]编译成功！\33[0m\n");
#ifdef HX_DEBUG
    end = clock();
    printf("\n\33[33m[DEG]\33[0m耗时%lfs\n", (double)(end - start) / CLOCKS_PER_SEC);
#endif
    return 0;
}