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
} CompileErrorType;
typedef enum OPCode {   //操作码
    OP_PUT_WCS,         //输出wchar_t*
    OP_PUT_CS,          //输出char*
} OPCode;
typedef struct ObjValue {
    union {
        void* ptr_val;
    } value;
    enum {
        TYPE_WCS,
        TYPE_CS,
    } type;
} ObjValue;
typedef struct Command {
    OPCode op;
    ObjValue* op_value;
    i32 op_value_size;
} Command;

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
typedef struct ObjectCode {
    i32 start_fun;    //入口点,main函数的索引
    ObjFunction* obj_fun;
    i32 obj_fun_size;
    ObjSymbol* obj_sym;
    i32 obj_sym_size;
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

void freeObjCode(void);
void compileError(CompileErrorType type, int errLine);
int ckeckMainFunction(CheckerOutput* IR);   //检查并设置入口点
int compile(CheckerOutput*);

/*编译：
*分析main函数,仅写入main函数内调用过的函数
*/
int compile(CheckerOutput* IR) {
    if(ckeckMainFunction(IR)) {
        return 255;
    }
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

            for(int i = 0; i < mainPtr->body_size; i++) {
                switch(mainPtr->body[i].type) {
                case TOK_ID: {
                    if(wcsequ(mainPtr->body[i].value, L"putString") || wcsequ(mainPtr->body[i].value, L"输出字符串")) {
                        if(cmd_index >= objMainPtr->body_size) {
                            objMainPtr->body_size = cmd_index+1;
                            void* temp = realloc(objMainPtr->body, sizeof(Command)*(mainPtr->body_size));
                            if(!temp) return -1;
                            objMainPtr->body = (Command*)temp;
                            objMainPtr->body[cmd_index].op_value = NULL;
                            objMainPtr->body[cmd_index].op_value_size = 0;
                        }
                        objMainPtr->body[cmd_index].op_value_size = 1;
                        objMainPtr->body[cmd_index].op = OP_PUT_CS;
                        objMainPtr->body[cmd_index].op_value = (ObjValue*)calloc(1, sizeof(ObjValue));
                        objMainPtr->body[cmd_index].op_value[0].type = TYPE_WCS;
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
                        int arg_value = i;
                        int count = 0;
                        while(i < mainPtr->body_size-1) {
                            if(wcsequ(mainPtr->body[i].value, L")")||wcsequ(mainPtr->body[i].value, L"）")) {
                                break;
                            }
                            count++;
                            i++;
                        }
                        if((!wcsequ(mainPtr->body[i].value, L")"))&&(!wcsequ(mainPtr->body[i].value, L"）"))) {
                            compileError(ERR_QUITE_NOT_CORRECTLY_CLOSE, mainPtr->body[i].lin);
                            return 255;
                        }
                        if(count == 0) {
                            compileError(ERR_FUN_ARGS_SIZE, mainPtr->body[i].lin);
                            return 255;
                        } else if(count == 1) {
                            //表面值作参数
                            if(mainPtr->body[arg_value].type == TOK_VAL) {
                                objMainPtr->body[cmd_index].op_value[0].value.ptr_val = calloc(wcslen(mainPtr->body[arg_value].value)+1, sizeof(wchar));
                                if(!(objMainPtr->body[cmd_index].op_value[0].value.ptr_val)) return -1;
                                wcscpy((wchar*)(objMainPtr->body[cmd_index].op_value[0].value.ptr_val), mainPtr->body[arg_value].value);
                                //printf("%ls\n", (wchar*)(objMainPtr->body[cmd_index].op_value[0].value.ptr_val));
                            }
                        } else {

                        }
                        cmd_index++;
                    }
                }
                }
            }
        }
    }
    return 0;
}

/* 彻底释放 ObjectCode 的实现：释放所有动态分配的子成员 */
void freeObjectCode(ObjectCode *c) {
    if (!c) return;

    if (c->obj_fun) {
        for (int i = 0; i < c->obj_fun_size; ++i) {
            ObjFunction *f = &c->obj_fun[i];
            if (f->name) {
                free(f->name);
                f->name = NULL;
            }
            if (f->ret_type) {
                free(f->ret_type);
                f->ret_type = NULL;
            }

            if (f->args) {
                for (int j = 0; j < f->args_size; ++j) {
                    if (f->args[j].name) {
                        free(f->args[j].name);
                        f->args[j].name = NULL;
                    }
                    if (f->args[j].type) {
                        free(f->args[j].type);
                        f->args[j].type = NULL;
                    }
                }
                free(f->args);
                f->args = NULL;
                f->args_size = 0;
            }

            if (f->body) {
                for (int k = 0; k < f->body_size; ++k) {
                    Command *cmd = &f->body[k];
                    if (cmd->op_value) {
                        for (int v = 0; v < cmd->op_value_size; ++v) {
                            ObjValue *ov = &cmd->op_value[v];
                            if (ov->type == TYPE_WCS) {
                                if (ov->value.ptr_val) {
                                    free(ov->value.ptr_val);
                                    ov->value.ptr_val = NULL;
                                }
                            } else if (ov->type == TYPE_CS) {
                                if (ov->value.ptr_val) {
                                    free(ov->value.ptr_val);
                                    ov->value.ptr_val = NULL;
                                }
                            }
                        }
                        free(cmd->op_value);
                        cmd->op_value = NULL;
                        cmd->op_value_size = 0;
                    }
                }
                free(f->body);
                f->body = NULL;
                f->body_size = 0;
            }
        }
        free(c->obj_fun);
        c->obj_fun = NULL;
        c->obj_fun_size = 0;
    }

    if (c->obj_sym) {
        for (int i = 0; i < c->obj_sym_size; ++i) {
            if (c->obj_sym[i].name) {
                free(c->obj_sym[i].name);
                c->obj_sym[i].name = NULL;
            }
            if (c->obj_sym[i].type) {
                free(c->obj_sym[i].type);
                c->obj_sym[i].type = NULL;
            }
        }
        free(c->obj_sym);
        c->obj_sym = NULL;
        c->obj_sym_size = 0;
    }

    c->start_fun = 0;
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
    }
    return;
}
#endif