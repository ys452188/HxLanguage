#include <stdio.h>
#include <stdlib.h>
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
    initLocale();
    wchar* data = getData("test.hxl");
    if(data == NULL) {
        exit(EXIT_FAILURE);
    }
    setSrc(data);
    free(data);
    data = NULL;
    int lexErr = setTokens();
    freeSrc();
    if(lexErr == -1) {
        fwprintf(stderr, L"\33[31m[E]内存分配失败！\33[0m\n");
        freeTokens();
        exit(EXIT_FAILURE);
        return -1;
    } else if(lexErr == 255) {
        fwprintf(stderr, L"\33[31m[E]出现了词法错误, 编译失败！\33[0m\n");
        freeTokens();
        exit(EXIT_FAILURE);
        return 255;
    }
    int syntaxErr = 0;
    syntaxErr = firstPass();
    freeTokens();
    if(syntaxErr == 255) {
        fwprintf(stderr, L"\33[31m[E]出现了语法错误, 编译失败1！\33[0m\n");
        freeCheckerOutput();
        exit(EXIT_FAILURE);
        return 255;
    }
    if(syntaxErr == -1) {
        fwprintf(stderr, L"\33[31m[E]内存分配失败！\33[0m\n");
        freeCheckerOutput();
        exit(EXIT_FAILURE);
        return -1;
    }
    int compileErr = compile(&checkerOutput);
    if(compileErr == 255) {
        fwprintf(stderr, L"\33[31m[E]出现了语法错误, 编译失败！\33[0m\n");
        freeCheckerOutput();
        exit(EXIT_FAILURE);
        return 255;
    }
    if(compileErr==-1) {
        fwprintf(stderr, L"\33[31m[E]内存分配失败！\33[0m\n");
        freeCheckerOutput();
        exit(EXIT_FAILURE);
        return -1;
    }
    freeCheckerOutput();
    writeObjectFile("test.hxe");
    //printf("%ls\n", (wchar*)objCode.obj_fun[0].body[0].op_value[0].value.ptr_val);
    freeObjectCode(&objCode);
    //printf("%zd\n", sizeof(ObjectCode));
    fwprintf(stdout,L"\33[32m[I]编译成功！\33[0m\n");
    return 0;
}