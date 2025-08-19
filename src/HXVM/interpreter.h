#ifndef HXVM_INTERPRETER_H
#define HXVM_INTERPRETER_H
#include "hxLocale.h"
#include "hsmLoader.h"
#include "hxSymbolTable.h"
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "HXVM.h"
#ifdef HX_DEBUG
long int interprete_commands_size = 0;
#endif

int interprete_OP_ADD();
int interprete();

int interprete() {
    initLocale();
    if(vm.stackFrame[vm.top_StackFrame-1].func == NULL) {
        return -1;
    }
    StackFrame* ptr = &(vm.stackFrame[vm.top_StackFrame-1]);
    //printf("size = %d\n", ptr->func->body_size);
    for(int i = 0; i < ptr->func->body_size; i++) {
#ifdef HX_DEBUG
        interprete_commands_size++;
#endif
        switch(ptr->func->body[i].op) {
        //解释OP_PUSH
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
            op_value.isAlloc = false;
            int err = pushValueIntoStack(&op_value);
            if(err) return err;
        }
        break;
        //解释 OP_PUT_STR
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
            sym.name = (wchar*)(wchar*)(ptr->func->body[i].op_value[0].value);
            sym.type = (wchar*)(ptr->func->body[i].op_value[1].value);
            int err = insert(&sym, &(vm.stackFrame[vm.top_StackFrame-1].localeSymbolTable));
            if(err) {
                wprintf(L"\33[31m[E]当符号插入表中时发生了哈希冲突！\33[0m\n");
                return err;
            }
        }
        break;

        case OP_ADD: {
            int err = interprete_OP_ADD();
            if(err) return err;
        }
        break;

        default: {
            wprintf(L"\33[31m[E]未知操作码%d！\33[0m\n", ptr->func->body[i].op);
            return 255;
        }
        }
    }
    return 0;
}
int interprete_OP_ADD() {
#ifdef SHOW_HX_DEBUG_DETAIL
    wprintf(L"\33[33m[DEG]\33[0m解释OP_ADD...\n");
#endif
    if(vm.top_stack<2) {
        HXVMError(ERR_NULL_PTR);
        return -1;
    }
    vm.top_stack--;       //此时指向第二个操作数
    if(vm.stack[vm.top_stack].type == STR|| vm.stack[vm.top_stack-1].type == STR) {
        wchar* val2 = NULL;
        bool isVal2Alloc = false;
        if(!(vm.stack[vm.top_stack].value)) {
            val2 = (wchar*)calloc(1, sizeof(wchar));
            isVal2Alloc = true;
        } else if(vm.stack[vm.top_stack].type != STR) {
            val2 = (wchar*)calloc(1, sizeof(wchar));
            isVal2Alloc = true;
            switch(vm.stack[vm.top_stack].type) {
            case INT: {
                *val2 = (wchar)(uint16_t)(*((long int*)vm.stack[vm.top_stack].value));
            }
            break;
            case FLOAT: {
                *val2 = (wchar)(uint16_t)(*((float*)vm.stack[vm.top_stack].value));
            }
            break;
            case DOUBLE: {
                *val2 = (wchar)(uint16_t)(*((double*)vm.stack[vm.top_stack].value));
            }
            break;
            case CHAR: {
                *val2 = (wchar)(*((wchar*)vm.stack[vm.top_stack].value));
            }
            break;
            }
        } else {
            val2 = vm.stack[vm.top_stack].value;
        }

        vm.top_stack--;    //此时指向第一个操作数
        bool isVal1Alloc = false;
        wchar* val1 = NULL;
        if(!(vm.stack[vm.top_stack].value)) {
            val1 = (wchar*)calloc(1, sizeof(wchar));
            isVal1Alloc = true;
        } else if(vm.stack[vm.top_stack].type != STR) {
            val1 = (wchar*)calloc(1, sizeof(wchar));
            isVal1Alloc = true;
            switch(vm.stack[vm.top_stack].type) {
            case INT: {
                *val1 = (wchar)(uint16_t)(*((long int*)vm.stack[vm.top_stack].value));
            }
            break;
            case FLOAT: {
                *val1 = (wchar)(uint16_t)(*((float*)vm.stack[vm.top_stack].value));
            }
            break;
            case DOUBLE: {
                *val1 = (wchar)(uint16_t)(*((double*)vm.stack[vm.top_stack].value));
            }
            break;
            case CHAR: {
                *val1 = (wchar)(*((wchar*)vm.stack[vm.top_stack].value));
            }
            break;
            }
        } else {
            val1 = vm.stack[vm.top_stack].value;
        }
        wchar* temp = (wchar*)calloc(wcslen(vm.stack[vm.top_stack].value)+wcslen(vm.stack[vm.top_stack+1].value)+1, sizeof(wchar));
        if(!temp) return -1;
        //连接字符串
        wcscpy(temp, val1);
        wcscat(temp, val2);

        if(isVal2Alloc) {
            free(val2);
            val2 = NULL;
        }
        if(isVal1Alloc) {
            free(val1);
            val1 = NULL;
        }
        vm.stack[vm.top_stack+1].value = NULL;
        vm.stack[vm.top_stack].value = temp;
        vm.stack[vm.top_stack].isAlloc = true;
        vm.stack[vm.top_stack].type = STR;
        vm.top_stack++;
    } else if(vm.stack[vm.top_stack].type == DOUBLE|| vm.stack[vm.top_stack-1].type == DOUBLE) {
        //操作数2
        double* val2 = NULL;
        bool isVal2Alloc = false;
        val2 = (double*)calloc(1, sizeof(double));
        if(!val2) return -1;
        isVal2Alloc = true;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            *val2 = (double)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            *val2 = (double)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;

        case FLOAT: {
            *val2 = (double)(*((float*)vm.stack[vm.top_stack].value));
        }
        break;

        case DOUBLE: {
            *val2 = *((double*)vm.stack[vm.top_stack].value);
        }
        break;
        }
        //操作数1
        vm.top_stack--;    //此时指向第一个操作数
        bool isVal1Alloc = false;
        double* val1 = (double*)calloc(1, sizeof(double));
        if(!val1) return -1;
        isVal1Alloc = true;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            *val1 = (double)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            *val1 = (double)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;

        case FLOAT: {
            *val1 = (double)(*((float*)vm.stack[vm.top_stack].value));
        }
        break;
        case DOUBLE: {
            *val1 = *((double*)vm.stack[vm.top_stack].value);
        }
        break;
        }

        double* temp = (double*)calloc(1, sizeof(double));
        if(!temp) return -1;

        *temp = *val1+*val2;

