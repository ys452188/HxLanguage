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
int interprete_OP_MUL();
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
            wprintf(L"\33[33m[DEG]\33[0m解释\33[36mOP_PUSH\33[0m...\n");
#endif
            if(ptr->func->body[i].op_value == NULL) {
                HXVMError(ERR_NULL_PTR);
                return -1;
            }
            StackType op_value = {0};
            if(ptr->func->body[i].op_value[0].type == TYPE_SYM) {
                //printf("dddfffffffg\n");
                int index = findSymbol(ptr->func->body[i].op_value[0].value, &(ptr->localeSymbolTable));
                if(index==-1) {
                    HXVMError(ERR_NULL_PTR);
                    return -1;
                }
                op_value.value = ptr->localeSymbolTable.symbol[index].address;
                op_value.isAlloc = false;
                if(wcsequ(ptr->localeSymbolTable.symbol[index].type, L"整型") || wcsequ(ptr->localeSymbolTable.symbol[index].type, L"int")) {
                    op_value.type = INT;
                } else if(wcsequ(ptr->localeSymbolTable.symbol[index].type, L"浮点型") || wcsequ(ptr->localeSymbolTable.symbol[index].type, L"float")) {
                    op_value.type = FLOAT;
                } else if(wcsequ(ptr->localeSymbolTable.symbol[index].type, L"字符串型") || wcsequ(ptr->localeSymbolTable.symbol[index].type, L"str")) {
                    op_value.type = STR;
                } else if(wcsequ(ptr->localeSymbolTable.symbol[index].type, L"字符型") || wcsequ(ptr->localeSymbolTable.symbol[index].type, L"char")) {
                    op_value.type = CHAR;
                } else if(wcsequ(ptr->localeSymbolTable.symbol[index].type, L"精确浮点型") || wcsequ(ptr->localeSymbolTable.symbol[index].type, L"double")) {
                    op_value.type = DOUBLE;
                } else {
                    op_value.type = UNKNOWN;
                }
            } else {
                op_value.value = ptr->func->body[i].op_value[0].value;
                op_value.type = ptr->func->body[i].op_value[0].type;
                op_value.isAlloc = false;
            }
            int err = pushValueIntoStack(&op_value);
            if(err) return err;
            //printf("eeeee\n");
        }
        break;
        //解释 OP_PUT_STR
        case OP_PUT_STR: {
#ifdef SHOW_HX_DEBUG_DETAIL
            wprintf(L"\33[33m[DEG]\33[0m解释\33[36mOP_PUT_STR\33[0m...\n");
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

        case OP_CALL: {    //调用函数
#ifdef SHOW_HX_DEBUG_DETAIL
            wprintf(L"\33[33m[DEG]\33[0m解释\33[36mOP_CALL\33[0m...\n");
#endif
            if(ptr->func->body[i].op_value == NULL) {
                HXVMError(ERR_NULL_PTR);
                return -1;
            }
            if(ptr->func->body[i].op_value_size < 1) {
                HXVMError(ERR_NULL_PTR);
                return -1;
            }
            wchar* funName = ptr->func->body[i].op_value[0].value;
            if(funName==NULL) {
                HXVMError(ERR_NULL_PTR);
                return -1;
            }
            ObjFunction* funPtr = NULL;
            if(ptr->func->body[i].op_value_size==1) {   //func(void)
#ifdef SHOW_HX_DEBUG_DETAIL
                wprintf(L"\33[33m[NOTE]以下是对函数\33[36m%ls(void)\33[33m的解释。\33[0m\n", funName);
#endif
                //查找
                for(int j=0; j < hsmCode.obj_fun_size; j++) {
                    if(wcsequ(hsmCode.obj_fun[j].name, funName)&&(hsmCode.obj_fun[j].args==NULL||hsmCode.obj_fun[j].args_size==0)) {
                        funPtr = &(hsmCode.obj_fun[j]);
                        break;
                    }
                }
                if(funPtr == NULL) {
                    HXVMError(ERR_NULL_PTR);
                    return -1;
                }
                int err = 0;
                err = pushFunIntoStackFrame(funPtr);  //函数入栈
                if(err) return err;
                err = interprete();      //解释
                if(err) return err;
#ifdef SHOW_HX_DEBUG_DETAIL
                wprintf(L"\33[33m[NOTE]对函数\33[36m%ls(void)\33[33m的解释结束。\33[0m\n", funName);
#endif
                popFunOutOfStackFrame();               //函数出栈
            }
        }
        break;

        case OP_DEFINE_VAR: {
#ifdef SHOW_HX_DEBUG_DETAIL
            wprintf(L"\33[33m[DEG]\33[0m解释\33[36mOP_DEFINE_VAR\33[0m...\n");
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
            //复制name
            sym.name = (wchar*)calloc(wcslen((wchar*)(ptr->func->body[i].op_value[0].value))+1, sizeof(wchar));
            if(sym.name == NULL) return -1;
            wcscpy(sym.name, (wchar*)(ptr->func->body[i].op_value[0].value));
            //复制type
            sym.type = (wchar*)calloc(wcslen((wchar*)(ptr->func->body[i].op_value[1].value))+1, sizeof(wchar));
            if(sym.type == NULL) return -1;
            wcscpy(sym.type, (wchar*)(ptr->func->body[i].op_value[1].value));
            //设置address
            if(wcsequ(sym.type, L"int")||wcsequ(sym.type, L"整型")) {
                sym.address = calloc(1, sizeof(long int));
                if(!(sym.address)) return -1;
                sym.isAllocHeap = true;
            } else if(wcsequ(sym.type, L"float")||wcsequ(sym.type, L"浮点型")) {
                sym.address = calloc(1, sizeof(float));
                if(!(sym.address)) return -1;
                sym.isAllocHeap = true;
            } else if(wcsequ(sym.type, L"double")||wcsequ(sym.type, L"精确浮点型")) {
                sym.address = calloc(1, sizeof(double));
                if(!(sym.address)) return -1;
                sym.isAllocHeap = true;
            } else if(wcsequ(sym.type, L"char")||wcsequ(sym.type, L"字符型")) {
                sym.address = calloc(1, sizeof(wchar));
                if(!(sym.address)) return -1;
                sym.isAllocHeap = true;
            } else if(wcsequ(sym.type, L"str")||wcsequ(sym.type, L"字符串型")) {
                sym.address = calloc(1, sizeof(wchar));
                if(!(sym.address)) return -1;
                sym.isAllocHeap = true;
            } else {     //类类型

            }
            //插
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

        case OP_MUL: {
            int err = interprete_OP_MUL();
            if(err) return err;
        }
        break;

        case OP_MOVE: {
#ifdef SHOW_HX_DEBUG_DETAIL
            wprintf(L"\33[33m[DEG]\33[0m解释\33[36mOP_MOVE\33[0m...\n");
#endif
            //检查操作数
            if(ptr->func->body[i].op_value_size == 0 || ptr->func->body[i].op_value == NULL) {
                HXVMError(ERR_NULL_PTR);
                return -1;
            }
            if(vm.top_stack-1 < 0) {    //检查栈中是否有元素
                HXVMError(ERR_NULL_PTR);
                return -1;
            }
            vm.top_stack--;
            int index = findSymbol(ptr->func->body[i].op_value->value, &ptr->localeSymbolTable);
            if(index == -1) {
                HXVMError(ERR_NULL_PTR);
                return -1;
            }
            if(!(ptr->localeSymbolTable.symbol[index].isAllocHeap)) {
                ptr->localeSymbolTable.symbol[index].address = vm.stack[vm.top_stack].value;
            } else {
                if(wcsequ(ptr->localeSymbolTable.symbol[index].type, L"int")||wcsequ(ptr->localeSymbolTable.symbol[index].type, L"整型")) {
                    *((long int*)ptr->localeSymbolTable.symbol[index].address) = *((long int*)vm.stack[vm.top_stack].value);
                } else if(wcsequ(ptr->localeSymbolTable.symbol[index].type, L"float")||wcsequ(ptr->localeSymbolTable.symbol[index].type, L"浮点型")) {
                    *((float*)ptr->localeSymbolTable.symbol[index].address) = *((float*)vm.stack[vm.top_stack].value);
                } else if(wcsequ(ptr->localeSymbolTable.symbol[index].type, L"double")||wcsequ(ptr->localeSymbolTable.symbol[index].type, L"精确浮点型")) {
                    *((double*)ptr->localeSymbolTable.symbol[index].address) = *((double*)vm.stack[vm.top_stack].value);
                } else if(wcsequ(ptr->localeSymbolTable.symbol[index].type, L"char")||wcsequ(ptr->localeSymbolTable.symbol[index].type, L"字符型")) {
                    *((wchar*)ptr->localeSymbolTable.symbol[index].address) = *((wchar*)vm.stack[vm.top_stack].value);
                } else if(wcsequ(ptr->localeSymbolTable.symbol[index].type, L"str")||wcsequ(ptr->localeSymbolTable.symbol[index].type, L"字符串型")) {
                    free(ptr->localeSymbolTable.symbol[index].address);
                    ptr->localeSymbolTable.symbol[index].address = calloc(wcslen(((wchar*)vm.stack[vm.top_stack].value))+1, sizeof(wchar));
                    if(!(ptr->localeSymbolTable.symbol[index].address)) return -1;
                    wcscpy((wchar*)ptr->localeSymbolTable.symbol[index].address, ((wchar*)vm.stack[vm.top_stack].value));
                } else {     //类类型

                }
            }
            //wprintf(L"%ld\n", *(long int*)(ptr->func->body[i].op_value->value));
            if(vm.stack[vm.top_stack].isAlloc) free(vm.stack[vm.top_stack].value);
            vm.stack[vm.top_stack].value = NULL;
        }
        break;

        default: {
            wprintf(L"\33[31m[E]未知操作码%d！\33[0m\n", ptr->func->body[i].op);
            return 255;
        }
        break;
        }
    }
    return 0;
}

