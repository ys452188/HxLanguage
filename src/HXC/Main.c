#define HX_DEBUG
#include <stdio.h>
#include <wchar.h>
#include <time.h>
#define log(msg) fwprintf(logStream, L"\33[33m[DEB]%ls\33[0m\n", msg);
FILE* outputStream = NULL;
FILE* logStream = NULL;
FILE* errorStream = NULL;
#include "Scanner.h"
#include "Lexer.h"
#include "Pass.h"
#include "Generator.h"

int main(int argc, char* argv[]) {
    clock_t start, end;
    start = clock();
    outputStream = stdout;
    logStream = stdout;
    errorStream = stderr;
    //读取源文件
    wchar_t* src = NULL;
    int scannerError = readSourceFile("test.hxl", &src);
    initLocale();
    if(scannerError) {
        fwprintf(outputStream, L"\33[31m[ERR]\33[0m在读取源文件时出错了！\n");
        fwprintf(outputStream, L"\33[31m[ERR]\33[0m编译失败。\n");
        return -1;
    }
    if(wcslen(src) == 0) {
        fwprintf(outputStream, L"\33[31m[ERR]\33[0m你的源文件里空空如也！\n");
        fwprintf(outputStream, L"\33[31m[ERR]\33[0m编译失败。\n");
        return 255;
    }
    //词法分析
    int lexerError = 0;
    fwprintf(outputStream, L"\33[34m[INFO]\33[0m正在进行词法分析\n");
    Tokens* tokens = lex(src, &lexerError);
    if(lexerError == 255) {
        fwprintf(errorStream, L"%ls\n", errorMessageBuffer);
        freeTokens(&tokens);
        fwprintf(outputStream, L"\33[31m[ERR]\33[0m编译失败。\n");
        return 255;
    } else if(lexerError==-1) {
        fwprintf(errorStream, L"\33[31m[ERR]\33[0m内存分配失败！\n");
        freeTokens(&tokens);
        return -1;
    }
    fwprintf(outputStream, L"\33[34m[INFO]\33[0m词法分析完成\n");
#ifdef HX_DEBUG
    showTokens(tokens);
#endif
    free(src);
    src = NULL;
    //词法分析结束
    IR_1* ir_1 = NULL;
    ir_1 = (IR_1*)calloc(1, sizeof(IR_1));
    if(!ir_1) {
        fwprintf(errorStream, L"\33[31m[ERR]\33[0m内存分配失败！\n");
        freeTokens(&tokens);
        return -1;
    }
    int passError = 0;
    fwprintf(outputStream, L"\33[34m[INFO]\33[0m正在进行第一次遍历\n");
    passError = pass(tokens, ir_1);
    if(passError == -1) {
        fwprintf(errorStream, L"\33[31m[ERR]\33[0m内存分配失败！\n");
        freeTokens(&tokens);
        freeIR_1(&ir_1);
        return -1;
    }
    if(passError == 255) {
        fwprintf(errorStream, L"%ls\n", errorMessageBuffer);
        fwprintf(outputStream, L"\33[31m[ERR]\33[0m编译失败。\n");
        freeTokens(&tokens);
        freeIR_1(&ir_1);
        return 255;
    }
    fwprintf(outputStream, L"\33[34m[INFO]\33[0m第一次遍历完成\n");
    freeTokens(&tokens);

    freeIR_1(&ir_1);
    end = clock();
    fwprintf(outputStream, L"\33[34m[INFO]\33[0m编译完成。共耗时%lfs\n", (double)(end - start) / CLOCKS_PER_SEC);
    return 0;
}