#ifdef SHOW_HX_DEBUG_DETAIL
        wprintf(L"\33[33m[DEG]\33[0m \33[31mADD\33[0m {v1:%lf, v2:%lf}-> %lf\n",*val1, *val2, *temp);
#endif
        if(isVal2Alloc) {
            free(val2);
            val2 = NULL;
        }
        if(isVal1Alloc) {
            free(val1);
            val1 = NULL;
        }
        vm.stack[vm.top_stack+1].value = NULL;
        vm.stack[vm.top_stack].value = temp;
        vm.stack[vm.top_stack].isAlloc = true;
        vm.stack[vm.top_stack].type = DOUBLE;
        vm.top_stack++;
    } else if(vm.stack[vm.top_stack].type == FLOAT|| vm.stack[vm.top_stack-1].type == FLOAT) {

    } else if(vm.stack[vm.top_stack].type == INT|| vm.stack[vm.top_stack-1].type == INT) {
        //操作数2
        long int* val2 = NULL;
        bool isVal2Alloc = false;
        val2 = (long int*)calloc(1, sizeof(long int));
        if(!val2) return -1;
        isVal2Alloc = true;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            *val2 = (long int)(uint16_t)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            *val2 = (long int)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;
        }
        //操作数1
        vm.top_stack--;    //此时指向第一个操作数
        bool isVal1Alloc = false;
        long int* val1 = (long int*)calloc(1, sizeof(long int));
        if(!val1) return -1;
        isVal1Alloc = true;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            *val1 = (long int)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            *val1 = (long int)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;
        }

        long int* temp = (long int*)calloc(1, sizeof(long int));
        if(!temp) return -1;

        *temp = *val1+*val2;

#ifdef SHOW_HX_DEBUG_DETAIL
        wprintf(L"\33[33m[DEG]\33[0m \33[31mADD\33[0m {v1:%ld, v2:%ld}-> %ld\n",*val1, *val2, *temp);
#endif
        if(isVal2Alloc) {
            free(val2);
            val2 = NULL;
        }
        if(isVal1Alloc) {
            free(val1);
            val1 = NULL;
        }
        vm.stack[vm.top_stack+1].value = NULL;
        vm.stack[vm.top_stack].value = temp;
        vm.stack[vm.top_stack].isAlloc = true;
        vm.stack[vm.top_stack].type = INT;
        vm.top_stack++;
    } else if(vm.stack[vm.top_stack].type == CHAR|| vm.stack[vm.top_stack-1].type == CHAR) {

    }
    return 0;
}
#endif