#ifndef COMPILER_H
#define COMPILER_H
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "lexer.h"
/*此结构体为试验阶段,后期会修改
typedef struct Variable {
    wchar_t* name;
    wchar_t* type;
    void* address;
} Variable;
typedef struct Block {
    wchar_t* data;
} Block;
typedef struct function {
    wchar_t* name;

} Function
typedef struct ObjectCode {
    Block* block;

} ObjectCode;*/
int check(TokenStream*);     //语义检查
void compile(TokenStream*,)  //编译
#endif