int interprete_OP_ADD() {
#ifdef SHOW_HX_DEBUG_DETAIL
    wprintf(L"\33[33m[DEG]\33[0m解释\33[36mOP_ADD\33[0m...\n");
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
            val2 = (wchar*)calloc(2, sizeof(wchar));
            if(!val2) return -1;
            isVal2Alloc = true;
            switch(vm.stack[vm.top_stack].type) {
            case INT: {
                //printf("182 %ld\n", (*((long int*)vm.stack[vm.top_stack].value)));
                val2[0] = (wchar)(uint16_t)(*((long int*)vm.stack[vm.top_stack].value));
            }
            break;
            case FLOAT: {
                val2[0] = (wchar)(uint16_t)(*((float*)vm.stack[vm.top_stack].value));
            }
            break;
            case DOUBLE: {
                val2[0] = (wchar)(uint16_t)(*((double*)vm.stack[vm.top_stack].value));
            }
            break;
            case CHAR: {
                val2[0] = (wchar)(*((wchar*)vm.stack[vm.top_stack].value));
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
            val1 = (wchar*)calloc(2, sizeof(wchar));
            isVal1Alloc = true;
            switch(vm.stack[vm.top_stack].type) {
            case INT: {
                val1[0] = (wchar)(uint16_t)(*((long int*)vm.stack[vm.top_stack].value));
            }
            break;
            case FLOAT: {
                val1[0] = (wchar)(uint16_t)(*((float*)vm.stack[vm.top_stack].value));
            }
            break;
            case DOUBLE: {
                val1[0] = (wchar)(uint16_t)(*((double*)vm.stack[vm.top_stack].value));
            }
            break;
            case CHAR: {
                val1[0] = (wchar)(*((wchar*)vm.stack[vm.top_stack].value));
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

        if(vm.stack[vm.top_stack+1].isAlloc) {
            free(vm.stack[vm.top_stack+1].value);
            vm.stack[vm.top_stack+1].isAlloc = false;
        }
        vm.stack[vm.top_stack+1].value = NULL;
        if(vm.stack[vm.top_stack].isAlloc) {
            free(vm.stack[vm.top_stack].value);
            vm.stack[vm.top_stack].isAlloc = false;
        }
        vm.stack[vm.top_stack].value = temp;
        vm.stack[vm.top_stack].isAlloc = true;
        vm.stack[vm.top_stack].type = STR;
        vm.top_stack++;
    } else if(vm.stack[vm.top_stack].type == DOUBLE|| vm.stack[vm.top_stack-1].type == DOUBLE) {
        //操作数2
        double val2 = 0.0;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            val2 = (double)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            val2 = (double)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;

        case FLOAT: {
            val2 = (double)(*((float*)vm.stack[vm.top_stack].value));
        }
        break;

        case DOUBLE: {
            val2 = *((double*)vm.stack[vm.top_stack].value);
        }
        break;
        }
        //操作数1
        vm.top_stack--;    //此时指向第一个操作数
        double val1 = 0.0;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            val1 = (double)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            val1 = (double)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;

        case FLOAT: {
            val1 = (double)(*((float*)vm.stack[vm.top_stack].value));
        }
        break;
        case DOUBLE: {
            val1 = *((double*)vm.stack[vm.top_stack].value);
        }
        break;
        }

        double* temp = (double*)calloc(1, sizeof(double));   //这里必须用堆内存
        if(!temp) return -1;
        *temp = val1+val2;

#ifdef SHOW_HX_DEBUG_DETAIL
        wprintf(L"\33[33m[DEG]\33[0m \33[31mADD\33[0m {v1:%lf, v2:%lf}-> %lf\n",val1, val2, *temp);
#endif
        if(vm.stack[vm.top_stack+1].isAlloc) {
            free(vm.stack[vm.top_stack+1].value);
            vm.stack[vm.top_stack+1].isAlloc = false;
        }
        vm.stack[vm.top_stack+1].value = NULL;
        if(vm.stack[vm.top_stack].isAlloc) {
            free(vm.stack[vm.top_stack].value);
            vm.stack[vm.top_stack].isAlloc = false;
        }
        vm.stack[vm.top_stack].value = temp;
        vm.stack[vm.top_stack].isAlloc = true;
        vm.stack[vm.top_stack].type = DOUBLE;
        vm.top_stack++;
    } else if(vm.stack[vm.top_stack].type == FLOAT|| vm.stack[vm.top_stack-1].type == FLOAT) {
        float val2 = 0.0f;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            val2 = (float)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            val2 = (float)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;

        case FLOAT: {
            val2 = (*((float*)vm.stack[vm.top_stack].value));
        }
        break;
        }

        //操作数1
        vm.top_stack--;    //此时指向第一个操作数
        float val1 = 0.0f;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            val1 = (float)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            val1 = (float)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;

        case FLOAT: {
            val1 = (*((float*)vm.stack[vm.top_stack].value));
        }
        break;
        }

        float* temp = (float*)calloc(1, sizeof(float));
        if(!temp) return -1;
        *temp = val1+ val2;

#ifdef SHOW_HX_DEBUG_DETAIL
        wprintf(L"\33[33m[DEG]\33[0m \33[31mADD\33[0m {v1:%f, v2:%f}-> %f\n",val1, val2, *temp);
#endif

        if(vm.stack[vm.top_stack+1].isAlloc) {
            free(vm.stack[vm.top_stack+1].value);
            vm.stack[vm.top_stack+1].isAlloc = false;
        }
        vm.stack[vm.top_stack+1].value = NULL;
        if(vm.stack[vm.top_stack].isAlloc) {
            free(vm.stack[vm.top_stack].value);
            vm.stack[vm.top_stack].isAlloc = false;
        }
        vm.stack[vm.top_stack].value = temp;
        vm.stack[vm.top_stack].isAlloc = true;
        vm.stack[vm.top_stack].type = FLOAT;
        vm.top_stack++;
    } else if(vm.stack[vm.top_stack].type == INT|| vm.stack[vm.top_stack-1].type == INT) {
        //操作数2
        long int val2 = 0;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            val2 = (long int)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            val2 = (long int)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;
        }
        //操作数1
        vm.top_stack--;    //此时指向第一个操作数
        long int val1 = 0;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            val1 = (long int)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            val1 = (long int)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;
        }

        long int* temp = (long int*)calloc(1, sizeof(long int));
        if(!temp) return -1;
        *temp = val1+val2;

