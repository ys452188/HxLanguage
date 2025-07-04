#ifndef COMPILER_H
#define COMPILER_H
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "lexer.h"
typedef struct ObjectToken {
    union {
        wchar_t* val;
        enum {
            TOK_ADD,    //加
            TOK_MIN,    //减
            TOK_EQU,    //等
            TOK_DIV,    //除
            TOK_MUL,    //乘
            TOK_GRE,    //大于
            TOK_LES,    //小于
            TOK_LOE,    //小于或等于
            TOK_GOE,    //大于或等于
            TOK_SEI,    //自增
            TOK_SER,    //自减
            TOK_ASG,    //赋值
            TOK_CAL,    //调用
            TOK_VAR,    //定义变量
            TOK_CON,    //定义常量
        } opr;
    } value;
    TokenType type;
} ObjectToken;
typedef struct Sentence {
    ObjectToken* tokens;
    int count;           //计数,用于goto
} Sentence;
typedef struct Block {
    Sentence* sentences;
    int count;
    struct Block* blocks;
} Block;
typedef struct Function {
    wchar_t* name;
    Sentence* sentences;
    Block* blocks;
} Function;
typedef struct ObjectCode {
    Function* functions;
    int functionCount;
    Sentence* sentences;
    int sentenceCount;
    Block* blocks;
    int blockCount;
} ObjectCode;
void freeObjectCode(ObjectCode*);
int compile(TokenStream*,ObjectCode*);  //编译
int compile(TokenStream* ts,ObjectCode* oc) {
#ifdef _WIN32
#include <locale.h>
    setlocale(LC_ALL,"zh_CN.UTF-8");
#endif
    if(ts->tokens==NULL) {
        return 255;
    }
    if(oc==NULL) {
        oc = (ObjectCode*)malloc(sizeof(ObjectCode));
        if(!oc) {
#ifndef _WIN32
            fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
            fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
            return 255;
        }
    }
    if(oc->functions==NULL) {
        oc->functions = (Function*)malloc(sizeof(Function)*2);
        if(!oc->functions) {
#ifndef _WIN32
            fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
            fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
            return 255;
        }
        oc->functionCount = 0;
    }
    if(oc->sentences==NULL) {
        oc->sentences = (Sentence*)malloc(sizeof(Sentence)*2);
        if(!oc->sentences) {
#ifndef _WIN32
            fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
            fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
            return 255;
        }
        oc->sentenceCount = 0;
    }
    if(oc->blocks==NULL) {
        oc->blocks = (Block*)malloc(sizeof(Block)*2);
        if(!oc->blocks) {
#ifndef _WIN32
            fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
            fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
            return 255;
        }
        oc->blockCount = 0;
    }
    for(long int i = 0; i < ts->size; i++) {

        //printf("%ls\n",ts->tokens[i].value);

        switch((ts->tokens[i]).type) {
        case TOKEN_KEYWORD:    //关键字
            if((wcscmp((ts->tokens[i]).value,L"var") == 0)||(wcscmp((ts->tokens[i]).value,L"定义变量") == 0)) {         //var
                int startIndex = i;
                int endIndex = 0;
                for(endIndex = startIndex; endIndex < ts->size; endIndex++) {

                    //printf("%ls\n",ts->tokens[endIndex].value);

                    if(wcscmp((ts->tokens[endIndex]).value,L";") == 0 || wcscmp((ts->tokens[endIndex]).value,L"；") == 0) {
                        break;
                    }
                }
                if(endIndex==startIndex+1) {
#ifndef _WIN32
                    fprintf(stderr,"\033[31m[E]关键字使用错误！\n(在%d行%d列)\n\033[0m",ts->tokens[startIndex].lin,ts->tokens[startIndex].col);
#else
                    fwprintf(stderr,L"\033[31m[E]关键字使用错误！\n(在%d行%d列)\n\033[0m",ts->tokens[startIndex].lin,ts->tokens[startIndex].col);
#endif
                    return 255;
                }
            } else if((wcscmp((ts->tokens[i]).value,L"con") == 0)||(wcscmp((ts->tokens[i]).value,L"定义常量") == 0)) {  //con

            } else if((wcscmp((ts->tokens[i]).value,L"fun") == 0)||(wcscmp((ts->tokens[i]).value,L"定义函数") == 0)) { //fun
                int index = i;
                index++;
                Function newFunction;
                if((ts->tokens[index].type) != TOKEN_INDENTIFIER) {
#ifndef _WIN32
                    fprintf(stderr,"\033[31m[E]关键字使用错误！\n(在%d行%d列)\n\033[0m",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\033[31m[E]关键字使用错误！\n(在%d行%d列)\n\033[0m",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    return 255;
                }
                newFunction.name = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(ts->tokens[index].value)+1));
                if(!(newFunction.name)) {
#ifndef _WIN32
                    fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
                    fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
                    return 255;
                }
                wcscpy(newFunction.name,ts->tokens[index].value);

                //printf("\n%ls\n",newFunction.name);

                index++;

                //printf("%ls\n",ts->tokens[index].value);

                if(((ts->tokens[index].type) != TOKEN_OPERATOR) || (wcscmp(ts->tokens[index].value,L"(")!=0 && wcscmp(ts->tokens[index].value,L"（")!=0)) {
                    free(newFunction.name);
#ifndef _WIN32
                    fprintf(stderr,"\33[31m[E]声明函数时,函数名后应为括号\n(在%d行%d列)\n\33[0m",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\33[31m[E]声明函数时,函数名后应为括号\n(在%d行%d列)\n\33[0m",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    return 255;
                }
                void* tmp = realloc(oc->functions,(oc->functionCount+3)*sizeof(Function));
                if(!tmp) {
                    free(newFunction.name);
                    return 255;
                }
                oc->functions = (Function*)tmp;
                oc->functions[oc->functionCount] = newFunction;
                oc->functionCount++;
            }
            break;
        case TOKEN_VALUE:      //字面量
            break;
        case TOKEN_INDENTIFIER://标识符
            break;
        case TOKEN_OPERATOR:   //运算符
            break;
        }
    }
    return 0;
}
void freeObjectCode(ObjectCode* oc) {
    if(oc==NULL) return;
    for(int i=0; i<(oc->functionCount); i++) {
        free(oc->functions[i].name);
        free(oc->functions[i].sentences);
        free(oc->functions[i].blocks);
    }
    free(oc->functions);
    free(oc->sentences);
    free(oc->blocks);
    return;
}
#endif