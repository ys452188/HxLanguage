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
    wchar_t* ret_type;
    Variable* args;
    int argc;
    TokenStream body;
} Function;
// 对象代码结构
typedef struct ObjectCode {
    Function* functions;      // 函数数组
    int function_index;       //  索引
    int function_size;
} ObjectCode;
struct {
    struct {
        Variable* vars;
        int size;
        int index;
    } varSymTable;
    struct {
        Constant* cons;
        int size;
        int index;
    } conSymTable;
} checker_symTable;
int initSymTable(void);
int enterVarSym(Variable variable);
void freeSymTable(void);
int isDuplicateDefineFunction(const Function* user_func, Function* table,int table_length);
int initObjectFunction(ObjectCode* oc);
int initObjectCode(ObjectCode*);
void freeObjectCode(ObjectCode*);
int setArgs(Token* p,int* index,Function* func);
int compile(TokenStream*,ObjectCode*);  //编译
int initObjectCode(ObjectCode* oc) {
    int err = initObjectFunction(oc);
    if(err != 0) return err;
    return 0;
}
void freeObjectCode(ObjectCode* oc) {
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
        func->args[func->argc].name = (wchar_t*)calloc(wcslen(p[*index].value) + 1, sizeof(wchar_t));
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
int isDuplicateDefineFunction(const Function* user_func, Function* table,int table_length) {
    if(table == NULL) return 0;
    if(user_func == NULL) return -1;
    //printf("%ls\n",user_func->name);
    for(int i = 0; i < table_length; i++) {
        if(table[i].name == NULL) continue;
        if((wcscmp(user_func->name,table[i].name) == 0) && (user_func->argc == table[i].argc)) {
            if(user_func->argc == 0) return 1;
            for(int j = 0; j <= user_func->argc; j++) {
                if(wcscmp(user_func->args[j].type,table[i].args[j].type) != 0) {
                    return 0;
                }
            }
            return 1;
        } else {
            continue;
        }
    }
    return 0;
}
int initSymTable(void) {
    checker_symTable.varSymTable.vars = NULL;
    checker_symTable.varSymTable.size = 0;
    checker_symTable.varSymTable.index = 0;
    checker_symTable.varSymTable.vars = (Variable*)calloc(1,sizeof(Variable));
    if(!(checker_symTable.varSymTable.vars)) {
        checker_symTable.conSymTable.cons = NULL;
        return 255;
    }
    checker_symTable.varSymTable.size = 1;
    checker_symTable.conSymTable.cons = NULL;
    checker_symTable.conSymTable.size = 0;
    checker_symTable.conSymTable.index = 0;
    checker_symTable.conSymTable.cons = (Constant*)calloc(1,sizeof(Constant));
    if(!(checker_symTable.conSymTable.cons)) {
        hxFree(&(checker_symTable.varSymTable.vars));
        return 255;
    }
    checker_symTable.conSymTable.size = 1;
    return 0;
}
void freeSymTable(void) {
    printf("freeing varTable\n");
    for(int i = 0; i<checker_symTable.varSymTable.size; i++) {
        hxFree(&(checker_symTable.varSymTable.vars[i].name));
        if(checker_symTable.varSymTable.vars[i].type) {
            hxFree(&(checker_symTable.varSymTable.vars[i].type));
        }
    }
    hxFree(&(checker_symTable.varSymTable.vars));
    printf("freeing conTable\n");
    for(int i = 0; i<checker_symTable.conSymTable.size; i++) {
        hxFree(&(checker_symTable.conSymTable.cons[i].name));
        if(checker_symTable.conSymTable.cons[i].type) {
            hxFree(&(checker_symTable.conSymTable.cons[i].type));
        }
    }
    hxFree(&(checker_symTable.conSymTable.cons));
    return;
}
int enterVarSym(Variable variable) {
    if(checker_symTable.varSymTable.index >= checker_symTable.varSymTable.size) {
        void* temp = realloc(checker_symTable.varSymTable.vars,checker_symTable.varSymTable.size+1);
        if(!(temp)) {
#ifndef _WIN32
            fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
            fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
            return 255;
        }
        checker_symTable.varSymTable.vars = temp;
        checker_symTable.varSymTable.size += 1;
    }
    checker_symTable.varSymTable.vars[checker_symTable.varSymTable.index].name = (wchar_t*)calloc(wcslen(variable.name)+1,sizeof(wchar_t));
    if(!(checker_symTable.varSymTable.vars[checker_symTable.varSymTable.index].name)) {
#ifndef _WIN32
        fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
        fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
        return 255;
    }
    wcscpy(checker_symTable.varSymTable.vars[checker_symTable.varSymTable.index].name, variable.name);
    if(variable.type == NULL) {
        checker_symTable.varSymTable.vars[checker_symTable.varSymTable.index].type = NULL;
    } else {
        checker_symTable.varSymTable.vars[checker_symTable.varSymTable.index].type = (wchar_t*)calloc(wcslen(variable.type)+1,sizeof(wchar_t));
        if(!(checker_symTable.varSymTable.vars[checker_symTable.varSymTable.index].type)) {
#ifndef _WIN32
            fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
            fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
            return 255;
        }
        wcscpy(checker_symTable.varSymTable.vars[checker_symTable.varSymTable.index].type, variable.type);
        printf("已将变量\"%ls\"加入符号表(地址：%p)\n",checker_symTable.varSymTable.vars[checker_symTable.varSymTable.index].name, &(checker_symTable.varSymTable.vars[checker_symTable.varSymTable.index]));
    }
    checker_symTable.varSymTable.index++;
    return 0;
}
int initObjectFunction(ObjectCode* oc) {
    if(oc->functions == NULL) {
        oc->functions = (Function*)calloc(1, sizeof(Function));
    }
    if(oc->functions == NULL) {
#ifndef _WIN32
        fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
        fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
        return 255;
    }
    oc->function_index = 0;
    oc->function_size = 1;
    return 0;
}
#endif