#ifndef OBJECT_CODE_H
#define OBJECT_CODE_H
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <locale.h>
#include "lexer.h"
// 释放指针ptr指向的内存空间
void hxFree(void** ptr) {
    // 如果ptr为空，则直接返回
    if(*ptr == NULL) return;
    //调试输出
    printf("freeing %p\n",*ptr);
    // 释放ptr指向的内存空间
    free(*ptr);
    *ptr = NULL;
    return;
}
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
    int count;
    Sentence* sentences;      // 语句数组
    int sentence_count;       // 语句数量

    struct Block** children;  // 子块数组
    int child_count;          // 索引
} Block;
typedef struct Variable {
    wchar_t* name;
    wchar_t* type;
} Variable;
typedef struct Constant {
    wchar_t* name;
    wchar_t* type;
} Constant;
// 函数
typedef struct Function {
    wchar_t* name;            // 函数名
    Variable* args;
    int argc;
    Block* main_block;        // 主代码块
} Function;
// 对象代码结构
typedef struct ObjectCode {
    Function* functions;      // 函数数组
    int function_count;       //  索引

    Sentence* global_sentences; // 全局语句
    int global_sentence_count;

    Block** global_blocks;      // 全局块
    int global_block_count;
} ObjectCode;
typedef struct VarSymTable {
    wchar_t* name;
    wchar_t* type;
} VarSymTable;
int initObjectCode(ObjectCode*);
void freeObjectCode(ObjectCode*);
int setArgs(Token* p,int* index,Function* func);
int compile(TokenStream*,ObjectCode*);  //编译
void freeBlock(Block*);
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
                        printf("1释放 %p\n",block->sentences[i].tokens[i1].value.val);
                        hxFree(&(block->sentences[i].tokens[i1].value.val));
                    }
                }
                printf("2释放 %p\n", block->sentences[i].tokens);
                hxFree(&(block->sentences[i].tokens));
            }
        }
        printf("3释放 %p\n", block->sentences);
        hxFree(&(block->sentences));
    }
    if(block->children) {
        for(int i = 0; i<=block->child_count; i++) {
            if(block->children[i]) {
                printf("4释放 %p\n",block->children[i]);
                freeBlock(block->children[i]);
            }
        }
        printf("释放");
        hxFree(&(block->children));
    }
    printf("释放");
    hxFree(&(block));
    return;
}
void freeObjectCode(ObjectCode* oc) {
    if(*(oc->global_blocks)) {
        for(int i=0; i<=oc->global_block_count; i++) {
            freeBlock(oc->global_blocks[i]);
        }
        if(oc->global_blocks) hxFree(&(oc->global_blocks));
    }
    if(oc->functions) {
        printf("7释放 %p\n",oc->functions);
        hxFree(&(oc->functions));
    }
    if(oc->global_sentences) {
        for(int i = 0; i <= oc->global_sentence_count; i++) {
            if(oc->global_sentences[i].tokens) {
                for(int i1 = 0; i1<oc->global_sentences[i].size; i1++) {
                    if(oc->global_sentences[i].tokens[i1].owns_memory) {
                        hxFree(&(oc->global_sentences[i].tokens[i1].value.val));
                    }
                }
                hxFree(&(oc->global_sentences[i].tokens));
            }
        }
        printf("8释放 %p\n",oc->global_sentences);
        hxFree(&(oc->global_sentences));
    }
    return;
}
int setArgs(Token* p,int* index,Function* func) {
    if(wcscmp(p[*index].value,L")")==0 || wcscmp(p[*index].value,L"）")==0) {
        return 0;
    }
    Token* ptr = p;
    if(p[*index].type != TOKEN_INDENTIFIER) {
        return 255;
    } else {
        //printf("%ls\n",p[*index].value);
        void* temp = realloc(func->args,sizeof(Variable)*(func->argc+1));
        if(!temp) {
#ifndef _WIN32
            fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
            fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
            return 255;
        }
        func->args = temp;
        func->args[func->argc].name = (wchar_t*)calloc(wcslen(p->value)+1,sizeof(wchar_t));
        if(!(func->args[func->argc].name)) {
#ifndef _WIN32
            fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
            fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
            return 255;
        }
        //printf("%ls\n",p[*index].value);
        wcscpy((func->args[func->argc]).name,p[*index].value);
        //printf("%ls\n",func->args[func->argc].name);
        (*index)++;
        //printf("%d\n",*index);
        //printf("%ls\n",p[*index].value);
        if(wcscmp(p[*index].value,L":") != 0 && wcscmp(p[*index].value,L"：") != 0) {
#ifndef _WIN32
            fprintf(stderr,"\033[31m[E]每一个参数的参数名后必须有冒号和该参数的类型！(%d行,%d列)\n\033[0m\n",p[*index].lin,p[*index].col);
#else
            fwprintf(stderr,L"\033[31m[E]每一个参数的参数名后必须有冒号和该参数的类型！(%d行,%d列)\n\033[0m\n",p[*index].lin,p[*index].col);
#endif
            return 255;
        }
        (*index)++;
        //printf("%d\n",p[*index].type);
        if((p[*index].type != TOKEN_INDENTIFIER) && (p[*index].type != TOKEN_KEYWORD)) {
#ifndef _WIN32
            fprintf(stderr,"\033[31m[E]参数的类型名必须是一个标识符或关键字！\n(%d行,%d列)\033[0m\n",p[*index].lin,p[*index].col);
#else
            fwprintf(stderr,L"\033[31m[E]参数的类型名必须是一个标识符或关键字！\n(%d行,%d列)\033[0m\n",p[*index].lin,p[*index].col);
#endif
            return 255;
        }
        func->args[func->argc].type = (wchar_t*)calloc(wcslen(p[*index].value)+1,sizeof(wchar_t));
        if(!(func->args[func->argc].type)) {
#ifndef _WIN32
            fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
            fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
            return 255;
        }
        wcscpy(func->args[func->argc].type,p[*index].value);

        //printf("arg%d { type = %ls, name  = %ls }\n",func->argc,func->args[func->argc].type,func->args[func->argc].name);

        func->argc++;
        (*index)++;
        if(wcscmp(p[*index].value,L",")==0 && ((wcscmp(p[*index+1].value,L")")!=0) || (wcscmp(p[*index+1].value,L"）")==0))) {
            (*index)++;
            return setArgs(ptr,index,func);
        } else if(wcscmp(p[*index].value,L")") == 0 || wcscmp(p[*index].value,L"）") == 0) {
            return 0;
        } else {
#ifndef _WIN32
            fprintf(stderr,"\033[31m[E]参数错误！\n(%d行,%d列)\033[0m\n",p[*index].lin,p[*index].col);
#else
            fwprintf(stderr,L"\033[31m[E]参数错误！\n(%d行,%d列)\033[0m\n",p[*index].lin,p[*index].col);
#endif
            return 255;
        }
        return 0;
    }
    return 255;
}
#endif