#ifdef SHOW_HX_DEBUG_DETAIL
        wprintf(L"\33[33m[DEG]\33[0m \33[31mADD\33[0m {v1:%ld(索引%d), v2:%ld}-> %ld\n",val1, vm.top_stack, val2, *temp);
#endif

        if(vm.stack[vm.top_stack+1].isAlloc) {
            free(vm.stack[vm.top_stack+1].value);
            vm.stack[vm.top_stack+1].isAlloc = false;
        }
        vm.stack[vm.top_stack+1].value = NULL;
        if(vm.stack[vm.top_stack].isAlloc) {
            free(vm.stack[vm.top_stack].value);
            vm.stack[vm.top_stack].isAlloc = false;
        }
        vm.stack[vm.top_stack].value = temp;
        vm.stack[vm.top_stack].isAlloc = true;
        vm.stack[vm.top_stack].type = INT;
        vm.top_stack++;
    } else if(vm.stack[vm.top_stack].type == CHAR|| vm.stack[vm.top_stack-1].type == CHAR) {

    }
    return 0;
}
int interprete_OP_MUL() {
#ifdef SHOW_HX_DEBUG_DETAIL
    wprintf(L"\33[33m[DEG]\33[0m解释\33[36mOP_MUL\33[0m...\n");
#endif
    if(vm.top_stack<2) {
        HXVMError(ERR_NULL_PTR);
        return -1;
    }
    vm.top_stack--;       //此时指向第二个操作数
    if(vm.stack[vm.top_stack].type == DOUBLE|| vm.stack[vm.top_stack-1].type == DOUBLE) {
        double val2 = 0.0;
        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            val2 = (double)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            val2 = (double)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;

        case FLOAT: {
            val2 = (double)(*((float*)vm.stack[vm.top_stack].value));
        }
        break;

        case DOUBLE: {
            val2 = *((double*)vm.stack[vm.top_stack].value);
        }
        break;
        }
        //操作数1
        vm.top_stack--;    //此时指向第一个操作数
        double val1 = 0.0;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            val1 = (double)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            val1 = (double)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;

        case FLOAT: {
            val1 = (double)(*((float*)vm.stack[vm.top_stack].value));
        }
        break;
        case DOUBLE: {
            val1 = *((double*)vm.stack[vm.top_stack].value);
        }
        break;
        }

        double* temp = (double*)calloc(1, sizeof(double));
        if(!temp) return -1;
        *temp = val1*val2;

