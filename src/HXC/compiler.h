#ifndef HXLANG_COMPILER_H
#define HXLANG_COMPILER_H
#include <stdlib.h>
#include <stdbool.h>
#include <wchar.h>
#include "lexer.h"
#include "output.h"
#include "theFirstPass.h"

typedef enum CompileErrorType {
    ERR_NO_FUN_MAIN,
} CompileErrorType;
typedef enum OPCode {   //操作码
    OP_PUT_WCS,         //输出wchar_t*
    OP_PUT_CS,          //输出char*
} OPCode;
typedef struct ObjToken {
    union {
        wchar* wcs_val;
        char*  cs_val;
    } value;
    enum {
        TYPE_WCS,
        TYPE_CS,
    } type;
} ObjToken;
typedef struct Command {
    OPCode op;
    ObjToken* op_value;
    int op_value_size;
} Command;

typedef struct ObjSymbol {
    wchar* name;
    wchar* type;
    bool isOnlyRead;
} ObjSymbol;
typedef struct ObjectCode {
    ObjSymbol* obj_sym;
    int obj_sym_size;
} ObjectCode;
ObjectCode objCode = {0};
void freeObjCode(void);
void compileError(CompileErrorType type, int errLine);
int compile(CheckerOutput* IR) {
    if(IR==NULL) {
        compileError(ERR_NO_FUN_MAIN, 0);
        return 255;
    }

}
void freeObjCode(void) {
    if(objCode.obj_sym) {
        for(int i = 0; i < objCode.obj_sym_size; i++) {
            if(objCode.obj_sym[i].name) free(objCode.obj_sym[i].name);
            if(objCode.obj_sym[i].type) free(objCode.obj_sym[i].type);
        }
        free(objCode.obj_sym);
    }
    return;
}
void compileError(CompileErrorType type, int errLine) {
    initLocale();
    switch(type) {
        case ERR_NO_FUN_MAIN: {
            fwprintf(stderr, L"\33[31m[E]缺少主函数！\33[0m\n");
        } break;
    }
    return;
}
#endif