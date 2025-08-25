#ifndef HXLANG_COMPILER_H
#define HXLANG_COMPILER_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>
#include "lexer.h"
#include "output.h"
#include "theFirstPass.h"
#pragma pack(1)     //取消结构体对齐

typedef uint32_t i32;
typedef enum CompileErrorType {
    ERR_NO_FUN_MAIN,                 //缺少main函数
    ERR_RELOAD_MAIN,                 //重载main函数
    ERR_MAIN_ARGS,                   //main函数参数错误
    ERR_MAIN_RET_TYPE,               //main函数返回值错误
    ERR_CALL_FUN,                    //调用函数的语法错误
    ERR_QUITE_NOT_CORRECTLY_CLOSE,   //括号未正确闭合
    ERR_FUN_ARGS_SIZE,               //传参个数错误
    ERR_FUN_ARGS_TYPE,               //参数类型不匹配
    ERR_VAR_REPEAT_DEFINE,           //变量重复定义
    ERR_SYM_NOT_DEFINED,             //变量未定义
    ERR_EXP,                         //错误的表达式
    ERR_VAR_DEFINITION,              //定义变量的语法错误
    ERR_CON_CAN_NOT_MOVE,            //对常量赋值"
    ERR_FUN_NOT_DEFINED,             //函数未定义
} CompileErrorType;
typedef enum OPCode {   //操作码
    OP_PUT_STR,         //从栈中输出wchar_t*
    OP_DEFINE_VAR,      //定义变量,第一个操作数为变量名,第二个为类型
    OP_MOVE,            //赋值
    OP_PUSH,
    OP_CALL,
    OP_ADD,
    OP_SUB,   //减
    OP_MUL,
    OP_DIV,   //除
} OPCode;
typedef struct ObjValue {
    void* value;
    int size;
    enum {
        TYPE_SYM,
        TYPE_STR,
        TYPE_DOUBLE,
        TYPE_FLOAT,
        TYPE_INT,
        TYPE_CH,
    } type;
} ObjValue;
typedef struct Command {
    OPCode op;
    ObjValue* op_value;
    i32 op_value_size;
} Command;
typedef struct ClassMember {
    wchar* name;
    wchar* type;
    bool isOnlyRead;
    int offest;   //偏移量
} ClassMember;

typedef struct ObjSymbol {
    wchar* name;
    wchar* type;
    bool isOnlyRead;
} ObjSymbol;
typedef struct ObjFunction {
    wchar* name;
    wchar* ret_type;
    ObjSymbol* args;
    i32 args_size;
    Command* body;
    i32 body_size;
} ObjFunction;
typedef struct ObjClass {
    wchar* name;
    ClassMember* pub_sym;
    int pub_sym_size;
    ClassMember* pri_sym;
    int pri_sym_size;
    ClassMember* pro_sym;
    int pro_sym_size;

    ObjFunction* pub_fun;
    int pub_fun_size;
    ObjFunction* pri_fun;
    int pri_fun_size;
    ObjFunction* pro_fun;
    int pro_fun_suze;
} ObjClass;
typedef struct ObjectCode {
    i32 start_fun;    //入口点,main函数的索引
    ObjFunction* obj_fun;
    i32 obj_fun_size;

    ObjSymbol* obj_sym;
    i32 obj_sym_size;

    ObjClass* obj_class;
    i32 obj_class_size;
} ObjectCode;

CheckerFunction* mainPtr = NULL;   //指向main函数的指针
extern ObjectCode objCode;
typedef union ResultType {
    enum {
        RESULT_TYPE_STR = 1,
        RESULT_TYPE_DOUBLE,
        RESULT_TYPE_FLOAT,
        RESULT_TYPE_INT,
        RESULT_TYPE_CH,
        UNKNOWN,
    } type;
    wchar* type_unknown;
} ResultType;

void free_local_symbol_table(CheckerSymbol* table, int size);   // 释放局部的 CheckerSymbol 数组
void stringEscape(wchar* str);    //字符串转义
bool findSymbol(wchar* name, CheckerSymbol** table, int table_size, CheckerSymbol** ptr);   //从table查找符号,ptr将指向该符号
ResultType parseType(wchar* str);          //通过表面值字符串判断表面值的类型
ResultType parseTypeByTypeStr(wchar* str);  //通过类型名判断类型
int allocOpValueByType(const ResultType* type, void** ptr);
int setValueByType(const ResultType* type, const wchar* str, void** ptr);
void freeObjCode(void);
void compileError(CompileErrorType type, int errLine);
int ckeckMainFunction(CheckerOutput* IR);                    //检查并设置入口点
int parseEXP(Command** cmd,int* cmd_index,int* cmd_size,Token* exp, int exp_size, ResultType* result_type, CheckerSymbol** table, int table_size); //分析表达式
int compile(CheckerOutput*);
#include "parseEXP.h"

ResultType parseTypeByTypeStr(wchar* str) {
    ResultType type = {
        .type=UNKNOWN,
        .type_unknown = NULL,
    };
    if(str == NULL) {
        return type;
    }
    if(wcsequ(str, L"int")||wcsequ(str, L"整型")) {
        type.type = RESULT_TYPE_INT;
    } else if(wcsequ(str, L"float")||wcsequ(str, L"浮点型")) {
        type.type = RESULT_TYPE_FLOAT;
    } else if(wcsequ(str, L"double")||wcsequ(str, L"精确浮点型")) {
        type.type = RESULT_TYPE_DOUBLE;
    } else if(wcsequ(str, L"char")||wcsequ(str, L"字符型")) {
        type.type = RESULT_TYPE_CH;
    } else if(wcsequ(str, L"str")||wcsequ(str, L"字符串型")) {
        type.type = RESULT_TYPE_STR;
    } else {
        type.type_unknown = str;
    }
    return type;
}

