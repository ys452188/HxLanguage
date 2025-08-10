#ifndef HXVM_INTERPRETER_H
#define HXVM_INTERPRETER_H
#include "hxLocale.h"
#include "hsmLoader.h"
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include "HXVM.h"
int interprete();
int interprete() {
    if(vm.stackFrame[vm.top_StackFrame-1].func == NULL) {
        return -1;
    }
    StackFrame* ptr = &(vm.stackFrame[vm.top_StackFrame-1]);
    for(int i = 0; i < ptr->func->body_size; i++) {
        switch(ptr->func->body[i].op) {
        case OP_PUT_STR: {
            if(ptr->func->body[i].op_value == NULL) {
                HXVMError(ERR_NULL_PTR);
                return -1;
            }
            if(ptr->func->body[i].op_value[0].type!=TYPE_STR) {
                HXVMError(ERR_NULL_PTR);
                return -1;
            }
            if(ptr->func->body[i].op_value[0].value.ptr_val == NULL) {
                HXVMError(ERR_NULL_PTR);
                return -1;
            }
            wprintf(L"%ls", (wchar*)(ptr->func->body[i].op_value[0].value.ptr_val));
        }
        break;
        }
    }
    return 0;
}
#endif