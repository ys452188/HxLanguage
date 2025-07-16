#ifndef COMPILER_H
#define COMPILER_H
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include "lexer.h"
#include "objectCode.h"
int compile(TokenStream* ts,ObjectCode* oc) {
#ifdef _WIN32
    setlocale(LC_ALL,"zh_CN.UTF-8");
#endif
    if(ts->tokens==NULL) {
        return 255;
    }
    if(initSymTable()) {
#ifndef _WIN32
        fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
        fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
        return 255;
    }
    for(long int index = 0; index < ts->size; index++) {

        //printf("%ls\n",ts->tokens[index].value);

        switch((ts->tokens[index]).type) {
        case TOKEN_KEYWORD:    //关键字
            //fun关键字
            //定义函数 ::= <"fun">|<"定义函数"> <标识符> <"("> <标识符> <":"> <关键字>|<标识符> <")"> <"{"> <代码块> <"}">
            if(wcscmp(ts->tokens[index].value,L"fun") == 0 || wcscmp(ts->tokens[index].value,L"定义函数") == 0) {
                index++;
                if(ts->tokens[index].type != TOKEN_INDENTIFIER) {
#ifndef _WIN32
                    fprintf(stderr,"\33[31m[E]\"定义函数\"或\"fun\"关键字后应为函数名(标识符)\n(位于 %d 行, %d 列)\33[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\33[31m[E]\"定义函数\"或\"fun\"关键字后应为函数名(标识符)\n(位于 %d 行, %d 列)\33[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    return 255;
                }
                Function newFunc = {0};
                //存储函数名
                newFunc.name = (wchar_t*)calloc(wcslen(ts->tokens[index].value)+1, sizeof(wchar_t));
                if(!newFunc.name) {
#ifndef _WIN32
                    fprintf(stderr,"\33[31m[E]内存分配失败！\33[0m\n");
#else
                    fwprintf(stderr,L"\33[31m[E]内存分配失败！\33[0m\n");
#endif
                    return 255;
                }
                wcscpy(newFunc.name, ts->tokens[index].value);
                printf("\33[33m函数名:%ls\t地址：%p\33[0m\n",newFunc.name,newFunc.name);

                index++;
                //printf("%ls\n",ts->tokens[index].value);
                if(!((wcscmp(ts->tokens[index].value, L"(")==0) || (wcscmp(ts->tokens[index].value, L"（")==0))) {
#ifndef _WIN32
                    fprintf(stderr,"\33[31m[E]函数名后应为括号(运算符)\n(位于 %d 行, %d 列)\33[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\33[31m[E]函数名后应为括号(运算符)\n(位于 %d 行, %d 列)\33[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    hxFree(&(newFunc.name));
                    return 255;
                }
                index++;
                //printf("%ls\n",ts->tokens[index].value);
                if(wcscmp(ts->tokens[index].value, L")")==0 || wcscmp(ts->tokens[index].value, L"）")==0) {
                    newFunc.args = NULL;
                    newFunc.argc = 0;
                    printf("\33[33m参数：无\n\33[0m");
                } else {
                    if(setArgs(ts->tokens, &index, &newFunc)) {
#ifndef _WIN32
                        fprintf(stderr,"\33[31m[E]内存分配失败！\33[0m\n");
#else
                        fwprintf(stderr,L"\33[31m[E]内存分配失败！\33[0m\n");
#endif
                        hxFree(&(newFunc.name));
                        for(int i = 0; i < newFunc.argc; i++) {
                            hxFree(&(newFunc.args[i].name));
                            hxFree(&(newFunc.args[i].type));
                        }
                        return 255;
                    }
                    for(int i = 0; i < newFunc.argc; i++) {
                        printf("\33[33m参数%d：名字 %ls, 类型 %ls\n\33[0m", i+1, newFunc.args[i].name, newFunc.args[i].type);
                    }
                }
                index++;
                //printf("%ls\n",ts->tokens[index].value);
                if(!(wcscmp(ts->tokens[index].value, L":")==0 || wcscmp(ts->tokens[index].value, L"：")==0)) {
#ifndef _WIN32
                    fprintf(stderr,"\33[31m[E]括号后应为冒号\n(位于 %d 行, %d 列)\33[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\33[31m[E]括号后应为冒号\n(位于 %d 行, %d 列)\33[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    hxFree(&(newFunc.name));
                    return 255;
                }
                index++;
                //printf("%ls\n",ts->tokens[index].value);
                if(!(ts->tokens[index].type == TOKEN_INDENTIFIER || ts->tokens[index].type == TOKEN_KEYWORD)) {
#ifndef _WIN32
                    fprintf(stderr,"\33[31m[E]冒号后应为该函数的返回类型(关键字或标识符)\n(位于 %d 行, %d 列)\33[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\33[31m[E]冒号后应为该函数的返回类型(关键字或标识符)\n(位于 %d 行, %d 列)\33[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    hxFree(&(newFunc.name));
                    return 255;
                }
                //printf("%ls\n",ts->tokens[index].value);
                if(wcscmp(ts->tokens[index].value, L"void") == 0) {
                    newFunc.ret_type = NULL;
                    printf("\33[33m返回类型：void\t地址：%p\n\33[0m",newFunc.ret_type);
                } else {
                    newFunc.ret_type = (wchar_t*)calloc(wcslen(ts->tokens[index].value)+1, sizeof(wchar_t));
                    if(!newFunc.ret_type) {
#ifndef _WIN32
                        fprintf(stderr,"\33[31m[E]内存分配失败！\33[0m\n");
#else
                        fwprintf(stderr,L"\33[31m[E]内存分配失败！\33[0m\n");
#endif
                        hxFree(&(newFunc.name));
                        return 255;
                    }
                    wcscpy(newFunc.ret_type,ts->tokens[index].value);
                    printf("\33[33m返回类型：%ls\t地址：%p\n\33[0m",newFunc.ret_type,newFunc.ret_type);
                }
                index++;
                if(wcscmp(ts->tokens[index].value, L"{") != 0) {
#ifndef _WIN32
                    fprintf(stderr,"\33[31m[E]函数名后应为花括号\n(位于 %d 行, %d 列)\33[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\33[31m[E]函数名后应为花括号\n(位于 %d 行, %d 列)\33[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    hxFree(&(newFunc.name));
                    if(newFunc.args) {
                        for(int i = 0; i < newFunc.argc; i++) {
                            hxFree(&(newFunc.args[i].name));
                            hxFree(&(newFunc.args[i].type));
                        }
                        free(newFunc.args);
                    }
                    return 255;
                }
                int open = 1;
                int close = 0;
                for(index++; index < ts->size; index++) {
                    if(wcscmp(ts->tokens[index].value, L"{") == 0) {
                        open++;
                    }
                    if(wcscmp(ts->tokens[index].value, L"}") == 0) {
                        close++;
                    }
                    if(wcscmp(ts->tokens[index].value, L"fun") == 0 || wcscmp(ts->tokens[index].value, L"定义函数") == 0) {
#ifndef _WIN32
                        fprintf(stderr, "\33[31m[E]函数体内不允许定义函数！(位于 %d 行, %d 列)\33[0m\n", ts->tokens[index].lin, ts->tokens[index].col);
#else
                        fwprintf(stderr, L"\33[31m[E]函数体内不允许定义函数！(位于 %d 行, %d 列)\33[0m\n", ts->tokens[index].lin, ts->tokens[index].col);
#endif
                        hxFree(&(newFunc.name));
                        hxFree(&(newFunc.ret_type));
                        if(newFunc.args) {
                            for(int i = 0; i < newFunc.argc; i++) {
                                hxFree(&(newFunc.args[i].name));
                                hxFree(&(newFunc.args[i].type));
                            }
                            free(newFunc.args);
                        }
                        return 255;
                    }

                    if(open == close) {
                        break;
                    }
                }
                if(open != close) {
#ifndef _WIN32
                    fprintf(stderr,"\33[31m[E]花括号未正确关闭！\n(位于 %d 行, %d 列)\33[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\33[31m[E]花括号未正确关闭！\n(位于 %d 行, %d 列)\33[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    hxFree(&(newFunc.name));
                    hxFree(&(newFunc.ret_type));
                    if(newFunc.args) {
                        for(int i = 0; i < newFunc.argc; i++) {
                            hxFree(&(newFunc.args[i].name));
                            hxFree(&(newFunc.args[i].type));
                        }
                        free(newFunc.args);
                    }
                    return 255;
                }
                //存储函数体

                hxFree(&(newFunc.name));
                hxFree(&(newFunc.ret_type));
                if(newFunc.args) {
                    for(int i = 0; i < newFunc.argc; i++) {
                        hxFree(&(newFunc.args[i].name));
                        hxFree(&(newFunc.args[i].type));
                    }
                    free(newFunc.args);
                }
            }
            break;
        case TOKEN_VALUE:      //字面量
            break;
        case TOKEN_INDENTIFIER://标识符
            break;
        case TOKEN_OPERATOR:   //运算符
            break;
        }
    }
    return 0;
}
#endif