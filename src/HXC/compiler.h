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
} CompileErrorType;
typedef enum OPCode {   //操作码
    OP_PUT_STR,         //从栈中输出wchar_t*
    OP_DEFINE_VAR,      //定义变量,第一个操作数为变量名,第二个为类型
    OP_PUSH,
} OPCode;
typedef struct ObjValue {
    void* value;
    int size;
    enum {
        TYPE_SYM,
        TYPE_STR,
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
ObjectCode objCode = {
    .start_fun = 0,
    .obj_fun = NULL,
    .obj_sym_size = 0,
    .obj_sym = NULL,
    .obj_fun_size = 0,
};

void stringEscape(wchar* str);
bool findSymbol(wchar* name, const CheckerSymbol* table, int table_size);
void freeObjCode(void);
void compileError(CompileErrorType type, int errLine);
int ckeckMainFunction(CheckerOutput* IR);                    //检查并设置入口点
int parseEXP(Command** cmd,int* cmd_index,int* cmd_size,Token* exp, int exp_size, wchar** result_type); //分析表达式
int compile(CheckerOutput*);

int parseEXP(Command** cmd,int* cmd_index,int* cmd_size,Token* exp, int exp_size, wchar** result_type) {
    if(exp == NULL) return -1;
    if(cmd == NULL) return -1;
    if(exp_size==1) {
        if(exp->type == TOK_ID) {
            if(*cmd==NULL) {
                (*cmd) = (Command*)calloc(1, sizeof(Command));
                if(!(*cmd)) return -1;
                *cmd_size = 1;
            }
            if(*cmd_size <= *cmd_index) {
                *cmd_size = *cmd_index+1;
                void* temp = realloc(*cmd, sizeof(Command)*(*cmd_size));
                if(temp == NULL) return -1;
                *cmd = (Command*)temp;
                memset(&(*cmd)[(*cmd_index)], 0, sizeof(Command));
            }
            (*cmd)[(*cmd_index)].op = OP_PUSH;
            (*cmd)[(*cmd_index)].op_value_size = 1;
            (*cmd)[(*cmd_index)].op_value = (ObjValue*)calloc(1, sizeof(ObjValue));
            if((*cmd)[(*cmd_index)].op_value==NULL) return -1;
            (*cmd)[(*cmd_index)].op_value[0].type = TYPE_SYM;
            (*cmd)[(*cmd_index)].op_value[0].value = (wchar*)calloc(wcslen(exp->value)+1, sizeof(wchar));
            (*cmd)[(*cmd_index)].op_value[0].size = (wcslen(exp->value)+1)*sizeof(wchar);
            wcscpy((wchar*)((*cmd)[(*cmd_index)].op_value[0].value), exp->value);
            //写返回类型


            (*cmd_index)++;
            return 0;
        } else if(exp->type == TOK_VAL) {
            if(*cmd==NULL) {
                (*cmd) = (Command*)calloc(1, sizeof(Command));
                if(!(*cmd)) return -1;
                *cmd_size = 1;
            }
            if(*cmd_size <= *cmd_index) {
                *cmd_size = *cmd_index+1;
                void* temp = realloc(*cmd, sizeof(Command)*(*cmd_size));
                if(temp == NULL) return -1;
                *cmd = (Command*)temp;
                memset(&(*cmd)[(*cmd_index)], 0, sizeof(Command));
            }
            (*cmd)[(*cmd_index)].op = OP_PUSH;
            (*cmd)[(*cmd_index)].op_value_size = 1;
            (*cmd)[(*cmd_index)].op_value = (ObjValue*)calloc(1, sizeof(ObjValue));
            if((*cmd)[(*cmd_index)].op_value==NULL) return -1;
            if(exp->mark==STR) {
                (*cmd)[(*cmd_index)].op_value[0].type = TYPE_STR;
                (*cmd)[(*cmd_index)].op_value[0].value = (wchar*)calloc(wcslen(exp->value)+1, sizeof(wchar));
                (*cmd)[(*cmd_index)].op_value[0].size = (wcslen(exp->value)+1)*sizeof(wchar);
                wcscpy((wchar*)((*cmd)[(*cmd_index)].op_value[0].value), exp->value);
                stringEscape((wchar*)((*cmd)[(*cmd_index)].op_value[0].value));
#ifdef HX_DEBUG
                printf("%ls\n", (wchar*)((*cmd)[(*cmd_index)].op_value[0].value));
#endif
                (*cmd_index)++;
                return 0;
            } else {

            }
        }
    } else {

    }
    return 0;
}

/*编译：
*分析main函数,仅写入main函数内调用过的函数
*/
int compile(CheckerOutput* IR) {
    if(ckeckMainFunction(IR)) {
        return 255;
    }
    CheckerSymbol* local_sym = NULL;
    int local_sym_size = 0;
    int sym_index = 0;
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


                        parseEXP(&(objMainPtr->body), &cmd_index, &(objMainPtr->body_size), &(mainPtr->body[exp_start]), exp_end-exp_start, NULL);

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
                        printf("%ls\n", mainPtr->body[i].value);
#endif

                        if(!wcsequ(mainPtr->body[i].value, L";")&&!wcsequ(mainPtr->body[i].value, L"；")) {
                            error(ERR_NO_END, mainPtr->body[i].lin);
                            return 255;
                        }
                        cmd_index++;
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
                        local_sym[sym_index].isOnlyRead = false;
                        local_sym[sym_index].name = (wchar*)calloc(wcslen(mainPtr->body[i].value)+1, sizeof(wchar));
                        if(!(local_sym[sym_index].name)) return -1;
                        wcscpy(local_sym[sym_index].name, mainPtr->body[i].value);
                        //变量名
#ifdef HX_DEBUG
                        printf("[DEB]varName:%ls\n", local_sym[sym_index].name);
#endif
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
                        //类型
#ifdef HX_DEBUG
                        printf("[DEB]varType:%ls\n", local_sym[sym_index].type);
#endif
                        if(!(i+1 < mainPtr->body_size)) {
                            error(ERR_NO_END, mainPtr->body[i].lin);
                            return 255;
                        }
                        i++;
                        if(wcsequ(mainPtr->body[i].value, L";")||wcsequ(mainPtr->body[i].value, L"；")) {
                            sym_index++;
                            break;
                        } else {


                            sym_index++;
                        }
                    }
                }
                break;
                }
            }
        }
    }
    return 0;
}
bool findSymbol(wchar* name, const CheckerSymbol* table, int table_size) {
    if(name == NULL) return false;
    if(table==NULL) {
        if(checkerOutput.global_sym == NULL) {
            return false;
        } else {
            for(int i = 0; i < checkerOutput.global_sym_size; i++) {
                if(wcsequ(name, checkerOutput.global_sym[i].name)) return true;
            }
        }
        return false;
    }
    for(int i = 0; i < table_size; i++) {
        if(wcsequ(name, table[i].name)) return true;
    }
    if(checkerOutput.global_sym == NULL) {
        return false;
    } else {
        for(int i = 0; i < checkerOutput.global_sym_size; i++) {
            if(wcsequ(name, checkerOutput.global_sym[i].name)) return true;
        }
    }
    return false;
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
#endif