#ifdef SHOW_HX_DEBUG_DETAIL
        wprintf(L"\33[33m[DEG]\33[0m \33[31mMUL\33[0m {v1:%lf, v2:%lf}-> %lf\n",val1, val2, *temp);
#endif
        if(vm.stack[vm.top_stack+1].isAlloc) {
            free(vm.stack[vm.top_stack+1].value);
            vm.stack[vm.top_stack+1].isAlloc = false;
        }
        vm.stack[vm.top_stack+1].value = NULL;
        if(vm.stack[vm.top_stack].isAlloc) {
            free(vm.stack[vm.top_stack].value);
            vm.stack[vm.top_stack].isAlloc = false;
        }
        vm.stack[vm.top_stack].value = temp;
        vm.stack[vm.top_stack].isAlloc = true;
        vm.stack[vm.top_stack].type = DOUBLE;
        vm.top_stack++;

    } else if(vm.stack[vm.top_stack].type == FLOAT|| vm.stack[vm.top_stack-1].type == FLOAT) {
        //操作数2
        float val2 = 0.0f;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            val2 = (float)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            val2 = (float)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;

        case FLOAT: {
            val2 = (*((float*)vm.stack[vm.top_stack].value));
        }
        break;
        }
        //操作数1
        vm.top_stack--;    //此时指向第一个操作数
        float val1 = 0.0f;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            val1 = (float)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            val1 = (float)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;

        case FLOAT: {
            val1 = (*((float*)vm.stack[vm.top_stack].value));
        }
        break;
        }

        float* temp = (float*)calloc(1, sizeof(float));
        if(!temp) return -1;
        *temp = val1*val2;