ObjectCode objCode = {   //目标代码
    .start_fun = 0,
    .obj_fun = NULL,
    .obj_sym_size = 0,
    .obj_sym = NULL,
    .obj_fun_size = 0,
};
int obj_fun_index = 0;
int compile_fun(CheckerFunction* func) {   //生成该函数的目标代码并加入至objCode.obj_fun  返回-1表内存错误,1表该函数无意义,255表语法错误
    if(func==NULL) return -1;
    if(func->name==NULL) return -1;
    if(func->body==NULL||func->body_size==0) return 1;  //废物函数
    if(objCode.obj_fun==NULL) {
        objCode.obj_fun=(ObjFunction*)calloc(1, sizeof(ObjFunction));
        if(!(objCode.obj_fun)) return -1;
        objCode.obj_fun_size = 1;
    }
    if(objCode.obj_fun_size <= obj_fun_index) {
        objCode.obj_fun_size = obj_fun_index+1;
        void* temp = realloc(objCode.obj_fun, (objCode.obj_fun_size)*sizeof(ObjFunction));
        if(!temp) return -1;
        objCode.obj_fun = (ObjFunction*)temp;
        memset(&(objCode.obj_fun[obj_fun_index]), 0, sizeof(ObjFunction));
    }
    //复制函数名
    objCode.obj_fun[obj_fun_index].name = (wchar*)calloc(wcslen(func->name)+1, sizeof(wchar));
    if(!(objCode.obj_fun[obj_fun_index].name)) return -1;
    wcscpy(objCode.obj_fun[obj_fun_index].name, func->name);
    //复制参数
    if(func->args==NULL||func->args_size==NULL) {
        objCode.obj_fun[obj_fun_index].args = NULL;
        objCode.obj_fun[obj_fun_index].args_size = 0;
    } else {



    }
    //复制返回值
    if(func->ret_type == NULL) {
        objCode.obj_fun[obj_fun_index].ret_type = NULL;
    } else {



    }
    CheckerSymbol* local_sym = NULL;
    int local_sym_size = 0;
    int sym_index = 0;
    objCode.obj_fun[obj_fun_index].body = (Command*)calloc(1, sizeof(Command));
    if(!(objCode.obj_fun[obj_fun_index].body)) return -1;
    objCode.obj_fun[obj_fun_index].body_size = 1;
    int cmd_index = 0;
    //分析函数体
    for(int i = 0; i < func->body_size; i++) {
        switch(func->body[i].type) {
        case TOK_ID: {
            if(wcsequ(func->body[i].value, L"putString") || wcsequ(func->body[i].value, L"输出字符串")) {
                if(i+1 >= func->body_size) {
                    compileError(ERR_CALL_FUN, func->body[i].lin);
                    return 255;
                }
                i++;
                if((!wcsequ(func->body[i].value, L"("))&&(!wcsequ(func->body[i].value, L"（"))) {
                    compileError(ERR_CALL_FUN, func->body[i].lin);
                    return 255;
                }
                if(i+1 >= func->body_size) {
                    compileError(ERR_CALL_FUN, func->body[i].lin);
                    return 255;
                }
                i++;
                int exp_start = i;
                int count = 0;
                int open = 1;
                int close = 0;
                while(i < func->body_size-1) {
                    if(wcsequ(func->body[i].value, L"(")||wcsequ(func->body[i].value, L"（")) {
                        open++;
                    }
                    if(wcsequ(func->body[i].value, L")")||wcsequ(func->body[i].value, L"）")) {
                        close++;
                    }
                    if(open == close) break;
                    count++;
                    i++;
                }
                if(open!=close) {
                    compileError(ERR_QUITE_NOT_CORRECTLY_CLOSE, func->body[i].lin);
                    return 255;
                }
                int exp_end = i;
                if(exp_end-exp_start==0) {
                    compileError(ERR_FUN_ARGS_SIZE,func->body[i].lin);
                    return 255;
                }
                //分析参数
                ResultType result_type = {0};
                int EXPErr = parseEXP(&(objCode.obj_fun[obj_fun_index].body), &cmd_index, &(objCode.obj_fun[obj_fun_index].body_size), &(func->body[exp_start]), exp_end-exp_start, &result_type, &local_sym, local_sym_size);
                if(EXPErr) return EXPErr;

                if(result_type.type != RESULT_TYPE_STR||result_type.type==UNKNOWN) {
                    compileError(ERR_FUN_ARGS_TYPE, func->body[i].lin);
                    return 255;
                }
                //生成指令OP_PUT_STR
                if(cmd_index >= objCode.obj_fun[obj_fun_index].body_size) {
                    objCode.obj_fun[obj_fun_index].body_size = cmd_index+1;
                    void* temp = realloc(objCode.obj_fun[obj_fun_index].body, sizeof(Command)*(objCode.obj_fun[obj_fun_index].body_size));
                    if(!temp) return -1;
                    objCode.obj_fun[obj_fun_index].body = (Command*)temp;
                    memset(&objCode.obj_fun[obj_fun_index].body[cmd_index], 0, sizeof(Command));
                }
                objCode.obj_fun[obj_fun_index].body[cmd_index].op = OP_PUT_STR;
#ifdef HX_DEBUG
                printf("生成指令[%d]：OP_PUT_STR \n", cmd_index);
#endif
                if(objCode.obj_fun[obj_fun_index].body_size <= cmd_index) {
                    objCode.obj_fun[obj_fun_index].body_size = cmd_index+1;
                    void* temp = realloc(objCode.obj_fun[obj_fun_index].body, objCode.obj_fun[obj_fun_index].body_size*sizeof(Command));
                    if(!temp) return -1;
                    objCode.obj_fun[obj_fun_index].body = (Command*)temp;
                }

                if(!(i+1 < func->body_size)) {
                    error(ERR_NO_END, func->body[i].lin);
                    return 255;
                }
                i++;
#ifdef HX_DEBUG
                //printf("%ls\n", mainPtr->body[i].value);
#endif

                if(!wcsequ(func->body[i].value, L";")&&!wcsequ(func->body[i].value, L"；")) {
                    error(ERR_NO_END, func->body[i].lin);
                    return 255;
                }
                cmd_index++;
            } else {     //调用函数或赋值
                if(!(i+1 < func->body_size)) {     //ID (NULL)
                    error(ERR_NO_END, func->body[i].lin);
                    return 255;
                }
                wchar* symName = func->body[i].value;      //符号名
                i++;
                if(wcsequ(func->body[i].value, L";")||wcsequ(func->body[i].value, L"；")) {    //ID ;|；
                    //废话
                    break;
                } else if(wcsequ(func->body[i].value, L"=")) {   //赋值
                    CheckerSymbol* symPtr = NULL;
                    //检查符号是否定义
                    if(!findSymbol(symName, &local_sym, local_sym_size, &symPtr)) {
                        compileError(ERR_SYM_NOT_DEFINED, func->body[i-1].lin);
                        return 255;
                    }
                    //检查是否为常量"
                    if((symPtr->isOnlyRead)&&(symPtr->isInited)) {
                        compileError(ERR_CON_CAN_NOT_MOVE, func->body[i-1].lin);
                        return 255;
                    }
                    if(!(i+1 < func->body_size)) {     //ID (NULL)
                        compileError(ERR_EXP, func->body[i].lin);
                        return 255;
                    }
                    i++;
                    int exp_start = i;
                    while(i+1 < func->body_size) {
                        i++;
                        if(wcsequ(func->body[i].value, L";")||wcsequ(func->body[i].value, L"；")) break;
                    }
                    if(!wcsequ(func->body[i].value, L";")&&!wcsequ(func->body[i].value, L"；")) {
                        error(ERR_NO_END, func->body[i].lin);
                        return 255;
                    }
                    int exp_end = i;
                    if(exp_end==exp_start) {
                        compileError(ERR_EXP, func->body[i].lin);
                        return 255;
                    }
                    ResultType result_type = {0};
                    int EXPErr = parseEXP(&(objCode.obj_fun[obj_fun_index].body), &cmd_index, &(objCode.obj_fun[obj_fun_index].body_size), &(func->body[exp_start]), exp_end-exp_start, &result_type, &local_sym, local_sym_size);
                    if(EXPErr) return EXPErr;
                    //检测类型


                    //生成指令
                    if(cmd_index >= objCode.obj_fun[obj_fun_index].body_size) {
                        objCode.obj_fun[obj_fun_index].body_size = cmd_index+1;
                        void* temp = realloc(objCode.obj_fun[obj_fun_index].body, sizeof(Command)*(objCode.obj_fun[obj_fun_index].body_size));
                        if(!temp) return -1;
                        objCode.obj_fun[obj_fun_index].body = (Command*)temp;
                        memset(&objCode.obj_fun[obj_fun_index].body[cmd_index], 0, sizeof(Command));
                    }
                    objCode.obj_fun[obj_fun_index].body[cmd_index].op_value_size = 1;
                    objCode.obj_fun[obj_fun_index].body[cmd_index].op = OP_MOVE;
                    objCode.obj_fun[obj_fun_index].body[cmd_index].op_value = (ObjValue*)calloc(1, sizeof(ObjValue));
                    objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].type = TYPE_SYM;
                    objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value = calloc(wcslen(symName)+1, sizeof(wchar));
                    if(!(objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value)) return -1;
                    objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].size = (wcslen(symName)+1)*sizeof(wchar);
                    wcscpy(((wchar*)objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value), symName);
#ifdef HX_DEBUG
                    printf("\33[33m生成指令：OP_MOVE\t%ls\33[0m\n", ((wchar*)objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value));
#endif
                    cmd_index++;

                } else if(wcsequ(func->body[i].value, L"(")||wcsequ(func->body[i].value, L"（")) { //调用函数
                    if(!(i+1 < func->body_size)) {     //ID (NULL)
                        error(ERR_NO_END, func->body[i].lin);
                        return 255;
                    }
                    i++;
                    if(wcsequ(func->body[i].value, L")")||wcsequ(func->body[i].value, L"）")) {   //无参
                        //查找函数.
                        CheckerFunction* funcPtr = NULL;
                        for(int j = 0; j < checkerOutput.func_size; j++) {  //查找函数
                            if(wcsequ(checkerOutput.checker_func[j].name, symName)&&(checkerOutput.checker_func[j].args == NULL)) {
                                funcPtr = &(checkerOutput.checker_func[j]);
                            }
                        }
                        if(funcPtr == NULL) {
                            compileError(ERR_FUN_NOT_DEFINED, mainPtr->body[i-1].lin);
                            return 255;
                        }
                        //检查函数是否已加载至objCode
                        bool isLoaded = false;
                        if(objCode.obj_fun_size == 0||objCode.obj_fun==NULL) isLoaded = false;
                        for(int j = 0; j < objCode.obj_fun_size; j++) {
                            if(wcsequ(objCode.obj_fun[j].name, symName)&&(objCode.obj_fun[j].args==NULL||objCode.obj_fun[j].args_size==0)) {
                                isLoaded = true;
                                break;
                            }
                        }
                        if(!isLoaded) {                                    // 加载函数到objCode
                            int flag = compile_fun(funcPtr);
                            if(flag != 1 && flag != 0) {
                                return flag;
                            } else if(flag==1) {
                                continue;
                            }
                        }
                        //OP_CALL <SYM_NAME>[args]
                        //生成指令
                        if(cmd_index >= objCode.obj_fun[obj_fun_index].body_size) {
                            objCode.obj_fun[obj_fun_index].body_size = cmd_index+1;
                            void* temp = realloc(objCode.obj_fun[obj_fun_index].body, sizeof(Command)*(objCode.obj_fun[obj_fun_index].body_size));
                            if(!temp) return -1;
                            objCode.obj_fun[obj_fun_index].body = (Command*)temp;
                            memset(&objCode.obj_fun[obj_fun_index].body[cmd_index], 0, sizeof(Command));
                        }
                        objCode.obj_fun[obj_fun_index].body[cmd_index].op_value_size = 1;
                        objCode.obj_fun[obj_fun_index].body[cmd_index].op = OP_CALL;
                        objCode.obj_fun[obj_fun_index].body[cmd_index].op_value = (ObjValue*)calloc(1, sizeof(ObjValue));
                        objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].type = TYPE_SYM;
                        objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value = calloc(wcslen(symName)+1, sizeof(wchar));
                        if(!(objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value )) return -1;
                        objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].size = (wcslen(symName)+1)*sizeof(wchar);
                        wcscpy(((wchar*)objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value), symName);
#ifdef HX_DEBUG
                        printf("\33[33m生成指令：OP_CALL\t%ls\33[0m\n", ((wchar*)objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value));
#endif
                        cmd_index++;

                    } else {   //有参数

                    }
                } else {  //废话

                }
            }
        }
        break;

        case TOK_KW: {
            if(wcsequ(func->body[i].value, L"var")||wcsequ(func->body[i].value, L"定义变量")) {
                if(local_sym==NULL) {
                    local_sym = (CheckerSymbol*)calloc(1, sizeof(CheckerSymbol));
                    if(!local_sym) return -1;
                    local_sym_size = 1;
                }
                if(sym_index>= local_sym_size) {
                    local_sym_size = sym_index+1;
                    void* temp = realloc(local_sym, local_sym_size*sizeof(CheckerSymbol));
                    if(!temp) return -1;
                    local_sym = (CheckerSymbol*)temp;
                }
                if(!(i+1 < func->body_size)) {
                    error(ERR_NO_VAR_NAME, func->body[i].lin);
                    return 255;
                }
                i++;
                if((!wcsequ(func->body[i].value, L":"))&&(!wcsequ(func->body[i].value, L"："))) {
                    error(ERR_NO_VAR_NAME, func->body[i].lin);
                    return 255;
                }
                if(!(i+1 < func->body_size)) {
                    error(ERR_NO_VAR_NAME, func->body[i].lin);
                    return 255;
                }
                i++;
                if(func->body[i].type != TOK_ID) {
                    error(ERR_NO_VAR_NAME, func->body[i].lin);
                    return 255;
                }
                CheckerSymbol* ptr = NULL;
                //检查是否重复定义
                if(findSymbol(func->body[i].value, &local_sym, local_sym_size-1, &ptr)) {
                    compileError(ERR_VAR_REPEAT_DEFINE, func->body[i].lin);
                    return 255;
                }
                local_sym[sym_index].isOnlyRead = false;
                local_sym[sym_index].name = (wchar*)calloc(wcslen(func->body[i].value)+1, sizeof(wchar));
                if(!(local_sym[sym_index].name)) return -1;
                wcscpy(local_sym[sym_index].name, func->body[i].value);
                //变量名
                if(!(i+1 < func->body_size)) {
                    error(ERR_BEHIND_SYMBOL_SHOULD_BE_DOUHAO, func->body[i].lin);
                    return 255;
                }
                i++;
                if(!wcsequ(func->body[i].value, L",")) {
                    error(ERR_BEHIND_SYMBOL_SHOULD_BE_DOUHAO, func->body[i].lin);
                    return 255;
                }
                if(!(i+1 < func->body_size)) {
                    error(ERR_SYM_NO_TYPE, func->body[i].lin);
                    return 255;
                }
                i++;
                if((!wcsequ(func->body[i].value, L"type"))&&(!wcsequ(func->body[i].value, L"它的类型是"))) {
                    error(ERR_SYM_NO_TYPE, func->body[i].lin);
                    return 255;
                }
                if(!(i+1 < func->body_size)) {
                    error(ERR_SYM_NO_TYPE, func->body[i].lin);
                    return 255;
                }
                i++;
                if((!wcsequ(func->body[i].value, L":"))&&(!wcsequ(func->body[i].value, L"："))) {
                    error(ERR_SYM_NO_TYPE, func->body[i].lin);
                    return 255;
                }
                if(!(i+1 < func->body_size)) {
                    error(ERR_SYM_NO_TYPE, func->body[i].lin);
                    return 255;
                }
                i++;
                if(func->body[i].type != TOK_ID && func->body[i].type != TOK_KW) {
                    error(ERR_SYM_NO_TYPE, func->body[i].lin);
                    return 255;
                }
                local_sym[sym_index].type = (wchar*)calloc(wcslen(func->body[i].value)+1, sizeof(wchar));
                if(!(local_sym[sym_index].type)) return -1;
                wcscpy(local_sym[sym_index].type, func->body[i].value);

                if(!(i+1 < func->body_size)) {
                    error(ERR_NO_END, func->body[i].lin);
                    return 255;
                }
                i++;
                if(cmd_index >= objCode.obj_fun[obj_fun_index].body_size) {
                    objCode.obj_fun[obj_fun_index].body_size = cmd_index+1;
                    void* temp = realloc(objCode.obj_fun[obj_fun_index].body, sizeof(Command)*(objCode.obj_fun[obj_fun_index].body_size));
                    if(!temp) return -1;
                    objCode.obj_fun[obj_fun_index].body = (Command*)temp;
                    memset(&objCode.obj_fun[obj_fun_index].body[cmd_index], 0, sizeof(Command));
                }

                objCode.obj_fun[obj_fun_index].body[cmd_index].op = OP_DEFINE_VAR;
                objCode.obj_fun[obj_fun_index].body[cmd_index].op_value = (ObjValue*)calloc(2, sizeof(ObjValue));
                objCode.obj_fun[obj_fun_index].body[cmd_index].op_value_size = 2;
                (objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value) = (wchar*)calloc(wcslen(local_sym[sym_index].name)+1, sizeof(wchar));
                if(!(objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value)) return -1;
                wcscpy((wchar*)(objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value), local_sym[sym_index].name);

                (objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[1].value) = (wchar*)calloc(wcslen(local_sym[sym_index].type)+1, sizeof(wchar));
                if(!(objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[1].value)) return -1;
                wcscpy((wchar*)(objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[1].value), local_sym[sym_index].type);
#ifdef HX_DEBUG
                printf("\33[33m生成指令：OP_DEFINE_VAR\t%ls  %ls\n\33[0m", (wchar*)(objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value), (wchar*)(objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[1].value));
#endif
                cmd_index++;
                if(wcsequ(func->body[i].value, L";")||wcsequ(func->body[i].value, L"；")) {
                    sym_index++;
                    break;
                } else {
                    if(!wcsequ(func->body[i].value, L"=")) {
                        compileError(ERR_VAR_DEFINITION, func->body[i].lin);
                        return 255;
                    }
                    if(!(i+1 < func->body_size)) {
                        error(ERR_VAR_DEFINITION, func->body[i].lin);
                        return 255;
                    }
                    i++;   //此时时向表达式
                    //分析表达式
                    int exp_start = i;
                    while(i+1 < func->body_size) {
                        if(wcsequ(func->body[i].value, L";")||wcsequ(func->body[i].value, L"；")) break;
                        i++;
                    }
                    if(!wcsequ(func->body[i].value, L";")&&!wcsequ(func->body[i].value, L"；")) {
                        error(ERR_NO_END, func->body[i].lin);
                        return 255;
                    }
                    int exp_end = i;
                    if(exp_end==exp_start) {
                        error(ERR_EXP, func->body[i].lin);
                        return 255;
                    }
                    ResultType result_type = {0};
                    int EXPErr = parseEXP(&(objCode.obj_fun[obj_fun_index].body), &cmd_index, &(objCode.obj_fun[obj_fun_index].body_size), &(func->body[exp_start]), exp_end-exp_start, &result_type, &local_sym, local_sym_size);
                    if(EXPErr) return EXPErr;
                    //检测类型




                    //生成指令
                    if(cmd_index >= objCode.obj_fun[obj_fun_index].body_size) {
                        objCode.obj_fun[obj_fun_index].body_size = cmd_index+1;
                        void* temp = realloc(objCode.obj_fun[obj_fun_index].body, sizeof(Command)*(objCode.obj_fun[obj_fun_index].body_size));
                        if(!temp) return -1;
                        objCode.obj_fun[obj_fun_index].body = (Command*)temp;
                        memset(&objCode.obj_fun[obj_fun_index].body[cmd_index], 0, sizeof(Command));
                    }
                    objCode.obj_fun[obj_fun_index].body[cmd_index].op_value_size = 1;
                    objCode.obj_fun[obj_fun_index].body[cmd_index].op = OP_MOVE;
                    objCode.obj_fun[obj_fun_index].body[cmd_index].op_value = (ObjValue*)calloc(1, sizeof(ObjValue));
                    objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].type = TYPE_SYM;
                    objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value = calloc(wcslen(local_sym[sym_index].name)+1, sizeof(wchar));
                    if(!(objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value )) return -1;
                    objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].size = (wcslen(local_sym[sym_index].name)+1)*sizeof(wchar);
                    wcscpy(((wchar*)objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value), local_sym[sym_index].name);
#ifdef HX_DEBUG
                    printf("\33[33m生成指令：OP_MOVE\t%ls\33[0m\n", ((wchar*)objCode.obj_fun[obj_fun_index].body[cmd_index].op_value[0].value));
#endif
                    sym_index++;
                    cmd_index++;
                }
            }
        }
        break;
        }
    }
    free_local_symbol_table(local_sym, local_sym_size);
    local_sym = NULL;
    return 0;
}
/*编译：
*分析main函数,仅写入main函数内调用过的函数
*/
int compile(CheckerOutput* IR) {
    if(ckeckMainFunction(IR)) {
        return 255;
    }
    memset(&objCode, 0, sizeof(ObjectCode));
    CheckerSymbol* local_sym = NULL;
    int local_sym_size = 0;
    int sym_index = 0;
    obj_fun_index++;
    if(mainPtr->args == NULL) {                 //无参main
        if(mainPtr->body == NULL||mainPtr->body_size == 0) {             //空函数不进行任何操作
            //写入空的main函数
            if(objCode.obj_fun==NULL) {
                objCode.obj_fun = (ObjFunction*)calloc(1, sizeof(ObjFunction));
                objCode.obj_fun_size = 1;
                if(!(objCode.obj_fun)) return -1;
            }
            //函数名
            objCode.obj_fun[0].name = (wchar*)calloc(5, sizeof(wchar));
            if(!(objCode.obj_fun[0].name)) return 0;
            wcscpy(objCode.obj_fun[0].name, L"main");
            //printf("%ls\n", objCode.obj_fun[0].name);

            objCode.obj_fun[0].args = NULL;
            objCode.obj_fun[0].args_size = 0;
            objCode.obj_fun[0].body = NULL;
            objCode.obj_fun[0].body_size = 0;
            objCode.obj_fun[0].ret_type = (wchar*)calloc(4, sizeof(wchar));
            if(!(objCode.obj_fun[0].ret_type)) return -1;
            wcscpy(objCode.obj_fun[0].ret_type, L"int");
            //printf("%ls\n",objCode.obj_fun[0].ret_type);
            objCode.start_fun = 0;
            return 0;
        } else {
            if(objCode.obj_fun==NULL) {
                objCode.obj_fun = (ObjFunction*)calloc(1, sizeof(ObjFunction));
                objCode.obj_fun_size = 1;
                if(!(objCode.obj_fun)) return -1;
            }
            //函数名
            objCode.obj_fun[0].name = (wchar*)calloc(5, sizeof(wchar));
            if(!(objCode.obj_fun[0].name)) return 0;
            wcscpy(objCode.obj_fun[0].name, L"main");
            //printf("%ls\n", objCode.obj_fun[0].name);
            objCode.obj_fun[0].args = NULL;
            objCode.obj_fun[0].args_size = 0;
            objCode.obj_fun[0].ret_type = (wchar*)calloc(4, sizeof(wchar));
            if(!(objCode.obj_fun[0].ret_type)) return -1;
            wcscpy(objCode.obj_fun[0].ret_type, L"int");
            objCode.start_fun = 0;
            ObjFunction* objMainPtr = &(objCode.obj_fun[0]);

            objMainPtr->body = (Command*)calloc(1, sizeof(Command));
            if(!(objMainPtr->body)) return -1;
            objMainPtr->body_size = 1;

            int cmd_index = 0;
//分析函数体
            for(int i = 0; i < mainPtr->body_size; i++) {
                switch(mainPtr->body[i].type) {
                case TOK_ID: {
                    if(wcsequ(mainPtr->body[i].value, L"putString") || wcsequ(mainPtr->body[i].value, L"输出字符串")) {
                        if(i+1 >= mainPtr->body_size) {
                            compileError(ERR_CALL_FUN, mainPtr->body[i].lin);
                            return 255;
                        }
                        i++;
                        if((!wcsequ(mainPtr->body[i].value, L"("))&&(!wcsequ(mainPtr->body[i].value, L"（"))) {
                            compileError(ERR_CALL_FUN, mainPtr->body[i].lin);
                            return 255;
                        }
                        if(i+1 >= mainPtr->body_size) {
                            compileError(ERR_CALL_FUN, mainPtr->body[i].lin);
                            return 255;
                        }
                        i++;
                        int exp_start = i;
                        int count = 0;
                        int open = 1;
                        int close = 0;
                        while(i < mainPtr->body_size-1) {
                            if(wcsequ(mainPtr->body[i].value, L"(")||wcsequ(mainPtr->body[i].value, L"（")) {
                                open++;
                            }
                            if(wcsequ(mainPtr->body[i].value, L")")||wcsequ(mainPtr->body[i].value, L"）")) {
                                close++;
                            }
                            if(open == close) break;
                            count++;
                            i++;
                        }
                        if(open!=close) {
                            compileError(ERR_QUITE_NOT_CORRECTLY_CLOSE, mainPtr->body[i].lin);
                            return 255;
                        }
                        int exp_end = i;
                        if(exp_end-exp_start==0) {
                            compileError(ERR_FUN_ARGS_SIZE,mainPtr->body[i].lin);
                            return 255;
                        }
                        //分析参数
                        ResultType result_type = {0};
                        int EXPErr = parseEXP(&(objMainPtr->body), &cmd_index, &(objMainPtr->body_size), &(mainPtr->body[exp_start]), exp_end-exp_start, &result_type, &local_sym, local_sym_size);
                        if(EXPErr) return EXPErr;

                        if(result_type.type != RESULT_TYPE_STR||result_type.type==UNKNOWN) {
                            compileError(ERR_FUN_ARGS_TYPE, mainPtr->body[i].lin);
                            return 255;
                        }
                        //生成指令OP_PUT_STR
                        if(cmd_index >= objMainPtr->body_size) {
                            objMainPtr->body_size = cmd_index+1;
                            void* temp = realloc(objMainPtr->body, sizeof(Command)*(objMainPtr->body_size));
                            if(!temp) return -1;
                            objMainPtr->body = (Command*)temp;
                            memset(&objMainPtr->body[cmd_index], 0, sizeof(Command));
                        }
                        objMainPtr->body[cmd_index].op_value_size = 1;
                        objMainPtr->body[cmd_index].op = OP_PUT_STR;
                        objMainPtr->body[cmd_index].op_value = (ObjValue*)calloc(1, sizeof(ObjValue));
                        objMainPtr->body[cmd_index].op_value[0].type = TYPE_STR;
#ifdef HX_DEBUG
                        printf("生成指令[%d]：OP_PUT_STR \n", cmd_index);
#endif
                        if(objMainPtr->body_size <= cmd_index) {
                            objMainPtr->body_size = cmd_index+1;
                            void* temp = realloc(objMainPtr->body, objMainPtr->body_size*sizeof(Command));
                            if(!temp) return -1;
                            objMainPtr->body = (Command*)temp;
                        }
                        memset(&objMainPtr->body[cmd_index], 0, sizeof(Command));

                        if(!(i+1 < mainPtr->body_size)) {
                            error(ERR_NO_END, mainPtr->body[i].lin);
                            return 255;
                        }
                        i++;
#ifdef HX_DEBUG
                        //printf("%ls\n", mainPtr->body[i].value);
#endif

                        if(!wcsequ(mainPtr->body[i].value, L";")&&!wcsequ(mainPtr->body[i].value, L"；")) {
                            error(ERR_NO_END, mainPtr->body[i].lin);
                            return 255;
                        }
                        cmd_index++;
                    } else {     //调用函数或赋值
                        if(!(i+1 < mainPtr->body_size)) {     //ID (NULL)
                            error(ERR_NO_END, mainPtr->body[i].lin);
                            return 255;
                        }
                        wchar* symName = mainPtr->body[i].value;      //符号名
                        i++;
                        if(wcsequ(mainPtr->body[i].value, L";")||wcsequ(mainPtr->body[i].value, L"；")) {    //ID ;|；
                            //废话
                            break;
                        } else if(wcsequ(mainPtr->body[i].value, L"=")) {   //赋值
                            CheckerSymbol* symPtr = NULL;
                            //检查符号是否定义
                            if(!findSymbol(symName, &local_sym, local_sym_size, &symPtr)) {
                                compileError(ERR_SYM_NOT_DEFINED, mainPtr->body[i-1].lin);
                                return 255;
                            }
                            //检查是否为常量"
                            if((symPtr->isOnlyRead)&&(symPtr->isInited)) {
                                compileError(ERR_CON_CAN_NOT_MOVE, mainPtr->body[i-1].lin);
                                return 255;
                            }
                            if(!(i+1 < mainPtr->body_size)) {     //ID (NULL)
                                compileError(ERR_EXP, mainPtr->body[i].lin);
                                return 255;
                            }
                            i++;
                            int exp_start = i;
                            while(i+1 < mainPtr->body_size) {
                                i++;
                                if(wcsequ(mainPtr->body[i].value, L";")||wcsequ(mainPtr->body[i].value, L"；")) break;
                            }
                            if(!wcsequ(mainPtr->body[i].value, L";")&&!wcsequ(mainPtr->body[i].value, L"；")) {
                                error(ERR_NO_END, mainPtr->body[i].lin);
                                return 255;
                            }
                            int exp_end = i;
                            if(exp_end==exp_start) {
                                compileError(ERR_EXP, mainPtr->body[i].lin);
                                return 255;
                            }
                            ResultType result_type = {0};
                            int EXPErr = parseEXP(&(objMainPtr->body), &cmd_index, &(objMainPtr->body_size), &(mainPtr->body[exp_start]), exp_end-exp_start, &result_type, &local_sym, local_sym_size);
                            if(EXPErr) return EXPErr;
                            //检测类型


                            //生成指令
                            if(cmd_index >= objMainPtr->body_size) {
                                objMainPtr->body_size = cmd_index+1;
                                void* temp = realloc(objMainPtr->body, sizeof(Command)*(objMainPtr->body_size));
                                if(!temp) return -1;
                                objMainPtr->body = (Command*)temp;
                                memset(&objMainPtr->body[cmd_index], 0, sizeof(Command));
                            }
                            objMainPtr->body[cmd_index].op_value_size = 1;
                            objMainPtr->body[cmd_index].op = OP_MOVE;
                            objMainPtr->body[cmd_index].op_value = (ObjValue*)calloc(1, sizeof(ObjValue));
                            objMainPtr->body[cmd_index].op_value[0].type = TYPE_SYM;
                            objMainPtr->body[cmd_index].op_value[0].value = calloc(wcslen(symName)+1, sizeof(wchar));
                            if(!(objMainPtr->body[cmd_index].op_value[0].value )) return -1;
                            objMainPtr->body[cmd_index].op_value[0].size = (wcslen(symName)+1)*sizeof(wchar);
                            wcscpy(((wchar*)objMainPtr->body[cmd_index].op_value[0].value), symName);
#ifdef HX_DEBUG
                            printf("\33[33m生成指令：OP_MOVE\t%ls\33[0m\n", ((wchar*)objMainPtr->body[cmd_index].op_value[0].value));
#endif
                            cmd_index++;

                        } else if(wcsequ(mainPtr->body[i].value, L"(")||wcsequ(mainPtr->body[i].value, L"（")) { //调用函数
                            if(!(i+1 < mainPtr->body_size)) {     //ID (NULL)
                                error(ERR_NO_END, mainPtr->body[i].lin);
                                return 255;
                            }
                            i++;
                            if(wcsequ(mainPtr->body[i].value, L")")||wcsequ(mainPtr->body[i].value, L"）")) {   //无参
                                //查找函数.
                                CheckerFunction* funcPtr = NULL;
                                for(int j = 0; j < checkerOutput.func_size; j++) {  //查找函数
                                    if(wcsequ(checkerOutput.checker_func[j].name, symName)&&(checkerOutput.checker_func[j].args == NULL)) {
                                        funcPtr = &(checkerOutput.checker_func[j]);
                                    }
                                }
                                if(funcPtr == NULL) {
                                    compileError(ERR_FUN_NOT_DEFINED, mainPtr->body[i-1].lin);
                                    return 255;
                                }
                                //检查函数是否已加载至objCode
                                bool isLoaded = false;
                                if(objCode.obj_fun_size == 0||objCode.obj_fun==NULL) isLoaded = false;
                                for(int j = 0; j < objCode.obj_fun_size; j++) {
                                    if(wcsequ(objCode.obj_fun[j].name, symName)&&(objCode.obj_fun[j].args==NULL||objCode.obj_fun[j].args_size==0)) {
                                        isLoaded = true;
                                        break;
                                    }
                                }
                                if(!isLoaded) {                                    // 加载函数到objCode
                                    int flag = compile_fun(funcPtr);
                                    // 在 realloc 可能发生后，立即刷新 objMainPtr 指针 <==
                                    objMainPtr = &(objCode.obj_fun[0]);
                                    if(flag != 1 && flag != 0) {
                                        return flag;
                                    } else if(flag==1) {
                                        continue;
                                    }
                                }
                                //OP_CALL <SYM_NAME>[args]
                                //生成指令
                                if(cmd_index >= objMainPtr->body_size) {
                                    objMainPtr->body_size = cmd_index+1;
                                    void* temp = realloc(objMainPtr->body, sizeof(Command)*(objMainPtr->body_size));
                                    if(!temp) return -1;
                                    objMainPtr->body = (Command*)temp;
                                    memset(&objMainPtr->body[cmd_index], 0, sizeof(Command));
                                }
                                objMainPtr->body[cmd_index].op_value_size = 1;
                                objMainPtr->body[cmd_index].op = OP_CALL;
                                objMainPtr->body[cmd_index].op_value = (ObjValue*)calloc(1, sizeof(ObjValue));
                                objMainPtr->body[cmd_index].op_value[0].type = TYPE_SYM;
                                objMainPtr->body[cmd_index].op_value[0].value = calloc(wcslen(symName)+1, sizeof(wchar));
                                if(!(objMainPtr->body[cmd_index].op_value[0].value )) return -1;
                                objMainPtr->body[cmd_index].op_value[0].size = (wcslen(symName)+1)*sizeof(wchar);
                                wcscpy(((wchar*)objMainPtr->body[cmd_index].op_value[0].value), symName);
#ifdef HX_DEBUG
                                printf("\33[33m生成指令：OP_CALL\t%ls\33[0m\n", ((wchar*)objMainPtr->body[cmd_index].op_value[0].value));
#endif
                                cmd_index++;

                            }  else {   //有参数

                            }
                        } else { //废话

                        }
                    }
                    break;

                    case TOK_KW: {
                        if(wcsequ(mainPtr->body[i].value, L"var")||wcsequ(mainPtr->body[i].value, L"定义变量")) {
                            if(local_sym==NULL) {
                                local_sym = (CheckerSymbol*)calloc(1, sizeof(CheckerSymbol));
                                if(!local_sym) return -1;
                                local_sym_size = 1;
                            }
                            if(sym_index>= local_sym_size) {
                                local_sym_size = sym_index+1;
                                void* temp = realloc(local_sym, local_sym_size*sizeof(CheckerSymbol));
                                if(!temp) return -1;
                                local_sym = (CheckerSymbol*)temp;
                            }
                            if(!(i+1 < mainPtr->body_size)) {
                                error(ERR_NO_VAR_NAME, mainPtr->body[i].lin);
                                return 255;
                            }
                            i++;
                            if((!wcsequ(mainPtr->body[i].value, L":"))&&(!wcsequ(mainPtr->body[i].value, L"："))) {
                                error(ERR_NO_VAR_NAME, mainPtr->body[i].lin);
                                return 255;
                            }
                            if(!(i+1 < mainPtr->body_size)) {
                                error(ERR_NO_VAR_NAME, mainPtr->body[i].lin);
                                return 255;
                            }
                            i++;
                            if(mainPtr->body[i].type != TOK_ID) {
                                error(ERR_NO_VAR_NAME, mainPtr->body[i].lin);
                                return 255;
                            }
                            CheckerSymbol* ptr = NULL;
                            //检查是否重复定义
                            if(findSymbol(mainPtr->body[i].value, &local_sym, local_sym_size-1, &ptr)) {
                                compileError(ERR_VAR_REPEAT_DEFINE, mainPtr->body[i].lin);
                                return 255;
                            }
                            local_sym[sym_index].isOnlyRead = false;
                            local_sym[sym_index].name = (wchar*)calloc(wcslen(mainPtr->body[i].value)+1, sizeof(wchar));
                            if(!(local_sym[sym_index].name)) return -1;
                            wcscpy(local_sym[sym_index].name, mainPtr->body[i].value);
                            //变量名
                            if(!(i+1 < mainPtr->body_size)) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_DOUHAO, mainPtr->body[i].lin);
                                return 255;
                            }
                            i++;
                            if(!wcsequ(mainPtr->body[i].value, L",")) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_DOUHAO, mainPtr->body[i].lin);
                                return 255;
                            }
                            if(!(i+1 < mainPtr->body_size)) {
                                error(ERR_SYM_NO_TYPE, mainPtr->body[i].lin);
                                return 255;
                            }
                            i++;
                            if((!wcsequ(mainPtr->body[i].value, L"type"))&&(!wcsequ(mainPtr->body[i].value, L"它的类型是"))) {
                                error(ERR_SYM_NO_TYPE, mainPtr->body[i].lin);
                                return 255;
                            }
                            if(!(i+1 < mainPtr->body_size)) {
                                error(ERR_SYM_NO_TYPE, mainPtr->body[i].lin);
                                return 255;
                            }
                            i++;
                            if((!wcsequ(mainPtr->body[i].value, L":"))&&(!wcsequ(mainPtr->body[i].value, L"："))) {
                                error(ERR_SYM_NO_TYPE, mainPtr->body[i].lin);
                                return 255;
                            }
                            if(!(i+1 < mainPtr->body_size)) {
                                error(ERR_SYM_NO_TYPE, mainPtr->body[i].lin);
                                return 255;
                            }
                            i++;
                            if(mainPtr->body[i].type != TOK_ID && mainPtr->body[i].type != TOK_KW) {
                                error(ERR_SYM_NO_TYPE, mainPtr->body[i].lin);
                                return 255;
                            }
                            local_sym[sym_index].type = (wchar*)calloc(wcslen(mainPtr->body[i].value)+1, sizeof(wchar));
                            if(!(local_sym[sym_index].type)) return -1;
                            wcscpy(local_sym[sym_index].type, mainPtr->body[i].value);

                            if(!(i+1 < mainPtr->body_size)) {
                                error(ERR_NO_END, mainPtr->body[i].lin);
                                return 255;
                            }
                            i++;
                            if(cmd_index >= objMainPtr->body_size) {
                                objMainPtr->body_size = cmd_index+1;
                                void* temp = realloc(objMainPtr->body, sizeof(Command)*(objMainPtr->body_size));
                                if(!temp) return -1;
                                objMainPtr->body = (Command*)temp;
                                memset(&objMainPtr->body[cmd_index], 0, sizeof(Command));
                            }

                            objMainPtr->body[cmd_index].op = OP_DEFINE_VAR;
                            objMainPtr->body[cmd_index].op_value = (ObjValue*)calloc(2, sizeof(ObjValue));
                            objMainPtr->body[cmd_index].op_value_size = 2;
                            (objMainPtr->body[cmd_index].op_value[0].value) = (wchar*)calloc(wcslen(local_sym[sym_index].name)+1, sizeof(wchar));
                            if(!(objMainPtr->body[cmd_index].op_value[0].value)) return -1;
                            wcscpy((wchar*)(objMainPtr->body[cmd_index].op_value[0].value), local_sym[sym_index].name);

                            (objMainPtr->body[cmd_index].op_value[1].value) = (wchar*)calloc(wcslen(local_sym[sym_index].type)+1, sizeof(wchar));
                            if(!(objMainPtr->body[cmd_index].op_value[1].value)) return -1;
                            wcscpy((wchar*)(objMainPtr->body[cmd_index].op_value[1].value), local_sym[sym_index].type);
#ifdef HX_DEBUG
                            printf("\33[33m生成指令：OP_DEFINE_VAR\t%ls  %ls\n\33[0m", (wchar*)(objMainPtr->body[cmd_index].op_value[0].value), (wchar*)(objMainPtr->body[cmd_index].op_value[1].value));
#endif
                            cmd_index++;
                            if(wcsequ(mainPtr->body[i].value, L";")||wcsequ(mainPtr->body[i].value, L"；")) {
                                sym_index++;
                                break;
                            } else {
                                if(!wcsequ(mainPtr->body[i].value, L"=")) {
                                    compileError(ERR_VAR_DEFINITION, mainPtr->body[i].lin);
                                    return 255;
                                }
                                if(!(i+1 < mainPtr->body_size)) {
                                    error(ERR_VAR_DEFINITION, mainPtr->body[i].lin);
                                    return 255;
                                }
                                i++;   //此时时向表达式
                                //分析表达式
                                int exp_start = i;
                                while(i+1 < mainPtr->body_size) {
                                    if(wcsequ(mainPtr->body[i].value, L";")||wcsequ(mainPtr->body[i].value, L"；")) break;
                                    i++;
                                }
                                if(!wcsequ(mainPtr->body[i].value, L";")&&!wcsequ(mainPtr->body[i].value, L"；")) {
                                    error(ERR_NO_END, mainPtr->body[i].lin);
                                    return 255;
                                }
                                int exp_end = i;
                                if(exp_end==exp_start) {
                                    error(ERR_EXP, mainPtr->body[i].lin);
                                    return 255;
                                }
                                ResultType result_type = {0};
                                int EXPErr = parseEXP(&(objMainPtr->body), &cmd_index, &(objMainPtr->body_size), &(mainPtr->body[exp_start]), exp_end-exp_start, &result_type, &local_sym, local_sym_size);
                                if(EXPErr) return EXPErr;
                                //检测类型




                                //生成指令
                                if(cmd_index >= objMainPtr->body_size) {
                                    objMainPtr->body_size = cmd_index+1;
                                    void* temp = realloc(objMainPtr->body, sizeof(Command)*(objMainPtr->body_size));
                                    if(!temp) return -1;
                                    objMainPtr->body = (Command*)temp;
                                    memset(&objMainPtr->body[cmd_index], 0, sizeof(Command));
                                }
                                objMainPtr->body[cmd_index].op_value_size = 1;
                                objMainPtr->body[cmd_index].op = OP_MOVE;
                                objMainPtr->body[cmd_index].op_value = (ObjValue*)calloc(1, sizeof(ObjValue));
                                objMainPtr->body[cmd_index].op_value[0].type = TYPE_SYM;
                                objMainPtr->body[cmd_index].op_value[0].value = calloc(wcslen(local_sym[sym_index].name)+1, sizeof(wchar));
                                if(!(objMainPtr->body[cmd_index].op_value[0].value )) return -1;
                                objMainPtr->body[cmd_index].op_value[0].size = (wcslen(local_sym[sym_index].name)+1)*sizeof(wchar);
                                wcscpy(((wchar*)objMainPtr->body[cmd_index].op_value[0].value), local_sym[sym_index].name);
#ifdef HX_DEBUG
                                printf("\33[33m生成指令：OP_MOVE\t%ls\33[0m\n", ((wchar*)objMainPtr->body[cmd_index].op_value[0].value));
#endif
                                sym_index++;
                                cmd_index++;
                            }
                        }
                    }
                    break;
                }
                }
            }
        }
    }
    // 释放局部的 CheckerSymbol 数组
    free_local_symbol_table(local_sym, local_sym_size);
    local_sym = NULL;
    return 0;
}
bool findSymbol(wchar* name, CheckerSymbol** table, int table_size, CheckerSymbol** ptr) {
    if (name == NULL) return false;
    //printf("%ls\n",name);
    /* 如果传入的 table 指针本身为空 -> 直接查全局或返回 false */
    if (table == NULL) {
        if (checkerOutput.global_sym == NULL) return false;
        for (int i = 0; i < checkerOutput.global_sym_size; ++i) {
            /*确保 global_sym[i].name 非空    */
            if (checkerOutput.global_sym[i].name == NULL) continue;
            if (wcsequ(name, checkerOutput.global_sym[i].name)) {
                if(ptr!=NULL) {
                    *ptr = &(checkerOutput.global_sym[i]);
                }
                return true;
            }
        }
        return false;
    }

    /* 如果*table 为 NULL 或 table_size <= 0，则退回查找全局符号表 */
    if (*table == NULL || table_size <= 0) {
        if (checkerOutput.global_sym == NULL) return false;
        for (int i = 0; i < checkerOutput.global_sym_size; ++i) {
            if (checkerOutput.global_sym[i].name == NULL) continue;
            if (wcsequ(name, checkerOutput.global_sym[i].name)) {
                if(ptr!=NULL) {
                    *ptr = &(checkerOutput.global_sym[i]);
                }
                return true;
            }
        }
        return false;
    }   /* 查找局部表*/
    for (int i = 0; i < table_size; ++i) {
        const CheckerSymbol *entry = &((*table)[i]);
        if (entry == NULL) continue;
        if (entry->name == NULL) continue;
        if (wcsequ(name, entry->name)) {
            if(ptr!=NULL) {
                *ptr = entry;
            }
            return true;
        }
    }

    /* 最后再查一遍全局符号（按原逻辑） */
    if (checkerOutput.global_sym == NULL) return false;
    for (int i = 0; i < checkerOutput.global_sym_size; ++i) {
        if (checkerOutput.global_sym[i].name == NULL) continue;
        if (wcsequ(name, checkerOutput.global_sym[i].name)) {
            if(ptr!=NULL) {
                *ptr = &(checkerOutput.global_sym[i]);
            }
            return true;
        }
    }
    return false;
}
ResultType parseType(wchar* str) {
    ResultType type = {0};
    type.type = UNKNOWN;
    if(str==NULL) return type;
    if(wcslen(str)==0) return type;
    int index = 0;
    if(str[0]!=L'\0') {
        if(str[0]==L'0'&&str[1]==L'x') {
            type.type = RESULT_TYPE_INT;
            return type;
        }
    }
    while(str[index]!=L'\0') {
        index++;
        if(str[index]==L'.') {
            while(str[index+1]!=L'\0') index++;
            if(str[index] == L'f'||str[index] == L'F') {
                type.type = RESULT_TYPE_FLOAT;
            } else {
                type.type = RESULT_TYPE_DOUBLE;
            }
            return type;
        }
    }
    type.type = RESULT_TYPE_INT;
    return type;
}
int ckeckMainFunction(CheckerOutput* IR) {
    if(IR==NULL) {
        compileError(ERR_NO_FUN_MAIN, 0);
        return 255;
    }
    if(IR->checker_func == NULL) {
        compileError(ERR_NO_FUN_MAIN, 0);
        return 255;
    }
    bool isMainExists = false;
    for(int i = 0; i < IR->func_size; i++) {
        if(wcsequ(IR->checker_func[i].name, L"main")||wcsequ(IR->checker_func[i].name, L"主函数")) {
            if(isMainExists) {
                compileError(ERR_RELOAD_MAIN, IR->checker_func[i].line);
                return 255;
            }
            if(IR->checker_func[i].args == NULL) {
                isMainExists = true;
            } else if(IR->checker_func[i].args_size == 1) {
                if((!wcsequ(IR->checker_func[i].args[0].type, L"String"))&&(!wcsequ(IR->checker_func[i].args[0].type, L"字符串型"))) {
                    compileError(ERR_MAIN_ARGS, IR->checker_func[i].line);
                    return 255;
                }
            } else {
                compileError(ERR_MAIN_ARGS, IR->checker_func[i].line);
                return 255;
            }
            mainPtr = &(IR->checker_func[i]);
        } else {
            continue;
        }
    }
    if(isMainExists) {
        if((!wcsequ(mainPtr->ret_type, L"int"))&&(!wcsequ(mainPtr->ret_type, L"整型"))) {
            compileError(ERR_MAIN_RET_TYPE, mainPtr->line);
            return 255;
        }
        return 0;
    } else {
        return 255;
    }
}
int allocOpValueByType(const ResultType* type, void** ptr) {
    if(ptr==NULL||type==NULL) return -1;
    switch(type->type) {
    case RESULT_TYPE_INT: {
        (*ptr) = (long int*)calloc(1, sizeof(long int));      //程序运行到这里时出现异常
        if(!(*ptr)) return -1;
    }
    break;
    case RESULT_TYPE_FLOAT: {
        *ptr = calloc(1, sizeof(float));
        if(!(*ptr)) return -1;
    }
    break;
    case RESULT_TYPE_DOUBLE: {
        *ptr = calloc(1, sizeof(double));
        if(!(*ptr)) return -1;
    }
    break;
    }
    return 0;
}
int setValueByType(const ResultType* type, const wchar* str, void** ptr) {
    if((!type)||(!str)||!ptr) return -1;
    if(!(*ptr)) return -1;
    wchar* end = NULL;
    switch(type->type) {
    case RESULT_TYPE_INT: {
        *(long int*)(*ptr) = wcstol(str,&end, 0);
    }
    break;

    case RESULT_TYPE_DOUBLE: {
        *(double*)(*ptr) = wcstod(str,&end);
    }
    break;

    case RESULT_TYPE_FLOAT: {
        *(float*)(*ptr) = (float)wcstod(str,&end);
    }
    break;
    }
    return 0;
}
void compileError(CompileErrorType type, int errLine) {
    initLocale();
    switch(type) {
    case ERR_NO_FUN_MAIN: {
        fwprintf(stderr, L"\33[31m[E]缺少主函数！\33[0m\n");
    }
    break;
    case ERR_RELOAD_MAIN: {
        fwprintf(stderr, L"\33[31m[E]主函数不能被重载！(位于第%d行)\33[0m\n", errLine);
    }
    break;
    case ERR_MAIN_ARGS: {
        fwprintf(stderr, L"\33[31m[E]主函数的参数只能是字符串型或无参数！\33[0m\n");
    }
    break;

    case ERR_MAIN_RET_TYPE: {
        fwprintf(stderr, L"\33[31m[E]主函数的返回值应为整型！\33[0m\n");
    }
    break;

    case ERR_CALL_FUN: {
        fwprintf(stderr, L"\33[31m[E]调用函数的语法错误！(位于第%d行)\33[0m\n", errLine);
    }
    break;

    case ERR_QUITE_NOT_CORRECTLY_CLOSE: {
        fwprintf(stderr, L"\33[31m[E]括号未正确闭合！(位于第%d行)\33[0m\n", errLine);
    }
    break;

    case ERR_FUN_ARGS_SIZE: {
        fwprintf(stderr, L"\33[31m[E]传递给函数的参数的个数有误！(位于第%d行)\33[0m\n", errLine);
    }
    break;

    case ERR_FUN_ARGS_TYPE: {
        fwprintf(stderr, L"\33[31m[E]传递给函数的参数的类型不匹配！(位于第%d行)\33[0m\n", errLine);
    }
    break;

    case ERR_VAR_REPEAT_DEFINE: {
        fwprintf(stderr, L"\33[31m[E]变量重复定义！(位于第%d行)\33[0m\n", errLine);
    }
    break;

    case ERR_SYM_NOT_DEFINED: {
        fwprintf(stderr, L"\33[31m[E]符号未定义！(位于第%d行)\33[0m\n", errLine);
    }
    break;

    case ERR_EXP: {
        fwprintf(stderr, L"\33[31m[E]错误的表达式！(位于第%d行)\33[0m\n", errLine);
    }
    break;

    case ERR_VAR_DEFINITION: {
        fwprintf(stderr, L"\33[31m[E]定义变量的语法错误！(位于第%d行)\33[0m\n", errLine);
    }
    break;

    case ERR_CON_CAN_NOT_MOVE: {
        fwprintf(stderr, L"\33[31m[E]常量符号不可赋值！(位于第%d行)\33[0m\n", errLine);
    }
    break;

    case ERR_FUN_NOT_DEFINED: {
        fwprintf(stderr, L"\33[31m[E]函数未定义！(位于第%d行)\33[0m\n", errLine);
    }
    }
    return;
}
void stringEscape(wchar* str) {
    if(str==NULL) return;
    int i = 0;
    while(str[i] != L'\0') {
        if(str[i] == L'\\') {
            i++;
            if(str[i] == L'\0') {
                break;
            } else {
                if(str[i]==L't') {
                    i--;
                    str[i] = L'\t';
                    i++;
                    int j = i;
                    while(str[j]!=L'\0') {
                        str[j] = str[j+1];
                        j++;
                    }
                    continue;
                } else if(str[i]==L'n') {
                    i--;
                    str[i] = L'\n';
                    i++;
                    int j = i;
                    while(str[j]!=L'\0') {
                        str[j] = str[j+1];
                        j++;
                    }
                    continue;
                } else if(str[i]==L'a') {
                    i--;
                    str[i] = L'\a';
                    i++;
                    int j = i;
                    while(str[j]!=L'\0') {
                        str[j] = str[j+1];
                        j++;
                    }
                    continue;
                } else if(str[i]==L'3' && str[i+1]==L'3') {
                    i--;
                    str[i] = L'\33';
                    i++;
                    int j = i;
                    while(str[j]!=L'\0') {
                        if(str[j+1]==L'\0') {
                            str[j] = str[j+1];
                            break;
                        }
                        str[j] = str[j+2];
                        j++;
                    }
                    continue;
                }
            }
        }
        i++;
    }
}
void freeObjectCode(ObjectCode *code) {
    if (code == NULL) return;
    /* ---------- 释放全局函数表 ---------- */
    if (code->obj_fun) {
        /* obj_fun_size 是 i32 (typedef uint32_t i32)，用 unsigned 循环更稳妥 */
        for (uint32_t fi = 0; fi < (uint32_t)code->obj_fun_size; ++fi) {
            ObjFunction *fn = &code->obj_fun[fi];
            /* 释放函数名与返回类型 */
            if (fn->name) {
                free(fn->name);
                fn->name = NULL;
            }
            if (fn->ret_type) {
                free(fn->ret_type);
                fn->ret_type = NULL;
            }
            /* 释放参数表 */
            if (fn->args) {
                for (uint32_t ai = 0; ai < (uint32_t)fn->args_size; ++ai) {
                    if (fn->args[ai].name) {
                        free(fn->args[ai].name);
                        fn->args[ai].name = NULL;
                    }
                    if (fn->args[ai].type) {
                        free(fn->args[ai].type);
                        fn->args[ai].type = NULL;
                    }
                }
                free(fn->args);
                fn->args = NULL;
                fn->args_size = 0;
            }

            if (fn->body) {
                for (uint32_t bi = 0; bi < (uint32_t)fn->body_size; ++bi) {
                    Command *cmd = &fn->body[bi];
                    if (cmd->op_value) {
                        for (uint32_t vi = 0; vi < (uint32_t)cmd->op_value_size; ++vi) {
                            ObjValue *ov = &cmd->op_value[vi];
                            if (ov->value) {
                                free(ov->value);
                                ov->value = NULL;
                            }
                            ov->size = 0;
                        }
                        free(cmd->op_value);
                        cmd->op_value = NULL;
                        cmd->op_value_size = 0;
                    }
                }
                free(fn->body);
                fn->body = NULL;
                fn->body_size = 0;
            }
        }
        free(code->obj_fun);
        code->obj_fun = NULL;
        code->obj_fun_size = 0;
    }
    /* ---------- 释放全局符号表 ---------- */
    if (code->obj_sym) {
        for (uint32_t si = 0; si < (uint32_t)code->obj_sym_size; ++si) {
            if (code->obj_sym[si].name) {
                free(code->obj_sym[si].name);
                code->obj_sym[si].name = NULL;
            }
            if (code->obj_sym[si].type) {
                free(code->obj_sym[si].type);
                code->obj_sym[si].type = NULL;
            }
        }
        free(code->obj_sym);
        code->obj_sym = NULL;
        code->obj_sym_size = 0;
    }

    /* ---------- 释放类表及类内部内容 ---------- */
    if (code->obj_class) {
        for (uint32_t ci = 0; ci < (uint32_t)code->obj_class_size; ++ci) {
            ObjClass *cls = &code->obj_class[ci];

            /* 类名 */
            if (cls->name) {
                free(cls->name);
                cls->name = NULL;
            }

            /* 成员数组（公/私/保护） */
            if (cls->pub_sym) {
                for (int m = 0; m < cls->pub_sym_size; ++m) {
                    if (cls->pub_sym[m].name) {
                        free(cls->pub_sym[m].name);
                        cls->pub_sym[m].name = NULL;
                    }
                    if (cls->pub_sym[m].type) {
                        free(cls->pub_sym[m].type);
                        cls->pub_sym[m].type = NULL;
                    }
                }
                free(cls->pub_sym);
                cls->pub_sym = NULL;
                cls->pub_sym_size = 0;
            }

            if (cls->pri_sym) {
                for (int m = 0; m < cls->pri_sym_size; ++m) {
                    if (cls->pri_sym[m].name) {
                        free(cls->pri_sym[m].name);
                        cls->pri_sym[m].name = NULL;
                    }
                    if (cls->pri_sym[m].type) {
                        free(cls->pri_sym[m].type);
                        cls->pri_sym[m].type = NULL;
                    }
                }
                free(cls->pri_sym);
                cls->pri_sym = NULL;
                cls->pri_sym_size = 0;
            }

            if (cls->pro_sym) {
                for (int m = 0; m < cls->pro_sym_size; ++m) {
                    if (cls->pro_sym[m].name) {
                        free(cls->pro_sym[m].name);
                        cls->pro_sym[m].name = NULL;
                    }
                    if (cls->pro_sym[m].type) {
                        free(cls->pro_sym[m].type);
                        cls->pro_sym[m].type = NULL;
                    }
                }
                free(cls->pro_sym);
                cls->pro_sym = NULL;
                cls->pro_sym_size = 0;
            }

            /* 类函数：pub_fun, pri_fun, pro_fun —— 释放逻辑同全局函数 */
            if (cls->pub_fun) {
                for (int f = 0; f < cls->pub_fun_size; ++f) {
                    ObjFunction *fn = &cls->pub_fun[f];
                    if (fn->name) {
                        free(fn->name);
                        fn->name = NULL;
                    }
                    if (fn->ret_type) {
                        free(fn->ret_type);
                        fn->ret_type = NULL;
                    }

                    if (fn->args) {
                        for (int a = 0; a < fn->args_size; ++a) {
                            if (fn->args[a].name) {
                                free(fn->args[a].name);
                                fn->args[a].name = NULL;
                            }
                            if (fn->args[a].type) {
                                free(fn->args[a].type);
                                fn->args[a].type = NULL;
                            }
                        }
                        free(fn->args);
                        fn->args = NULL;
                        fn->args_size = 0;
                    }

                    if (fn->body) {
                        for (int b = 0; b < fn->body_size; ++b) {
                            Command *cmd = &fn->body[b];
                            if (cmd->op_value) {
                                for (int v = 0; v < cmd->op_value_size; ++v) {
                                    if (cmd->op_value[v].value) {
                                        free(cmd->op_value[v].value);
                                        cmd->op_value[v].value = NULL;
                                    }
                                    cmd->op_value[v].size = 0;
                                }
                                free(cmd->op_value);
                                cmd->op_value = NULL;
                                cmd->op_value_size = 0;
                            }
                        }
                        free(fn->body);
                        fn->body = NULL;
                        fn->body_size = 0;
                    }
                }
                free(cls->pub_fun);
                cls->pub_fun = NULL;
                cls->pub_fun_size = 0;
            }

            if (cls->pri_fun) {
                for (int f = 0; f < cls->pri_fun_size; ++f) {
                    ObjFunction *fn = &cls->pri_fun[f];
                    if (fn->name) {
                        free(fn->name);
                        fn->name = NULL;
                    }
                    if (fn->ret_type) {
                        free(fn->ret_type);
                        fn->ret_type = NULL;
                    }

                    if (fn->args) {
                        for (int a = 0; a < fn->args_size; ++a) {
                            if (fn->args[a].name) {
                                free(fn->args[a].name);
                                fn->args[a].name = NULL;
                            }
                            if (fn->args[a].type) {
                                free(fn->args[a].type);
                                fn->args[a].type = NULL;
                            }
                        }
                        free(fn->args);
                        fn->args = NULL;
                        fn->args_size = 0;
                    }

                    if (fn->body) {
                        for (int b = 0; b < fn->body_size; ++b) {
                            Command *cmd = &fn->body[b];
                            if (cmd->op_value) {
                                for (int v = 0; v < cmd->op_value_size; ++v) {
                                    if (cmd->op_value[v].value) {
                                        free(cmd->op_value[v].value);
                                        cmd->op_value[v].value = NULL;
                                    }
                                    cmd->op_value[v].size = 0;
                                }
                                free(cmd->op_value);
                                cmd->op_value = NULL;
                                cmd->op_value_size = 0;
                            }
                        }
                        free(fn->body);
                        fn->body = NULL;
                        fn->body_size = 0;
                    }
                }
                free(cls->pri_fun);
                cls->pri_fun = NULL;
                cls->pri_fun_size = 0;
            }
            if (cls->pro_fun) {
                for (int f = 0; f < cls->pro_fun_suze; ++f) {
                    ObjFunction *fn = &cls->pro_fun[f];
                    if (fn->name) {
                        free(fn->name);
                        fn->name = NULL;
                    }
                    if (fn->ret_type) {
                        free(fn->ret_type);
                        fn->ret_type = NULL;
                    }

                    if (fn->args) {
                        for (int a = 0; a < fn->args_size; ++a) {
                            if (fn->args[a].name) {
                                free(fn->args[a].name);
                                fn->args[a].name = NULL;
                            }
                            if (fn->args[a].type) {
                                free(fn->args[a].type);
                                fn->args[a].type = NULL;
                            }
                        }
                        free(fn->args);
                        fn->args = NULL;
                        fn->args_size = 0;
                    }

                    if (fn->body) {
                        for (int b = 0; b < fn->body_size; ++b) {
                            Command *cmd = &fn->body[b];
                            if (cmd->op_value) {
                                for (int v = 0; v < cmd->op_value_size; ++v) {
                                    if (cmd->op_value[v].value) {
                                        free(cmd->op_value[v].value);
                                        cmd->op_value[v].value = NULL;
                                    }
                                    cmd->op_value[v].size = 0;
                                }
                                free(cmd->op_value);
                                cmd->op_value = NULL;
                                cmd->op_value_size = 0;
                            }
                        }
                        free(fn->body);
                        fn->body = NULL;
                        fn->body_size = 0;
                    }
                }
                free(cls->pro_fun);
                cls->pro_fun = NULL;
                cls->pro_fun_suze = 0;
            }
        }
        free(code->obj_class);
        code->obj_class = NULL;
        code->obj_class_size = 0;
    }
    code->start_fun = 0;
    /* 把剩余内存清零（确保所有计数字段为0） */
    memset(code, 0, sizeof(ObjectCode));
}
void free_local_symbol_table(CheckerSymbol* table, int size) {
    if (!table) return;
    for (int i = 0; i < size; ++i) {
        if (table[i].name) {
            free(table[i].name);
            table[i].name = NULL;
        }
        if (table[i].type) {
            free(table[i].type);
            table[i].type = NULL;
        }
    }
    free(table);
    table = NULL;
}
#endif