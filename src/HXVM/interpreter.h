#ifndef HXVM_INTERPRETER_H
#define HXVM_INTERPRETER_H
#include "hxLocale.h"
#include "hsmLoader.h"
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include "HXVM.h"
#ifdef HX_DEBUG
long int interprete_commands_size = 0;
#endif

int interprete();
int interprete() {
    initLocale();
    if(vm.stackFrame[vm.top_StackFrame-1].func == NULL) {
        return -1;
    }
    StackFrame* ptr = &(vm.stackFrame[vm.top_StackFrame-1]);
    for(int i = 0; i < ptr->func->body_size; i++) {
#ifdef HX_DEBUG
        interprete_commands_size++;
#endif
        switch(ptr->func->body[i].op) {
        case OP_PUSH: {
#ifdef SHOW_HX_DEBUG_DETAIL
            wprintf(L"\33[33m[DEG]\33[0m解释OP_PUSH...\n");
#endif
            if(ptr->func->body[i].op_value == NULL) {
                HXVMError(ERR_NULL_PTR);
                return -1;
            }
            StackType op_value = {0};
            op_value.value = ptr->func->body[i].op_value[0].value;
            op_value.type = ptr->func->body[i].op_value[0].type;
            pushValueIntoStack(&op_value);
        }
        break;

        case OP_PUT_STR: {
#ifdef SHOW_HX_DEBUG_DETAIL
            wprintf(L"\33[33m[DEG]\33[0m解释OP_PUT_STR...\n");
#endif
            popValueOutOfStack();
            if(vm.stack[vm.top_stack].value) {
                wprintf(L"%ls", (wchar*)vm.stack[vm.top_stack].value);
            } else {
                wprintf(L"%ls",PUT_STRING_DISPLAY_NULL? L"（空）":L"\0");
            }
        }
        break;
        }
    }
    return 0;
}
#endif