#ifdef SHOW_HX_DEBUG_DETAIL
        wprintf(L"\33[33m[DEG]\33[0m \33[31mMUL\33[0m {v1:%ld, v2:%ld}-> %ld\n",val1, val2, *temp);
#endif

        if(vm.stack[vm.top_stack+1].isAlloc) {
            free(vm.stack[vm.top_stack+1].value);
            vm.stack[vm.top_stack+1].isAlloc = false;
        }
        vm.stack[vm.top_stack+1].value = NULL;
        if(vm.stack[vm.top_stack].isAlloc) {
            free(vm.stack[vm.top_stack].value);
            vm.stack[vm.top_stack].isAlloc = false;
        }
        vm.stack[vm.top_stack].value = temp;
        vm.stack[vm.top_stack].isAlloc = true;
        vm.stack[vm.top_stack].type = FLOAT;
        vm.top_stack++;
    } else if(vm.stack[vm.top_stack].type == INT|| vm.stack[vm.top_stack-1].type == INT) {
        //操作数2
        long int val2 = 0;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            val2 = (long int)(uint16_t)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            val2 = (long int)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;
        }
        //操作数1
        vm.top_stack--;    //此时指向第一个操作数
        long int val1 = 0;

        switch(vm.stack[vm.top_stack].type) {
        case INT: {
            val1 = (long int)(*((long int*)vm.stack[vm.top_stack].value));
        }
        break;

        case CHAR: {
            val1 = (long int)(uint16_t)(wchar)(*((wchar*)vm.stack[vm.top_stack].value));
        }
        break;
        }

        long int* temp = (long int*)calloc(1, sizeof(long int));
        if(!temp) return -1;
        *temp = val1*val2;

#ifdef SHOW_HX_DEBUG_DETAIL
        wprintf(L"\33[33m[DEG]\33[0m \33[31mMUL\33[0m {v1:%ld, v2:%ld}-> %ld\n",val1, val2, *temp);
#endif

        if(vm.stack[vm.top_stack+1].isAlloc) {
            free(vm.stack[vm.top_stack+1].value);
            vm.stack[vm.top_stack+1].isAlloc = false;
        }
        vm.stack[vm.top_stack+1].value = NULL;
        if(vm.stack[vm.top_stack].isAlloc) {
            free(vm.stack[vm.top_stack].value);
            vm.stack[vm.top_stack].isAlloc = false;
        }
        vm.stack[vm.top_stack].value = temp;
        vm.stack[vm.top_stack].isAlloc = true;
        vm.stack[vm.top_stack].type = INT;
        vm.top_stack++;
    } else if(vm.stack[vm.top_stack].type == CHAR|| vm.stack[vm.top_stack-1].type == CHAR) {

    }
    return 0;
}
#endif