#ifndef HXVM_INTERPRETER_H
#define HXVM_INTERPRETER_H
#include "hxLocale.h"
#include "hsmLoader.h"
#include "hxSymbolTable.h"
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
            vm.top_stack--;
            if(vm.stack[vm.top_stack].value) {
                wprintf(L"%ls", (wchar*)vm.stack[vm.top_stack].value);
            } else {
                wprintf(L"%ls",PUT_STRING_DISPLAY_NULL? L"（空）":L"\0");
            }
            vm.top_stack++;
            popValueOutOfStack();
        }
        break;

        case OP_DEFINE_VAR: {
#ifdef SHOW_HX_DEBUG_DETAIL
            wprintf(L"\33[33m[DEG]\33[0m解释OP_DEFINE_VAR...\n");
#endif
            if(ptr->func->body[i].op_value == NULL) {
                HXVMError(ERR_NULL_PTR);
                return -1;
            }
            if(ptr->func->body[i].op_value_size != 2) {
                HXVMError(ERR_NULL_PTR);
                return -1;
            }
            if(ptr->func->body[i].op_value[0].value == NULL || ptr->func->body[i].op_value[1].value == NULL) {
                HXVMError(ERR_NULL_PTR);
                return -1;
            }

            Symbol sym = {0};
            sym.name = (wchar*)(ptr->func->body[i].op_value[0].value);
            sym.type = (wchar*)(ptr->func->body[i].op_value[1].value);
            int err = insert(&sym, &(vm.stackFrame[vm.top_StackFrame-1].localeSymbolTable));
            if(err) {
                wprintf(L"\33[31m[E]当符号插入表中时发生了哈希冲突！\33[0m\n");
                return err;
            }
        }
        break;
        }
    }
    return 0;
}
#endif