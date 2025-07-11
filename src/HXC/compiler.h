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
    if(initObjectCode(oc) == 255) {
#ifndef _WIN32
        fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
        fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
        return 255;
    }
    for(long int i = 0; i < ts->size; i++) {

        //printf("%ls\n",ts->tokens[i].value);

        switch((ts->tokens[i]).type) {
        case TOKEN_KEYWORD:    //关键字
            if((wcscmp((ts->tokens[i]).value,L"var") == 0)||(wcscmp((ts->tokens[i]).value,L"定义变量") == 0)) {         //var
                int startIndex = i;
                int endIndex = 0;
                for(endIndex = startIndex; endIndex < ts->size; endIndex++) {

                    //printf("%ls\n",ts->tokens[endIndex].value);

                    if(wcscmp((ts->tokens[endIndex]).value,L";") == 0 || wcscmp((ts->tokens[endIndex]).value,L"；") == 0) {
                        break;
                    }
                }
                if(endIndex==startIndex+1) {
#ifndef _WIN32
                    fprintf(stderr,"\033[31m[E]关键字使用错误！\n(在%d行%d列)\n\033[0m",ts->tokens[startIndex].lin,ts->tokens[startIndex].col);
#else
                    fwprintf(stderr,L"\033[31m[E]关键字使用错误！\n(在%d行%d列)\n\033[0m",ts->tokens[startIndex].lin,ts->tokens[startIndex].col);
#endif
                    return 255;
                }
            } else if((wcscmp((ts->tokens[i]).value,L"con") == 0)||(wcscmp((ts->tokens[i]).value,L"定义常量") == 0)) {  //con

            } else if((wcscmp((ts->tokens[i]).value,L"fun") == 0)||(wcscmp((ts->tokens[i]).value,L"定义函数") == 0)) { //fun
                int index = i;
                index++;
                Function newFunction = {
                    NULL,
                    NULL,
                    0,
                    NULL
                };
                Block newBlock;
                if((ts->tokens[index].type) != TOKEN_INDENTIFIER) {
#ifndef _WIN32
                    fprintf(stderr,"\033[31m[E]关键字使用错误！\n(在%d行%d列)\n\033[0m",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\033[31m[E]关键字使用错误！\n(在%d行%d列)\n\033[0m",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    return 255;
                }
                newFunction.name = (wchar_t*)calloc(1+wcslen(ts->tokens[index].value),sizeof(wchar_t));
                if(!(newFunction.name)) {
#ifndef _WIN32
                    fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
                    fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
                    return 255;
                }
                wcscpy(newFunction.name,ts->tokens[index].value);

                //printf("%ls\n",newFunction.name);

                index++;

                //printf("%ls\n",ts->tokens[index].value);

                if(((ts->tokens[index].type) != TOKEN_OPERATOR) || (wcscmp(ts->tokens[index].value,L"(")!=0 && wcscmp(ts->tokens[index].value,L"（")!=0)) {
#ifndef _WIN32
                    fprintf(stderr,"\33[31m[E]声明函数时,函数名后应为括号\n(在%d行%d列)\n\33[0m",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\33[31m[E]声明函数时,函数名后应为括号\n(在%d行%d列)\n\33[0m",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    hxFree(&(newFunction.name));
                    return 255;
                }
                index++;
                //printf("%ls\n",ts->tokens[index].value);
                if(ts->tokens[index].type == TOKEN_KEYWORD) {
                    //printf("%ls\n",ts->tokens[index].value);
                    if(wcscmp(ts->tokens[index].value,L"void") == 0 && (wcscmp(ts->tokens[index+1].value,L")") == 0 || wcscmp(ts->tokens[index+1].value,L"）") == 0)) {
                        newFunction.args = NULL;
                        newFunction.argc = 0;
                    } else {
#ifndef _WIN32
                        fprintf(stderr,"\33[31m[E]关键字使用错误！\n(在%d行%d列)\n\33[0m",ts->tokens[index].lin,ts->tokens[index].col);
#else
                        fwprintf(stderr,L"\33[31m[E]关键字使用错误！\n(在%d行%d列)\n\33[0m",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                        hxFree(&(newFunction.name));
                        return 255;
                    }
                } else if(ts->tokens[index].type == TOKEN_INDENTIFIER) {
                    //printf("%ls\n",ts->tokens[index].value);
                    newFunction.args = (Variable*)calloc(1,sizeof(Variable));
                    newFunction.argc = 0;
                    if(!(newFunction.args)) {
#ifndef _WIN32
                        fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
                        fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
                        return 255;
                    }
                    int err = setArgs(ts->tokens,&index,&newFunction);
                    if(err != 0) {
                        hxFree(&(newFunction.name)); 
                        if(newFunction.argc == 0) {
                            hxFree(&(newFunction.args));
                        	return 255;
                        }
                        for(int j = 0; j<=newFunction.argc; j++) {
                            hxFree(&(newFunction.args[j].name));
                            hxFree(&(newFunction.args[j].type));
                        }
                        hxFree(&(newFunction.args));
                        return 255;
                    }
                } else if(wcscmp(ts->tokens[index].value,L")")==0 || wcscmp(ts->tokens[index].value,L"）")==0) {
                    newFunction.args = NULL;
                    newFunction.argc = 0;
                } else {
#ifndef _WIN32
                    fprintf(stderr,"\33[31m[E]声明函数时,函数名后的括号内的第一个token应为void或标识符\n(在%d行%d列)\n\33[0m",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\33[31m[E]声明函数时,函数名后的括号内的第一个token应为void或标识符\n(在%d行%d列)\n\33[0m",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    hxFree(&(newFunction.name));
                    return 255;
                }
                //填表
                void* tmp = realloc(oc->functions,(oc->function_count+1)*sizeof(Function));
                if(!tmp) {
#ifndef _WIN32
                    fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
                    fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
                    hxFree(&(newFunction.name));
                    if(newFunction.argc == 0) {
                        hxFree(&(newFunction.args));
                    	return 255;
                    }
                    for(int j = 0; j<=newFunction.argc; j++) {
                        hxFree(&(newFunction.args[j].name));
                        hxFree(&(newFunction.args[j].type));
                    }
                    hxFree(&(newFunction.args));
                    return 255;
                }
                oc->functions = (Function*)tmp;
                if(isDuplicateDefineFunction(&newFunction,oc->functions,oc->function_count+1) == 1) {
#ifndef _WIN32
                    fprintf(stderr,"\033[31m[E]函数被重复定义！\n(在%d行%d列)\033[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#else
                    fwprintf(stderr,L"\033[31m[E]函数被重复定义！\n(在%d行%d列)\033[0m\n",ts->tokens[index].lin,ts->tokens[index].col);
#endif
                    hxFree(&(newFunction.name));
                    if (newFunction.argc == 0) {
                        hxFree(&(newFunction.args));
                        return 255;
                    }
                    for(int j = 0; j<=newFunction.argc; j++) {
                        hxFree(&(newFunction.args[j].name));
                        hxFree(&(newFunction.args[j].type));
                    }
                    hxFree(&(newFunction.args));
                    return 255;
                }
                oc->functions[oc->function_count] = newFunction;
                printf("%ls\n",oc->functions[oc->function_count].name);
                oc->function_count++;
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