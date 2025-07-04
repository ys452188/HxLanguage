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
    Sentence* sentences;
    Block* blocks;
} Function;
typedef struct ObjectCode {
    Function* functions;
    Sentence* sentences;
    Block* blocks;
} ObjectCode;
int compile(TokenStream*,ObjectCode*);  //编译
int compile(TokenStream* ts,ObjectCode* oc) {
    if(ts->tokens==NULL) {
        return 255;
    }
    for(long int i; i < ts->size; i++) {
        //printf("%ls\n",ts->tokens[i].value);
        switch((ts->tokens[i]).type) {
        case TOKEN_KEYWORD:    //关键字
            if(wcscmp((ts->tokens[i]).value,L"var") == 0) {         //var
                int startIndex = i;
                int endIndex;
                for(long int i_1; i_1 < ts->size; i_1++) {

                    //printf("%ls\n",ts->tokens[i_1].value);

                    if(wcscmp((ts->tokens[i_1]).value,L";") == 0) {
                        endIndex = i_1;
                    }
                }
                if(endIndex==startIndex+1) {
                    fprintf(stderr,"\033[31m[E]关键字\"var\"使用错误！\n(在%d行%d列)\n\033[0m",lexerStatus.lin,lexerStatus.col);
                    return 255;
                }
            } else if(wcscmp((ts->tokens[i]).value,L"con") == 0) {  //con

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
#endif