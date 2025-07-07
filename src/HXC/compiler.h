#ifndef COMPILER_H
#define COMPILER_H
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include "lexer.h"
typedef enum {
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
} Opr;
// 对象令牌 - 更安全的联合设计
typedef struct ObjectToken {
    TokenType type;
    union {
        wchar_t* val;         // 值类型使用
        Opr opr;              // 操作符类型（使用枚举值）
    } value;
    bool owns_memory;         // 标记是否需要释放val的内存
} ObjectToken;

// 语句 - 包含令牌数组
typedef struct Sentence {
    ObjectToken* tokens;      // 令牌数组
    int size;
    int count;                // 令牌数,用于goto
} Sentence;

// 代码块 - 更清晰的层次结构
typedef struct Block {
    Sentence* sentences;      // 语句数组
    int sentence_count;       // 语句数量

    struct Block** children;  // 子块数组
    int child_count;          // 索引
} Block;

// 函数 - 包含代码块
typedef struct Function {
    wchar_t* name;            // 函数名
    Block* main_block;        // 主代码块
} Function;

// 对象代码 - 顶层结构
typedef struct ObjectCode {
    Function* functions;      // 函数数组
    int function_count;       //  索引

    Sentence* global_sentences; // 全局语句
    int global_sentence_count;

    Block** global_blocks;      // 全局块
    int global_block_count;
} ObjectCode;
int initObjectCode(ObjectCode*);
void freeObjectCode(ObjectCode*);
int compile(TokenStream*,ObjectCode*);  //编译
void freeBlock(Block*);

int compile(TokenStream* ts,ObjectCode* oc) {
#ifdef _WIN32
    setlocale(LC_ALL,"zh_CN.UTF-8");
#endif
    if(ts->tokens==NULL) {
        return 255;
    }
    if(initObjectCode(oc) == 255) {
#ifndef _WIN32
        fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
        fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
        return 255;
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
                Block newBlock;
                if((ts->tokens[index].type) != TOKEN_INDENTIFIER) {
#ifndef _WIN32
                    fprintf(stderr,"\033[31m[E]关键字使用错误！\n(在%d行%d列)\n\033[0m",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\033[31m[E]关键字使用错误！\n(在%d行%d列)\n\033[0m",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    return 255;
                }
                newFunction.name = (wchar_t*)calloc(1+wcslen(ts->tokens[index].value),sizeof(wchar_t));
                if(!(newFunction.name)) {
#ifndef _WIN32
                    fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
                    fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
                    return 255;
                }
                wcscpy(newFunction.name,ts->tokens[index].value);

                //printf("%ls\n",newFunction.name);

                index++;

                //printf("%ls\n",ts->tokens[index].value);

                if(((ts->tokens[index].type) != TOKEN_OPERATOR) || (wcscmp(ts->tokens[index].value,L"(")!=0 && wcscmp(ts->tokens[index].value,L"（")!=0)) {
#ifndef _WIN32
                    fprintf(stderr,"\33[31m[E]声明函数时,函数名后应为括号\n(在%d行%d列)\n\33[0m",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\33[31m[E]声明函数时,函数名后应为括号\n(在%d行%d列)\n\33[0m",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    free(newFunction.name);
                    return 255;
                }
                /*
                //填表
                void* tmp = realloc(oc->functions,(oc->function_count+3)*sizeof(Function));
                if(!tmp) {
                    #ifndef _WIN32
                    fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
                #else
                    fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
                #endif
                    free(newFunction.name);
                    return 255;
                }
                oc->functions = (Function*)tmp;
                oc->functions[oc->function_count] = newFunction;
                oc->function_count++;
                */
                free(newFunction.name);
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
int initObjectCode(ObjectCode* obj) {
    if(obj == NULL) {
        return 255;
    }
    if(obj->functions == NULL) {
        obj->functions = (Function*)calloc(1,sizeof(Function));
        if(!obj->functions) {
            return 255;
        }
        obj->function_count = 0;
    }
    if(obj->global_sentences == NULL) {
        obj->global_sentences = (Sentence*)calloc(1,sizeof(Sentence));
        if(!obj->global_sentences) {
            return 255;
        }
        obj->global_sentence_count = 0;
    }
    if(obj->global_blocks == NULL) {
        obj->global_blocks = (Block**)calloc(1,sizeof(Block*));
        if(!(obj->global_blocks)) {
            return 255;
        }
        *(obj->global_blocks) = (Block*)calloc(1,sizeof(Block));
        if(!(*(obj->global_blocks))) {
            return 255;
        }
        obj->global_block_count = 0;
    }
    return 0;
}
void freeBlock(Block* block) {
    if(!block) return;
    if(block->sentences) {
        for(int i = 0; i <= block->sentence_count; i++) {
            Sentence* sentence = &(block->sentences[i]);
            if(sentence -> tokens) {
                for(int i1 = 0; i1 < sentence->size; i1++) {
                    if(sentence->tokens[i1].owns_memory) {

                        printf("释放 %p\n",block->sentences[i].tokens[i1].value.val);

                        free(block->sentences[i].tokens[i1].value.val);
                    }
                }

                printf("释放 %p\n", block->sentences[i].tokens);

                free(block->sentences[i].tokens);
            }
        }

        printf("释放 %p\n", block->sentences);

        free(block->sentences);
    }
    if(block->children) {
        for(int i = 0; i<=block->child_count; i++) {
            if(block->children[i]) {

                printf("释放 %p\n",block->children[i]);

                freeBlock(block->children[i]);
            }
        }
        free(block->children);
    }
    free(block);
    return;
}
void freeObjectCode(ObjectCode* oc) {
    if(*(oc->global_blocks)) {
        for(int i=0; i<=oc->global_block_count; i++) {
            freeBlock(oc->global_blocks[i]);
        }

        printf("释放 %p\n",*(oc->global_blocks));

        if(*(oc->global_blocks)) free(*(oc->global_blocks));
        if(oc->global_blocks) free(oc->global_blocks);
    }
    if(oc->functions) {
        for(int i=0; i<=oc->function_count; i++) {
            if(oc->functions[i].name) {

                printf("释放 %p\n",oc->functions[i].name);

                free(oc->functions[i].name);
            }
            if(oc->functions[i].main_block) {
                freeBlock(oc->functions[i].main_block);
            }
        }

        printf("释放 %p\n",oc->functions);

        free(oc->functions);
    }
    if(oc->global_sentences) {
        for(int i = 0; i <= oc->global_sentence_count; i++) {
            if(oc->global_sentences[i].tokens) {
                for(int i1 = 0; i1<oc->global_sentences[i].size; i1++) {
                    if(oc->global_sentences[i].tokens[i1].owns_memory) {
                        free(oc->global_sentences[i].tokens[i1].value.val);
                    }
                }
                free(oc->global_sentences[i].tokens);
            }
        }

        printf("释放 %p\n",oc->global_sentences);

        free(oc->global_sentences);
    }
    return;
}
#endif