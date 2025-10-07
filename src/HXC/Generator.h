//优化&目标代码生成
#ifndef HXLANG_GENERATOR_H
#define HXLANG_GENERATOR_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include "Lexer.h"
#include "Pass.h"
#include "Error.h"
typedef struct Symbol {
    int used_time;        //被访问的次数,方便优化。从未被访问过则不生成相关代码
    wchar_t* name;
    wchar_t* type;     //为NULL表示该变量类型尚未明确
    int type_arr_num;
    bool isOnlyRead;
    int def_command_index;  //定义它的北令的索引
} Symbol;
typedef enum OpCode {   //指令码
    ADD,   //加
    SUB,   //减
    MUL,   //乘
    DIV,   //除
    MOVE,   //赋值
    PUSH,   //压栈
    CALL,   //调用函数
} OpCode;
typedef struct Command { //指令
    OpCode op;
} Command;
typedef struct Function {
    int used_time;
    wchar_t* name;
    wchar_t* ret_type;
    int ret_type_arr_num;
    bool isRetTypeKnown;
    Symbol* args;
    int args_size;
    Command* body;
    int body_size;
} Function;
static int parseExpression(Tokens*, int* index,Command** obj_fun_body, int* obj_fun_body_size, int* obj_body_index);   //分析表达式
static int parseVariableDef(Tokens*, int* ir_index, Command** obj_fun_body, int* obj_fun_body_size, int* obj_body_index, Symbol*);
static int gen_function(_Function* ir_fun,Function* obj_fun);
static void freeSymbolTable(Symbol** sym, int sym_size) {
    if(!sym) return;
    for(int i = 0; i<sym_size; i++) {
        if((*sym)[i].name) free((*sym)[i].name);
        (*sym)[i].name = NULL;
        if((*sym)[i].type) free((*sym)[i].type);
        (*sym)[i].type = NULL;
    }
    free(*sym);
    *sym = NULL;
    return;
}
static int gen_function(_Function* ir_fun,Function* obj_fun) {
    if(!ir_fun || !obj_fun) return -1;
    //复制函数名
    obj_fun->name = (wchar_t*)calloc(wcslen(ir_fun->name)+1, sizeof(wchar_t));
    if(!(obj_fun->name)) return -1;
    wcscpy(obj_fun->name, ir_fun->name);
    //复制返回类型
    if(ir_fun->isRetTypeKnown) {
        if(ir_fun->ret_type != NULL) {
            obj_fun->ret_type = (wchar_t*)calloc(wcslen(ir_fun->ret_type)+1, sizeof(wchar_t));
            if(!(obj_fun->ret_type)) return -1;
            wcscpy(obj_fun->ret_type, ir_fun->ret_type);
            obj_fun->ret_type_arr_num = ir_fun->ret_type_arr_num;
        } else {
            obj_fun->ret_type = NULL;
        }
    } else {
        obj_fun->isRetTypeKnown=false;
        obj_fun->ret_type = NULL;
    }
    //复制参数
    if(ir_fun->args == NULL || ir_fun->args_size==0) {
        obj_fun->args = NULL;
        obj_fun->args_size = 0;
    } else {
        obj_fun->args_size = ir_fun->args_size;
        obj_fun->args = (Symbol*)calloc(obj_fun->args_size, sizeof(Symbol));
        if(!(obj_fun->args))return -1;
        for(int i = 0; i < obj_fun->args_size; i++) {
            obj_fun->args[i].name = (wchar_t*)calloc(wcslen(ir_fun->args[i].name)+1, sizeof(wchar_t));
            if(!(obj_fun->args[i].name)) return -1;
            wcscpy(obj_fun->args[i].name, ir_fun->args[i].name);
            obj_fun->args[i].type = (wchar_t*)calloc(wcslen(ir_fun->args[i].type)+1, sizeof(wchar_t));
            if(!(obj_fun->args[i].type)) return -1;
            wcscpy(obj_fun->args[i].type, ir_fun->args[i].type);
            obj_fun->args[i].type_arr_num = ir_fun->args[i].array_num;
        }
    }
    //分析函数体
    if(ir_fun->body == NULL) {
        obj_fun->body = NULL;
    } else {
        Symbol* localeSymTable = NULL;   //局部符号表
        int localeSymTable_size = 0;
        int localeSymTable_index = 0;
        int ir_body_index = 0;
        int obj_body_index = 0;
        while(ir_body_index < ir_fun->body->size) {
            if(wcscmp(ir_fun->body->tokens[ir_body_index].value, L"var")==0||wcscmp(ir_fun->body->tokens[ir_body_index].value, L"con")==0
                    || wcscmp(ir_fun->body->tokens[ir_body_index].value, L"定义变量")==0||wcscmp(ir_fun->body->tokens[ir_body_index].value, L"定义常量")==0) {
                if(localeSymTable) {
                    localeSymTable_size = 1;
                    localeSymTable = (Symbol*)calloc(localeSymTable_size, sizeof(Symbol));
                    if(!localeSymTable) return -1;
                }
                if(localeSymTable_index >= localeSymTable_size) {
                    void* temp = realloc(localeSymTable, (localeSymTable_size+1)*sizeof(Symbol));
                    if(!temp) {
                        freeSymbolTable(&localeSymTable, localeSymTable_size);
                        return -1;
                    }
                    localeSymTable=(Symbol*)temp;
                    memset(&(localeSymTable[localeSymTable_index]), 0, sizeof(Symbol));
                    localeSymTable_size++;
                }
                int parseError=parseVariableDef(ir_fun->body, &ir_body_index, &(obj_fun->body), obj_fun->body_size, &obj_body_index, &(localeSymTable[localeSymTable_index]));
                if(parseError) {
                    freeSymbolTable(&localeSymTable, localeSymTable_size);
                    return parseError;
                }
                localeSymTable_index++;
            }
            ir_body_index++;
        }
    }
}
static int parseVariableDef(Tokens* tokens, int* index, Command** obj_fun_body, int* obj_fun_body_size, int* obj_body_index, Symbol* sym) {
    if(!tokens||!index||!obj_fun_body||!obj_fun_body_size||!obj_body_index) return -1;
    if(wcscmp(tokens->tokens[*index].value, L"con")==0 || wcscmp(tokens->tokens[*index].value, L"定义常量")==0) sym->isOnlyRead = true;
    if(wcscmp(tokens->tokens[*index].value, L"定义常量")==0 || wcscmp(tokens->tokens[*index].value, L"定义变量")==0) {
        if((*index)+1 >= tokens->size) {
            setError(ERR_DEF_VAR, tokens->tokens[*index].line, NULL);
            return 255;
        }
        (*index)++;
        if(wcscmp(tokens->tokens[*index].value, L":")!=0 &&wcscmp(tokens->tokens[*index].value, L"：")!=0) {
            setError(ERR_DEF_VAR, tokens->tokens[*index].line, NULL);
            return 255;
        }
        if((*index)+1 >= tokens->size) {
            setError(ERR_NO_SYM_TYPE, tokens->tokens[*index].line, NULL);
            return 255;
        }
        (*index)++;
        //分析变量名
        if(tokens->tokens[*index].type != TOK_ID) {
            setError(ERR_NO_SYM_TYPE, tokens->tokens[*index].line, NULL);
            return 255;
        }
        sym->name = (wchar_t*)calloc(wcslen(tokens->tokens[*index].value)+1, sizeof(wchar_t));
        if(!(sym->name)) return -1;
        wcscpy(sym->name, tokens->tokens[*index].value);
        //(分析类型|分析右值|结束) & 生成指令
        if((*index)+1 >= tokens->size) {
            setError(ERR_DEF_VAR, tokens->tokens[*index].line, NULL);
            return 255;
        }
        if(tokens->tokens[*index].type==TOK_END) {

        }
    }
}
#endif