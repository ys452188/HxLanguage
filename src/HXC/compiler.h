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
    if(ts->tokens==NULL) {
        return 255;
    }
    if(oc==NULL) {
        oc = (ObjectCode*)malloc(sizeof(ObjectCode));
        if(!oc) {
            fprintf(stderr,"\033[31m[E]内存分配失败！\n\033[0m");
            return 255;
        }
    }
    if(oc->functions==NULL) {
        oc->functions = (Function*)malloc(sizeof(Function));
        if(!oc->functions) {
            fprintf(stderr,"\033[31m[E]内存分配失败！\n\033[0m");
            return 255;
        }
        oc->functionCount = 1;
    }
    if(oc->sentences==NULL) {
        oc->sentences = (Sentence*)malloc(sizeof(Sentence));
        if(!oc->sentences) {
            fprintf(stderr,"\033[31m[E]内存分配失败！\n\033[0m");
            return 255;
        }
        oc->sentenceCount = 1;
    }
    if(oc->blocks==NULL) {
        oc->blocks = (Block*)malloc(sizeof(Block));
        if(!oc->blocks) {
            fprintf(stderr,"\033[31m[E]内存分配失败！\n\033[0m");
            return 255;
        }
        oc->blockCount = 1;
    }
    for(long int i; i < ts->size; i++) {
        //printf("%ls\n",ts->tokens[i].value);
        switch((ts->tokens[i]).type) {
        case TOKEN_KEYWORD:    //关键字
            if((wcscmp((ts->tokens[i]).value,L"var") == 0)||(wcscmp((ts->tokens[i]).value,L"定义变量") == 0)) {         //var
                int startIndex = i;
                int endIndex;
                for(long int i_1; i_1 < ts->size; i_1++) {

                    //printf("%ls\n",ts->tokens[i_1].value);

                    if(wcscmp((ts->tokens[i_1]).value,L";") == 0) {
                        endIndex = i_1;
                    }
                }
                if(endIndex==startIndex+1) {
                    fprintf(stderr,"\033[31m[E]关键字使用错误！\n(在%d行%d列)\n\033[0m",ts->tokens[startIndex].lin,ts->tokens[startIndex].col);
                    return 255;
                }
            } else if((wcscmp((ts->tokens[i]).value,L"con") == 0)||(wcscmp((ts->tokens[i]).value,L"定义常量") == 0)) {  //con

            } else if((wcscmp((ts->tokens[i]).value,L"fun") == 0)||(wcscmp((ts->tokens[i]).value,L"定义函数") == 0)) { //fun
                int index = i;
                index++;
                Function newFunction;
                if((ts->tokens[index].type) != TOKEN_INDENTIFIER) {
                    fprintf(stderr,"\033[31m[E]关键字使用错误！\n(在%d行%d列)\n\033[0m",ts->tokens[index].lin,ts->tokens[index].col);
                    return 255;
                }
                newFunction.name = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(ts->tokens[index].value)+1));
                if(!(newFunction.name)) {
                    fprintf(stderr,"\033[31m[E]内存分配失败！\n\033[0m");
                    return 255;
                }
                wcscpy(newFunction.name,ts->tokens[index].value);
                newFunction.name[wcslen(newFunction.name)] = L'\0';
                //printf("%ls\n",newFunction.name);
                index++;
                //printf("%ls\n",ts->tokens[index].value);
                if(((ts->tokens[index].type) != TOKEN_OPERATOR) || (wcscmp(ts->tokens[index].value,L"(")!=0 && wcscmp(ts->tokens[index].value,L"；")!=0)) {
                    free(newFunction.name);
                    fprintf(stderr,"\33[31m[E]声明函数时,函数名后应为括号\33[0m");
                    return 255;
                }

                oc->functions[oc->functionCount-1] = newFunction;
                oc->functionCount++;
                void* tmp = realloc(oc->functions,oc->functionCount*sizeof(Function));
                if(!tmp) {
                    return 255;
                }
                oc->functions = (Function*)tmp;
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