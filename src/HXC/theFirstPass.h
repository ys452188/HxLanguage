#ifndef HXLANG_THE_FIRST_PASS_H
#define HXLANG_THE_FIRST_PASS_H
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lexer.h"
#include "output.h"

typedef enum ErrorType {
    ERR_FUN_NAME_SHOULD_BEHIND_FUN,                      //关键字后应为函数名
    ERR_FUN_QUITE_SHOULD_BEHIND_NAME,                    //函数名后应为括号
    ERR_FUN_BEHIND_QUITE_SHOULD_BE_VOID_OR_ARGS,         //开括号后应为参数或void
    ERR_QUITE_NOT_CLOSE,                                 //括号未正确开闭
    ERR_FUN_ARG_NAME_MUST_BE_ID,                         //参数名必须是标识符
    ERR_FUN_MAOHAO_SHOULD_BEHIND_ARG_NAME,               //参数名后应为冒号
    ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE,                    //冒号后应为类型
    ERR_FUN_BEHIND_DOUHAO_SHOULD_BE_NEXT_ARG,            //逗号后应为下一参数
    ERR_FUN_BEHIND_QUITE_SHOULD_BE_MAOHAO,               //闭括号后应为冒号
    ERR_FUN_BEHIND_MAOHAO_SHOULD_BE_RET_TYPE,            //闭括号后的冒号后应为返回值
    ERR_FUN_BEHIND_RET_TYPE_SHOULD_BE_HUAKUOHAO,         //返回类型后应为花括号
    ERR_FUN_CONNOT_DEFIND_FUN_IN_ANOTHER_FUN_S_BODY,     //不可在一个函数的函数体内定义另一函数
    ERR_HUAKUOHAO_NOT_CLOSE,                             //花括号未正确开闭
    ERR_FUN_REPEAT_DEFINED,                              //函数重复定义
    ERR_CLASS_BEHIND_CLASS_SHOULD_BE_NAME,               //class关键字后应为类名(标识符)
    ERR_CLASS_BEHIND_NAME_MUST_BE_HUAKUOHAO,             //类名后必须是花括号
    ERR_CLASS_BEHIND_ACCESS_SHOULD_BE_MAOHAO,            //访问权限修饰符后应为冒号
    ERR_NO_VAR_NAME,                                     //缺少变量名
    ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO,                  //变/常量符号后应为冒号
    ERR_CLASS_MEMBER_SHOULD_ASSGIN_IN_CONSTRUCT_FUNC,    //类成员应在构造方法中进行初始化
    ERR_SYNTAX_WHEN_DEFINE_CLASS,                        //定义类时的语法错误
    ERR_CLASS_BEHIND_PARENT_KEYWORD_MUST_BE_MAOHAO,      //父类关键字后应为冒号
    ERR_CLASS_NO_PARENT_CLASS_NAME,                      //定义派生类时缺父类名
} ErrorType;
typedef struct CheckerSymbol {
    wchar* name;
    wchar* type;
    bool isOnlyRead;
}  CheckerSymbol;
typedef struct CheckerClass {
    wchar* name;
    wchar* parent_class_name;  //父类名

    CheckerSymbol* pub_sym;
    int pub_sym_size;
    CheckerSymbol* pri_sym;
    int pri_sym_size;
    CheckerSymbol* pro_sym;
    int pro_sym_size;
} CheckerClass;
typedef struct CheckerFunction {   //检查阶段的函数
    wchar* name;
    wchar* ret_type;               //NULL表示无返回值

    CheckerSymbol* args;           //NULL表示无参数
    int args_size;

    Token* body;
    int body_size;
} CheckerFunction;
typedef struct CheckerOutput {
    CheckerFunction* checker_func;
    int func_size;
    CheckerSymbol* global_sym;
    int global_sym_size;
    CheckerClass* checker_class;
    int checker_class_size;
} CheckerOutput;

CheckerOutput checkerOutput = {0};

int isFunctionRepeat(CheckerFunction* fun1, CheckerFunction* fun2);
int initCheckerOutput();
void freeCheckerOutput();
void error(ErrorType, int lin);
int firstPass() {
    initLocale();
    if(initCheckerOutput())  {
        return -1;
    }
    int func_index = 0;
    int class_index = 0;
    while(getNextToken() == 0) {
        if(wcsequ(tokensPtr->value, L"fun") == 1 || wcsequ(tokensPtr->value, L"定义函数") == 1) {  //定义函数
            if(getNextToken()) {
                error(ERR_FUN_NAME_SHOULD_BEHIND_FUN,tokensPtr->lin);
                return 255;
            }
            if(func_index >= checkerOutput.func_size) {
                //扩容
                checkerOutput.func_size = func_index+1;
                void* temp = realloc(checkerOutput.checker_func, sizeof(CheckerFunction)*checkerOutput.func_size);
                if(!temp) return -1;
                //初始化
                checkerOutput.checker_func = (CheckerFunction*)temp;
                checkerOutput.checker_func[func_index].name = NULL;
                checkerOutput.checker_func[func_index].ret_type = NULL;
                checkerOutput.checker_func[func_index].args = NULL;
                checkerOutput.checker_func[func_index].args_size = 0;
                checkerOutput.checker_func[func_index].body = NULL;
                checkerOutput.checker_func[func_index].body_size = 0;
            }
            //分析函数名
            if(tokensPtr->type != TOK_ID) {
                error(ERR_FUN_NAME_SHOULD_BEHIND_FUN,tokensPtr->lin);
                return 255;
            }
            //printf("%ls\n", tokensPtr->value);
            checkerOutput.checker_func[func_index].name = (wchar*)calloc(wcslen(tokensPtr->value) + 1, sizeof(wchar));
            if(!(checkerOutput.checker_func[func_index].name)) return -1;
            wcscpy(checkerOutput.checker_func[func_index].name, tokensPtr->value);
            //printf("func.name:%ls\n",checkerOutput.checker_func[func_index].name);

            //分析参数
            if(getNextToken()) {
                error(ERR_FUN_QUITE_SHOULD_BEHIND_NAME,tokensPtr->lin);
                return 255;
            }
            //printf("%ls\n",tokensPtr->value);
            if(wcsequ(tokensPtr->value, L"(") != 1 && wcsequ(tokensPtr->value, L"（") != 1) {
                error(ERR_FUN_QUITE_SHOULD_BEHIND_NAME,tokensPtr->lin);
                return 255;
            }
            if(getNextToken()) {
                error(ERR_FUN_BEHIND_QUITE_SHOULD_BE_VOID_OR_ARGS, tokensPtr->lin);
                return 255;
            }
            //printf("%ls\n",tokensPtr->value);
            if(wcsequ(tokensPtr->value, L")") == 1 || wcsequ(tokensPtr->value, L"）") == 1) {
                checkerOutput.checker_func[func_index].args = NULL;
                checkerOutput.checker_func[func_index].args_size = 0;
                //printf("func.args:null\n");
            } else if(wcsequ(tokensPtr->value, L"void") == 1 || wcsequ(tokensPtr->value, L"无参数") == 1) {
                checkerOutput.checker_func[func_index].args = NULL;
                checkerOutput.checker_func[func_index].args_size = 0;
                //printf("func.args:null\n");
                if(getNextToken()) {
                    error(ERR_QUITE_NOT_CLOSE, tokensPtr->lin);
                    return 255;
                }
                if(wcsequ(tokensPtr->value, L")") != 1 && wcsequ(tokensPtr->value, L"）") != 1) {
                    error(ERR_QUITE_NOT_CLOSE, tokensPtr->lin);
                    return 255;
                }
            } else {    //处理参数
                checkerOutput.checker_func[func_index].args = (CheckerSymbol*)calloc(1, sizeof(CheckerSymbol));
                if(!(checkerOutput.checker_func[func_index].args)) return -1;
                checkerOutput.checker_func[func_index].args_size = 1;
                int args_index = 0;
                checkerOutput.checker_func[func_index].args[args_index].name = NULL;
                checkerOutput.checker_func[func_index].args[args_index].type = NULL;
                checkerOutput.checker_func[func_index].args[args_index].isOnlyRead = false;
                while(1) {
                    if(args_index >= checkerOutput.checker_func[func_index].args_size) {
                        void* temp = realloc(checkerOutput.checker_func[func_index].args, sizeof(CheckerSymbol)*(args_index+1));
                        if(!temp) return -1;
                        checkerOutput.checker_func[func_index].args_size = args_index+1;
                        checkerOutput.checker_func[func_index].args = (CheckerSymbol*)temp;
                        checkerOutput.checker_func[func_index].args[args_index].name = NULL;
                        checkerOutput.checker_func[func_index].args[args_index].type = NULL;
                        checkerOutput.checker_func[func_index].args[args_index].isOnlyRead = false;
                    }
                    //printf("tokensPtr->value : %ls\n",tokensPtr->value);
                    if(tokensPtr->type != TOK_ID) {
                        error(ERR_FUN_ARG_NAME_MUST_BE_ID, tokensPtr->lin);
                        return 255;
                    }
                    checkerOutput.checker_func[func_index].args[args_index].name = (wchar*)calloc(wcslen(tokensPtr->value) + 1, sizeof(wchar));
                    if(!(checkerOutput.checker_func[func_index].args[args_index].name)) return -1;
                    wcscpy(checkerOutput.checker_func[func_index].args[args_index].name, tokensPtr->value);
                    //printf("func.args.name:%ls\n",checkerOutput.checker_func[func_index].args[args_index].name);
                    if(getNextToken()) {
                        error(ERR_FUN_MAOHAO_SHOULD_BEHIND_ARG_NAME, tokensPtr->lin);
                        return 255;
                    }
                    if(wcsequ(tokensPtr->value, L":") != 1 && wcsequ(tokensPtr->value, L"：") !=1) {
                        error(ERR_FUN_MAOHAO_SHOULD_BEHIND_ARG_NAME, tokensPtr->lin);
                        return 255;
                    }
                    if(getNextToken()) {
                        error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                        return 255;
                    }
                    if(tokensPtr->type != TOK_ID && tokensPtr->type != TOK_KW) {
                        error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                        return 255;
                    }
                    checkerOutput.checker_func[func_index].args[args_index].type = (wchar*)calloc(wcslen(tokensPtr->value) + 1, sizeof(wchar));
                    if(!(checkerOutput.checker_func[func_index].args[args_index].type)) return -1;
                    wcscpy(checkerOutput.checker_func[func_index].args[args_index].type, tokensPtr->value);
                    //printf("func.args.type:%ls\n",checkerOutput.checker_func[func_index].args[args_index].type);
                    args_index++;
                    if(getNextToken()) {
                        error(ERR_QUITE_NOT_CLOSE, tokensPtr->lin);
                        return 255;
                    }
                    if(wcsequ(tokensPtr->value, L")") == 1 || wcsequ(tokensPtr->value, L"）") == 1) break;
                    else if(wcsequ(tokensPtr->value, L",") == 1) {
                        if(getNextToken()) {
                            error(ERR_FUN_BEHIND_DOUHAO_SHOULD_BE_NEXT_ARG, tokensPtr->lin);
                            return 255;
                        }
                        if(wcsequ(tokensPtr->value, L")") == 1 || wcsequ(tokensPtr->value, L"）") == 1) {
                            error(ERR_FUN_BEHIND_DOUHAO_SHOULD_BE_NEXT_ARG, tokensPtr->lin);
                            return 255;
                        }
                        continue;
                    }
                    else {
                        error(ERR_QUITE_NOT_CLOSE, tokensPtr->lin);
                        return 255;
                    }
                    if(getNextToken()) {
                        error(ERR_QUITE_NOT_CLOSE, tokensPtr->lin);
                        return 255;
                    }
                }
            }
            //检查是否重复定义
            //printf("%d\n",func_index);
            if(func_index != 0) {
                for(int i = func_index-1; i >= 0; i--) {
                    if(isFunctionRepeat(&(checkerOutput.checker_func[func_index]), &(checkerOutput.checker_func[i]))) {
                        error(ERR_FUN_REPEAT_DEFINED, tokensPtr->lin);
                        return 255;
                    }
                }
            }
            //分析返回值
            if(getNextToken()) {
                error(ERR_FUN_BEHIND_QUITE_SHOULD_BE_MAOHAO, tokensPtr->lin);
                return 255;
            }
            if(wcsequ(tokensPtr->value, L":") != 1 && wcsequ(tokensPtr->value, L"：") != 1) {
                error(ERR_FUN_BEHIND_QUITE_SHOULD_BE_MAOHAO, tokensPtr->lin);
                return 255;
            }
            if(getNextToken()) {
                error(ERR_FUN_BEHIND_MAOHAO_SHOULD_BE_RET_TYPE, tokensPtr->lin);
                return 255;
            }
            if(tokensPtr->type != TOK_ID && tokensPtr->type != TOK_KW) {
                error(ERR_FUN_BEHIND_MAOHAO_SHOULD_BE_RET_TYPE, tokensPtr->lin);
                return 255;
            }
            if(wcsequ(tokensPtr->value, L"void") || wcsequ(tokensPtr->value, L"无参数")) {
                checkerOutput.checker_func[func_index].ret_type = NULL;
            } else {
                checkerOutput.checker_func[func_index].ret_type = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                if(!(checkerOutput.checker_func[func_index].ret_type)) return -1;
                wcscpy(checkerOutput.checker_func[func_index].ret_type, tokensPtr->value);
            }
            //分析函数体
            if(getNextToken()) {
                error(ERR_FUN_BEHIND_RET_TYPE_SHOULD_BE_HUAKUOHAO, tokensPtr->lin);
                return 255;
            }
            if(wcsequ(tokensPtr->value, L"{") != 1) {
                error(ERR_FUN_BEHIND_RET_TYPE_SHOULD_BE_HUAKUOHAO, tokensPtr->lin);
                return 255;
            }
            Token* start = NULL;
            if(getNextToken()) {
                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                return 255;
            }
            start = tokensPtr;
            int open = 1;
            int close = 0;
            while(1) {
                if(wcsequ(tokensPtr->value, L"{")) open++;
                if(wcsequ(tokensPtr->value, L"}")) close++;
                if(wcsequ(tokensPtr->value, L"fun") || wcsequ(tokensPtr->value, L"定义函数")) {
                    error(ERR_FUN_CONNOT_DEFIND_FUN_IN_ANOTHER_FUN_S_BODY, tokensPtr->lin);
                    return 255;
                }
                if(open == close) break;
                if(getNextToken()) break;
            }
            if(open != close) {
                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                return 255;
            }
            int body_length = tokensPtr-start;
            if(body_length == 0) {
                checkerOutput.checker_func[func_index].body = NULL;             //函数体为空
                //printf("函数体为空\n");
            } else {
                checkerOutput.checker_func[func_index].body_size = body_length;
                checkerOutput.checker_func[func_index].body = (Token*)calloc(body_length, sizeof(Token));
                if(!(checkerOutput.checker_func[func_index].body)) return -1;
                for(int i = 0; i < body_length; i++) {
                    checkerOutput.checker_func[func_index].body[i].value = (wchar*)calloc(wcslen(start[i].value)+1, sizeof(wchar));
                    if(!(checkerOutput.checker_func[func_index].body[i].value)) return -1;
                    wcscpy(checkerOutput.checker_func[func_index].body[i].value, start[i].value);
                    checkerOutput.checker_func[func_index].body[i].type = start[i].type;
                    checkerOutput.checker_func[func_index].body[i].mark = start[i].mark;
                    checkerOutput.checker_func[func_index].body[i].lin = start[i].lin;
                }
            }
            func_index++;
        } else if(wcsequ(tokensPtr->value, L"class") || wcsequ(tokensPtr->value, L"定义类")) {
            //分析类名
            if(getNextToken()) {
                error(ERR_CLASS_BEHIND_CLASS_SHOULD_BE_NAME, tokensPtr->lin);
                return 255;
            }
            //printf("value: %ls\n",tokensPtr->value);
            if(tokensPtr->type != TOK_ID) {
                error(ERR_CLASS_BEHIND_CLASS_SHOULD_BE_NAME, tokensPtr->lin);
                return 255;
            }
            if(class_index >= checkerOutput.checker_class_size) {
                checkerOutput.checker_class_size = class_index+1;
                void* temp = realloc(checkerOutput.checker_class, checkerOutput.checker_class_size*sizeof(CheckerClass));
                if(!temp) return -1;
                checkerOutput.checker_class = (CheckerClass*)temp;

                checkerOutput.checker_class[class_index].name = NULL;
                checkerOutput.checker_class[class_index].parent_class_name = NULL;
                checkerOutput.checker_class[class_index].pub_sym = NULL;
                checkerOutput.checker_class[class_index].pub_sym_size = 0;
                checkerOutput.checker_class[class_index].pri_sym = NULL;
                checkerOutput.checker_class[class_index].pri_sym_size = 0;
                checkerOutput.checker_class[class_index].pro_sym = NULL;
                checkerOutput.checker_class[class_index].pro_sym_size = 0;
            }
            checkerOutput.checker_class[class_index].name = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
            if(!(checkerOutput.checker_class[class_index].name)) return -1;
            wcscpy(checkerOutput.checker_class[class_index].name, tokensPtr->value);
            wprintf(L"className: %ls\taddress: %p\n", checkerOutput.checker_class[class_index].name, &(checkerOutput.checker_class[class_index]));
            if(getNextToken()) {
                error(ERR_CLASS_BEHIND_NAME_MUST_BE_HUAKUOHAO, tokensPtr->lin);
                return 255;
            }
            if(wcsequ(tokensPtr->value, L"它的父类是") || wcsequ(tokensPtr->value, L"parent")) {   //分析父类
                if(getNextToken()) {
                    error(ERR_CLASS_NO_PARENT_CLASS_NAME, tokensPtr->lin);
                    return 255;
                }
                if((!wcsequ(tokensPtr->value, L":")) && (!wcsequ(tokensPtr->value, L"："))) {
                    //printf("%ls\n", tokensPtr->value);
                    error(ERR_CLASS_BEHIND_PARENT_KEYWORD_MUST_BE_MAOHAO,tokensPtr->lin);
                    return 255;
                }

                if(getNextToken()) {
                    error(ERR_CLASS_NO_PARENT_CLASS_NAME, tokensPtr->lin);
                    return 255;
                }
                if(tokensPtr->type != TOK_ID) {
                    error(ERR_CLASS_NO_PARENT_CLASS_NAME, tokensPtr->lin);
                    return 255;
                }

                checkerOutput.checker_class[class_index].parent_class_name = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                if(!(checkerOutput.checker_class[class_index].parent_class_name)) return -1;
                wcscpy(checkerOutput.checker_class[class_index].parent_class_name, tokensPtr->value);
                //printf("父类：%ls\n",checkerOutput.checker_class[class_index].parent_class_name);

                if(getNextToken()) {
                    error(ERR_CLASS_BEHIND_NAME_MUST_BE_HUAKUOHAO, tokensPtr->lin);
                    return 255;
                }
            }
            if(!wcsequ(tokensPtr->value, L"{")) {
                error(ERR_CLASS_BEHIND_NAME_MUST_BE_HUAKUOHAO, tokensPtr->lin);
                return 255;
            }
            if(getNextToken()) {
                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                return 255;
            }
            int pub_sym_index = 0;
            int pri_sym_index = 0;
            int pro_sym_index = 0;
            int isClassEnd = 0;
            while(!isClassEnd) {
                //公有成员
                if(wcsequ(tokensPtr->value, L"pub") || wcsequ(tokensPtr->value, L"公有成员")) {
parsePublicMember:
                    if(getNextToken()) {
                        error(ERR_CLASS_BEHIND_ACCESS_SHOULD_BE_MAOHAO, tokensPtr->lin);
                        return 255;
                    }
                    if((!wcsequ(tokensPtr->value, L":")) && (!(wcsequ(tokensPtr->value, L"：")))) {
                        error(ERR_CLASS_BEHIND_ACCESS_SHOULD_BE_MAOHAO, tokensPtr->lin);
                        return 255;
                    }
                    if(getNextToken()) {
                        error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                        return 255;
                    }
                    //printf("公有成员：\n");
                    while(1) {
                        //变量成员
                        if(wcsequ(tokensPtr->value, L"var")||wcsequ(tokensPtr->value, L"定义变量")) {
                            //变量名
                            if(getNextToken()) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(!(checkerOutput.checker_class[class_index].pub_sym)) {
                                checkerOutput.checker_class[class_index].pub_sym = (CheckerSymbol*)calloc(1, sizeof(CheckerSymbol));
                                if(!checkerOutput.checker_class[class_index].pub_sym) return -1;
                                checkerOutput.checker_class[class_index].pub_sym_size = 1;
                            }
                            if(pub_sym_index >= checkerOutput.checker_class[class_index].pub_sym_size) {
                                checkerOutput.checker_class[class_index].pub_sym_size = pub_sym_index + 1;
                                void* temp = realloc(checkerOutput.checker_class[class_index].pub_sym, (checkerOutput.checker_class[class_index].pub_sym_size)*sizeof(CheckerSymbol));
                                if(!temp) return -1;
                                checkerOutput.checker_class[class_index].pub_sym = (CheckerSymbol*)temp;
                                checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].name = NULL;
                                checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].type = NULL;
                                //printf("%d\n",checkerOutput.checker_class[class_index].pub_sym_size);
                            }
                            checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].name = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].name, tokensPtr->value);
                            checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].isOnlyRead = false;
                            //printf("name: %ls\taddress: %p\n", checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].name,&(checkerOutput.checker_class[class_index].pub_sym[pub_sym_index]));
                            //变量类型
                            if(getNextToken()) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L":"))&&(!wcsequ(tokensPtr->value, L"："))) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if(getNextToken()) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID && tokensPtr->type != TOK_KW) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].type = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].type, tokensPtr->value);
                            //printf("%ls\n", checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].type);
                            //检查分号
                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L";")) && (!wcsequ(tokensPtr->value, L"；"))) {
                                error(ERR_CLASS_MEMBER_SHOULD_ASSGIN_IN_CONSTRUCT_FUNC, tokensPtr->lin);
                                return 255;
                            }

                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            pub_sym_index++;
                            continue;
                            //常量成员
                        } else if(wcsequ(tokensPtr->value, L"con")||wcsequ(tokensPtr->value, L"定义常量")) {
                            if(getNextToken()) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(!(checkerOutput.checker_class[class_index].pub_sym)) {
                                checkerOutput.checker_class[class_index].pub_sym = (CheckerSymbol*)calloc(1, sizeof(CheckerSymbol));
                                if(!checkerOutput.checker_class[class_index].pub_sym) return -1;
                                checkerOutput.checker_class[class_index].pub_sym_size = 1;
                            }
                            if(pub_sym_index >= checkerOutput.checker_class[class_index].pub_sym_size) {
                                checkerOutput.checker_class[class_index].pub_sym_size = pub_sym_index + 1;
                                void* temp = realloc(checkerOutput.checker_class[class_index].pub_sym, (checkerOutput.checker_class[class_index].pub_sym_size)*sizeof(CheckerSymbol));
                                if(!temp) return -1;
                                checkerOutput.checker_class[class_index].pub_sym = (CheckerSymbol*)temp;
                                checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].name = NULL;
                                checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].type = NULL;
                                //printf("%d\n",checkerOutput.checker_class[class_index].pub_sym_size);
                            }
                            checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].name = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].name, tokensPtr->value);
                            checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].isOnlyRead = true;
                            //printf("name: %ls\taddress: %p\n", checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].name,&(checkerOutput.checker_class[class_index].pub_sym[pub_sym_index]));
                            //常量类型
                            if(getNextToken()) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L":"))&&(!wcsequ(tokensPtr->value, L"："))) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if(getNextToken()) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID && tokensPtr->type != TOK_KW) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].type = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].type, tokensPtr->value);
                            //printf("%ls\n", checkerOutput.checker_class[class_index].pub_sym[pub_sym_index].type);
                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L";")) && (!wcsequ(tokensPtr->value, L"；"))) {
                                error(ERR_CLASS_MEMBER_SHOULD_ASSGIN_IN_CONSTRUCT_FUNC, tokensPtr->lin);
                                return 255;
                            }

                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            pub_sym_index++;
                            continue;
                        } else if(wcsequ(tokensPtr->value, L"pri") || wcsequ(tokensPtr->value, L"私有成员")) {
                            goto parsePrivateMember;
                        } else if(wcsequ(tokensPtr->value, L"pro") || wcsequ(tokensPtr->value, L"受保护成员")) {
                            goto parseProtectedMember;
                        } else if(wcsequ(tokensPtr->value, L"pub") || wcsequ(tokensPtr->value, L"公有成员")) {
                            goto parsePublicMember;
                        } else if(wcsequ(tokensPtr->value, L"}")) {
                            isClassEnd = 1;
                            break;
                        }  else {
                            error(ERR_SYNTAX_WHEN_DEFINE_CLASS, tokensPtr->lin);
                            return 255;
                        }
                    }
                    //私有成员
                } else if(wcsequ(tokensPtr->value, L"pri") || wcsequ(tokensPtr->value, L"私有成员")) {
parsePrivateMember:
                    if(getNextToken()) {
                        error(ERR_CLASS_BEHIND_ACCESS_SHOULD_BE_MAOHAO, tokensPtr->lin);
                        return 255;
                    }
                    if((!wcsequ(tokensPtr->value, L":")) && (!(wcsequ(tokensPtr->value, L"：")))) {
                        error(ERR_CLASS_BEHIND_ACCESS_SHOULD_BE_MAOHAO, tokensPtr->lin);
                        return 255;
                    }
                    if(getNextToken()) {
                        error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                        return 255;
                    }
                    //printf("私有成员：\n");
                    while(1) {
                        //变量成员
                        if(wcsequ(tokensPtr->value, L"var")||wcsequ(tokensPtr->value, L"定义变量")) {
                            //变量名
                            if(getNextToken()) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(!(checkerOutput.checker_class[class_index].pri_sym)) {
                                checkerOutput.checker_class[class_index].pri_sym = (CheckerSymbol*)calloc(1, sizeof(CheckerSymbol));
                                if(!checkerOutput.checker_class[class_index].pri_sym) return -1;
                                checkerOutput.checker_class[class_index].pri_sym_size = 1;
                            }
                            if(pri_sym_index >= checkerOutput.checker_class[class_index].pri_sym_size) {
                                checkerOutput.checker_class[class_index].pri_sym_size = pri_sym_index + 1;
                                void* temp = realloc(checkerOutput.checker_class[class_index].pri_sym, (checkerOutput.checker_class[class_index].pri_sym_size)*sizeof(CheckerSymbol));
                                if(!temp) return -1;
                                checkerOutput.checker_class[class_index].pri_sym = (CheckerSymbol*)temp;
                                checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name = NULL;
                                checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type = NULL;
                            }
                            checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name, tokensPtr->value);
                            checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].isOnlyRead = false;
                            //printf("name: %ls\taddress: %p\n", checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name, &(checkerOutput.checker_class[class_index].pri_sym[pri_sym_index]));
                            //变量类型
                            if(getNextToken()) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L":"))&&(!wcsequ(tokensPtr->value, L"："))) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if(getNextToken()) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID && tokensPtr->type != TOK_KW) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type, tokensPtr->value);
                            //printf("%ls\n", checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type);
                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L";")) && (!wcsequ(tokensPtr->value, L"；"))) {
                                error(ERR_CLASS_MEMBER_SHOULD_ASSGIN_IN_CONSTRUCT_FUNC, tokensPtr->lin);
                                return 255;
                            }

                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            pri_sym_index++;
                            continue;
                            //常量成员
                        } else if(wcsequ(tokensPtr->value, L"con")||wcsequ(tokensPtr->value, L"定义常量")) {
                            if(getNextToken()) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(!(checkerOutput.checker_class[class_index].pri_sym)) {
                                checkerOutput.checker_class[class_index].pri_sym = (CheckerSymbol*)calloc(1, sizeof(CheckerSymbol));
                                if(!checkerOutput.checker_class[class_index].pri_sym) return -1;
                                checkerOutput.checker_class[class_index].pri_sym_size = 1;
                            }
                            if(pri_sym_index >= checkerOutput.checker_class[class_index].pri_sym_size) {
                                checkerOutput.checker_class[class_index].pri_sym_size = pri_sym_index + 1;
                                void* temp = realloc(checkerOutput.checker_class[class_index].pri_sym, (checkerOutput.checker_class[class_index].pri_sym_size)*sizeof(CheckerSymbol));
                                if(!temp) return -1;
                                checkerOutput.checker_class[class_index].pri_sym = (CheckerSymbol*)temp;
                                checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name = NULL;
                                checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type = NULL;
                            }
                            checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name, tokensPtr->value);
                            checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].isOnlyRead = true;
                            //printf("name: %ls\taddress: %p\n", checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name, &(checkerOutput.checker_class[class_index].pri_sym[pri_sym_index]));
                            //常量类型
                            if(getNextToken()) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L":"))&&(!wcsequ(tokensPtr->value, L"："))) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if(getNextToken()) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID && tokensPtr->type != TOK_KW) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type, tokensPtr->value);
                            //printf("%ls\n", checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type);
                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L";")) && (!wcsequ(tokensPtr->value, L"；"))) {
                                error(ERR_CLASS_MEMBER_SHOULD_ASSGIN_IN_CONSTRUCT_FUNC, tokensPtr->lin);
                                return 255;
                            }

                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            pri_sym_index++;
                            continue;
                        } else if(wcsequ(tokensPtr->value, L"pri") || wcsequ(tokensPtr->value, L"私有成员")) {
                            goto parsePrivateMember;
                        } else if(wcsequ(tokensPtr->value, L"pro") || wcsequ(tokensPtr->value, L"受保护成员")) {
                            goto parseProtectedMember;
                        } else if(wcsequ(tokensPtr->value, L"pub") || wcsequ(tokensPtr->value, L"公有成员")) {
                            goto parsePublicMember;
                        } else if(wcsequ(tokensPtr->value, L"}")) {
                            isClassEnd = 1;
                            break;
                        }  else {
                            error(ERR_SYNTAX_WHEN_DEFINE_CLASS, tokensPtr->lin);
                            return 255;
                        }
                    }
                    //受保护成员
                } else if(wcsequ(tokensPtr->value, L"pro") || wcsequ(tokensPtr->value, L"受保护成员")) {
parseProtectedMember:
                    if(getNextToken()) {
                        error(ERR_CLASS_BEHIND_ACCESS_SHOULD_BE_MAOHAO, tokensPtr->lin);
                        return 255;
                    }
                    if((!wcsequ(tokensPtr->value, L":")) && (!(wcsequ(tokensPtr->value, L"：")))) {
                        error(ERR_CLASS_BEHIND_ACCESS_SHOULD_BE_MAOHAO, tokensPtr->lin);
                        return 255;
                    }
                    if(getNextToken()) {
                        error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                        return 255;
                    }
                    //printf("受保护成员：\n");
                    while(1) {
                        //变量成员
                        if(wcsequ(tokensPtr->value, L"var")||wcsequ(tokensPtr->value, L"定义变量")) {
                            //变量名
                            if(getNextToken()) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(!(checkerOutput.checker_class[class_index].pro_sym)) {
                                checkerOutput.checker_class[class_index].pro_sym = (CheckerSymbol*)calloc(1, sizeof(CheckerSymbol));
                                if(!checkerOutput.checker_class[class_index].pro_sym) return -1;
                                checkerOutput.checker_class[class_index].pro_sym_size = 1;
                            }
                            if(pro_sym_index >= checkerOutput.checker_class[class_index].pro_sym_size) {
                                checkerOutput.checker_class[class_index].pro_sym_size = pro_sym_index + 1;
                                void* temp = realloc(checkerOutput.checker_class[class_index].pro_sym, (checkerOutput.checker_class[class_index].pro_sym_size)*sizeof(CheckerSymbol));
                                if(!temp) return -1;
                                checkerOutput.checker_class[class_index].pro_sym = (CheckerSymbol*)temp;
                                checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].name = NULL;
                                checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].type = NULL;
                            }
                            checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].name = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].name, tokensPtr->value);
                            checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].isOnlyRead = false;
                            //printf("name: %ls\taddress: %p\n", checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].name, &(checkerOutput.checker_class[class_index].pro_sym[pro_sym_index]));
                            //变量类型
                            if(getNextToken()) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L":"))&&(!wcsequ(tokensPtr->value, L"："))) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if(getNextToken()) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID && tokensPtr->type != TOK_KW) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].type = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].type, tokensPtr->value);
                            //printf("%ls\n", checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].type);
                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L";")) && (!wcsequ(tokensPtr->value, L"；"))) {
                                error(ERR_CLASS_MEMBER_SHOULD_ASSGIN_IN_CONSTRUCT_FUNC, tokensPtr->lin);
                                return 255;
                            }

                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            pro_sym_index++;
                            continue;
                            //常量成员
                        } else if(wcsequ(tokensPtr->value, L"con")||wcsequ(tokensPtr->value, L"定义常量")) {
                            if(getNextToken()) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(!(checkerOutput.checker_class[class_index].pro_sym)) {
                                checkerOutput.checker_class[class_index].pro_sym = (CheckerSymbol*)calloc(1, sizeof(CheckerSymbol));
                                if(!checkerOutput.checker_class[class_index].pro_sym) return -1;
                                checkerOutput.checker_class[class_index].pro_sym_size = 1;
                            }
                            if(pro_sym_index >= checkerOutput.checker_class[class_index].pro_sym_size) {
                                checkerOutput.checker_class[class_index].pro_sym_size = pro_sym_index + 1;
                                void* temp = realloc(checkerOutput.checker_class[class_index].pro_sym, (checkerOutput.checker_class[class_index].pro_sym_size)*sizeof(CheckerSymbol));
                                if(!temp) return -1;
                                checkerOutput.checker_class[class_index].pro_sym = (CheckerSymbol*)temp;
                                checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].name = NULL;
                                checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].type = NULL;
                            }
                            checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].name = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].name, tokensPtr->value);
                            checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].isOnlyRead = true;
                            //printf("name: %ls\taddress: %p\n", checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].name, &(checkerOutput.checker_class[class_index].pro_sym[pro_sym_index]));
                            //常量类型
                            if(getNextToken()) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L":"))&&(!wcsequ(tokensPtr->value, L"："))) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if(getNextToken()) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID && tokensPtr->type != TOK_KW) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].type = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].type, tokensPtr->value);
                            //printf("%ls\n", checkerOutput.checker_class[class_index].pro_sym[pro_sym_index].type);
                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L";")) && (!wcsequ(tokensPtr->value, L"；"))) {
                                error(ERR_CLASS_MEMBER_SHOULD_ASSGIN_IN_CONSTRUCT_FUNC, tokensPtr->lin);
                                return 255;
                            }

                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            pro_sym_index++;
                            continue;
                        } else if(wcsequ(tokensPtr->value, L"pri") || wcsequ(tokensPtr->value, L"私有成员")) {
                            goto parsePrivateMember;
                        } else if(wcsequ(tokensPtr->value, L"pro") || wcsequ(tokensPtr->value, L"受保护成员")) {
                            goto parseProtectedMember;
                        } else if(wcsequ(tokensPtr->value, L"pub") || wcsequ(tokensPtr->value, L"公有成员")) {
                            goto parsePublicMember;
                        } else if(wcsequ(tokensPtr->value, L"}")) {
                            isClassEnd = 1;
                            break;
                        }  else {
                            error(ERR_SYNTAX_WHEN_DEFINE_CLASS, tokensPtr->lin);
                            return 255;
                        }
                    }
                    //默认私有成员
                } else {
                    //printf("私有成员：\n");
                    while(1) {
                        //变量成员
                        if(wcsequ(tokensPtr->value, L"var")||wcsequ(tokensPtr->value, L"定义变量")) {
                            //变量名
                            if(getNextToken()) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(!(checkerOutput.checker_class[class_index].pri_sym)) {
                                checkerOutput.checker_class[class_index].pri_sym = (CheckerSymbol*)calloc(1, sizeof(CheckerSymbol));
                                if(!checkerOutput.checker_class[class_index].pri_sym) return -1;
                                checkerOutput.checker_class[class_index].pri_sym_size = 1;
                            }
                            if(pri_sym_index >= checkerOutput.checker_class[class_index].pri_sym_size) {
                                checkerOutput.checker_class[class_index].pri_sym_size = pri_sym_index + 1;
                                void* temp = realloc(checkerOutput.checker_class[class_index].pri_sym, (checkerOutput.checker_class[class_index].pri_sym_size)*sizeof(CheckerSymbol));
                                if(!temp) return -1;
                                checkerOutput.checker_class[class_index].pri_sym = (CheckerSymbol*)temp;
                                checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name = NULL;
                                checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type = NULL;
                            }
                            checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name, tokensPtr->value);
                            checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].isOnlyRead = false;
                            //printf("name: %ls\taddress: %p\n", checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name, &(checkerOutput.checker_class[class_index].pri_sym[pri_sym_index]));
                            //变量类型
                            if(getNextToken()) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L":"))&&(!wcsequ(tokensPtr->value, L"："))) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if(getNextToken()) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID && tokensPtr->type != TOK_KW) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type, tokensPtr->value);
                            //printf("%ls\n", checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type);
                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L";")) && (!wcsequ(tokensPtr->value, L"；"))) {
                                error(ERR_CLASS_MEMBER_SHOULD_ASSGIN_IN_CONSTRUCT_FUNC, tokensPtr->lin);
                                return 255;
                            }

                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            pri_sym_index++;
                            continue;
                            //常量成员
                        } else if(wcsequ(tokensPtr->value, L"con")||wcsequ(tokensPtr->value, L"定义常量")) {
                            if(getNextToken()) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID) {
                                error(ERR_NO_VAR_NAME, tokensPtr->lin);
                                return 255;
                            }
                            if(!(checkerOutput.checker_class[class_index].pri_sym)) {
                                checkerOutput.checker_class[class_index].pri_sym = (CheckerSymbol*)calloc(1, sizeof(CheckerSymbol));
                                if(!checkerOutput.checker_class[class_index].pri_sym) return -1;
                                checkerOutput.checker_class[class_index].pri_sym_size = 1;
                            }
                            if(pri_sym_index >= checkerOutput.checker_class[class_index].pri_sym_size) {
                                checkerOutput.checker_class[class_index].pri_sym_size = pri_sym_index + 1;
                                void* temp = realloc(checkerOutput.checker_class[class_index].pri_sym, (checkerOutput.checker_class[class_index].pri_sym_size)*sizeof(CheckerSymbol));
                                if(!temp) return -1;
                                checkerOutput.checker_class[class_index].pri_sym = (CheckerSymbol*)temp;
                                checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name = NULL;
                                checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type = NULL;
                            }
                            checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name, tokensPtr->value);
                            checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].isOnlyRead = true;
                            //printf("name: %ls\taddress: %p\n", checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].name, &(checkerOutput.checker_class[class_index].pri_sym[pri_sym_index]));
                            //常量类型
                            if(getNextToken()) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L":"))&&(!wcsequ(tokensPtr->value, L"："))) {
                                error(ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO, tokensPtr->lin);
                                return 255;
                            }
                            if(getNextToken()) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            if(tokensPtr->type != TOK_ID && tokensPtr->type != TOK_KW) {
                                error(ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE, tokensPtr->lin);
                                return 255;
                            }
                            checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type = (wchar*)calloc(wcslen(tokensPtr->value)+1, sizeof(wchar));
                            wcscpy(checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type, tokensPtr->value);
                            //printf("%ls\n", checkerOutput.checker_class[class_index].pri_sym[pri_sym_index].type);
                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            if((!wcsequ(tokensPtr->value, L";")) && (!wcsequ(tokensPtr->value, L"；"))) {
                                error(ERR_CLASS_MEMBER_SHOULD_ASSGIN_IN_CONSTRUCT_FUNC, tokensPtr->lin);
                                return 255;
                            }

                            if(getNextToken()) {
                                error(ERR_HUAKUOHAO_NOT_CLOSE, tokensPtr->lin);
                                return 255;
                            }
                            pri_sym_index++;
                            continue;
                        } else if(wcsequ(tokensPtr->value, L"pri") || wcsequ(tokensPtr->value, L"私有成员")) {
                            goto parsePrivateMember;
                        } else if(wcsequ(tokensPtr->value, L"pro") || wcsequ(tokensPtr->value, L"受保护成员")) {
                            goto parseProtectedMember;
                        } else if(wcsequ(tokensPtr->value, L"pub") || wcsequ(tokensPtr->value, L"公有成员")) {
                            goto parsePublicMember;
                        } else if(wcsequ(tokensPtr->value, L"}")) {
                            isClassEnd = 1;
                            break;
                        }  else {
                            error(ERR_SYNTAX_WHEN_DEFINE_CLASS, tokensPtr->lin);
                            return 255;
                        }
                    }
                }
            }
            class_index++;
        }
    }
    getNextToken_index = 0;
    return 0;
}
void freeCheckerOutput() {
    if(checkerOutput.checker_func) {
        for(int i = 0; i < checkerOutput.func_size; i++) {
            if(checkerOutput.checker_func[i].name) {
                free(checkerOutput.checker_func[i].name);
                checkerOutput.checker_func[i].name = NULL;
            }
            if(checkerOutput.checker_func[i].ret_type) {
                free(checkerOutput.checker_func[i].ret_type);
                checkerOutput.checker_func[i].ret_type = NULL;
            }
            if(checkerOutput.checker_func[i].args) {
                for(int j = 0; j < checkerOutput.checker_func[i].args_size; j++) {
                    if(checkerOutput.checker_func[i].args[j].name) {
                        free(checkerOutput.checker_func[i].args[j].name);
                        checkerOutput.checker_func[i].args[j].name = NULL;
                    }
                    if(checkerOutput.checker_func[i].args[j].type) {
                        free(checkerOutput.checker_func[i].args[j].type);
                        checkerOutput.checker_func[i].args[j].type = NULL;
                    }
                }
                free(checkerOutput.checker_func[i].args);
            }
            if(checkerOutput.checker_func[i].body) {
                for(int j = 0; j < checkerOutput.checker_func[i].body_size; j++) {
                    if(checkerOutput.checker_func[i].body[j].value) {
                        free(checkerOutput.checker_func[i].body[j].value);
                        checkerOutput.checker_func[i].body[j].value = NULL;
                    }
                }
                free(checkerOutput.checker_func[i].body);
            }
        }
        free(checkerOutput.checker_func);
        checkerOutput.checker_func = NULL;
    }
    if(checkerOutput.global_sym) {
        for(int i = 0; i < checkerOutput.global_sym_size; i++) {
            if(checkerOutput.global_sym[i].name) {
                free(checkerOutput.global_sym[i].name);
                checkerOutput.global_sym[i].name = NULL;
            }
            if(checkerOutput.global_sym[i].type) {
                free(checkerOutput.global_sym[i].type);
                checkerOutput.global_sym[i].type = NULL;
            }
        }
        free(checkerOutput.global_sym);
        checkerOutput.global_sym = NULL;
    }
    if(checkerOutput.checker_class) {
        for(int i = 0; i < checkerOutput.checker_class_size; i++) {
            if(checkerOutput.checker_class[i].name) {
                free(checkerOutput.checker_class[i].name);
                checkerOutput.checker_class[i].name = NULL;
            }
            if(checkerOutput.checker_class[i].parent_class_name) {
                free(checkerOutput.checker_class[i].parent_class_name);
                checkerOutput.checker_class[i].parent_class_name = NULL;
            }
            if(checkerOutput.checker_class[i].pub_sym) {
                for(int j = 0; j < checkerOutput.checker_class[i].pub_sym_size; j++) {
                    if(checkerOutput.checker_class[i].pub_sym[j].name) {
                        free(checkerOutput.checker_class[i].pub_sym[j].name);
                        checkerOutput.checker_class[i].pub_sym[j].name = NULL;
                    }
                    if(checkerOutput.checker_class[i].pub_sym[j].type) {
                        free(checkerOutput.checker_class[i].pub_sym[j].type);
                        checkerOutput.checker_class[i].pub_sym[j].type = NULL;
                    }
                }
            }
            free(checkerOutput.checker_class[i].pub_sym);
            checkerOutput.checker_class[i].pub_sym = NULL;

            if(checkerOutput.checker_class[i].pri_sym) {
                for(int j = 0; j < checkerOutput.checker_class[i].pri_sym_size; j++) {
                    if(checkerOutput.checker_class[i].pri_sym[j].name) {
                        free(checkerOutput.checker_class[i].pri_sym[j].name);
                        checkerOutput.checker_class[i].pri_sym[j].name = NULL;
                    }
                    if(checkerOutput.checker_class[i].pri_sym[j].type) {
                        free(checkerOutput.checker_class[i].pri_sym[j].type);
                        checkerOutput.checker_class[i].pri_sym[j].type = NULL;
                    }
                }
            }
            free(checkerOutput.checker_class[i].pri_sym);
            checkerOutput.checker_class[i].pri_sym = NULL;

            if(checkerOutput.checker_class[i].pro_sym) {
                for(int j = 0; j < checkerOutput.checker_class[i].pro_sym_size; j++) {
                    if(checkerOutput.checker_class[i].pro_sym[j].name) {
                        free(checkerOutput.checker_class[i].pro_sym[j].name);
                        checkerOutput.checker_class[i].pro_sym[j].name = NULL;
                    }
                    if(checkerOutput.checker_class[i].pro_sym[j].type) {
                        free(checkerOutput.checker_class[i].pro_sym[j].type);
                        checkerOutput.checker_class[i].pro_sym[j].type = NULL;
                    }
                }
            }
            free(checkerOutput.checker_class[i].pro_sym);
            checkerOutput.checker_class[i].pro_sym = NULL;
        }
        free(checkerOutput.checker_class);
        checkerOutput.checker_class = NULL;
    }
    return;
}
void error(ErrorType e, int lin) {
    initLocale();
    switch(e) {
    case ERR_FUN_NAME_SHOULD_BEHIND_FUN: {
        fwprintf(stderr, L"\33[31m[E]“定义函数”或“fun”关键字后应为函数名(标识符)！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_FUN_QUITE_SHOULD_BEHIND_NAME: {
        fwprintf(stderr, L"\33[31m[E]函数名后应为括号(运算符)！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_FUN_BEHIND_QUITE_SHOULD_BE_VOID_OR_ARGS: {
        fwprintf(stderr, L"\33[31m[E]开括号后应为参数或关键字“无参数”或“void”！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_QUITE_NOT_CLOSE: {
        fwprintf(stderr, L"\33[31m[E]括号未正确开闭！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_FUN_ARG_NAME_MUST_BE_ID: {
        fwprintf(stderr, L"\33[31m[E]参数名必须是标识符！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_FUN_MAOHAO_SHOULD_BEHIND_ARG_NAME: {
        fwprintf(stderr, L"\33[31m[E]参数名后应为冒号！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_BEHIND_MAOHAO_SHOULD_BE_TYPE: {
        fwprintf(stderr, L"\33[31m[E]冒号后应为类型名,类型名只能是标识符或部分关键字！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_FUN_BEHIND_DOUHAO_SHOULD_BE_NEXT_ARG: {
        fwprintf(stderr, L"\33[31m[E]逗号后应为下一参数！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_FUN_BEHIND_QUITE_SHOULD_BE_MAOHAO: {
        fwprintf(stderr, L"\33[31m[E]闭括号后面应为冒号及其返回类型！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_FUN_BEHIND_MAOHAO_SHOULD_BE_RET_TYPE: {
        fwprintf(stderr, L"\33[31m[E]闭括号后的冒号后应为返回类型,返回类型只能是标识符或部分关键字！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_FUN_BEHIND_RET_TYPE_SHOULD_BE_HUAKUOHAO: {
        fwprintf(stderr, L"\33[31m[E]返回类型后应为花括号！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_FUN_CONNOT_DEFIND_FUN_IN_ANOTHER_FUN_S_BODY: {
        fwprintf(stderr, L"\33[31m[E]不可在一个函数的函数体内定义另一函数！(位于第%d行)\33[0m\n\33[33m[N]有可能是您在定义上个函数时,花括号没有正确闭合。\33[0m\n", lin);
    }
    break;

    case ERR_HUAKUOHAO_NOT_CLOSE: {
        fwprintf(stderr, L"\33[31m[E]花括号未正确开闭！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_FUN_REPEAT_DEFINED: {
        fwprintf(stderr, L"\33[31m[E]函数重复定义！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_CLASS_BEHIND_CLASS_SHOULD_BE_NAME: {
        fwprintf(stderr, L"\33[31m[E]关键字“class”或“定义类”后应为类名(标识符)！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_CLASS_BEHIND_NAME_MUST_BE_HUAKUOHAO: {
        fwprintf(stderr, L"\33[31m[E]类名或父类名后必须是花括号！(位于第%d行)\33[0m\n\33[33m[N]HxLanguage不支持多继承,因此只能继承一个类\33[0m\n", lin);
    }
    break;

    case ERR_CLASS_BEHIND_ACCESS_SHOULD_BE_MAOHAO: {
        fwprintf(stderr, L"\33[31m[E]访问权限修饰符后缺少冒号！(位于第%d行)\33[0m\n", lin-1);
    }
    break;

    case ERR_NO_VAR_NAME: {
        fwprintf(stderr, L"\33[31m[E]关键字“var”或“定义变量”后应为变量名(标识符)！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_BEHIND_SYMBOL_SHOULD_BE_MAOHAO: {
        fwprintf(stderr, L"\33[31m[E]定义变量或常量时,变量或常量符号后应为冒号！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_CLASS_MEMBER_SHOULD_ASSGIN_IN_CONSTRUCT_FUNC: {
        fwprintf(stderr, L"\33[31m[E]类成员应在构造方法中进行初始化！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_SYNTAX_WHEN_DEFINE_CLASS: {
        fwprintf(stderr, L"\33[31m[E]定义类的语法错误！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_CLASS_NO_PARENT_CLASS_NAME: {
        fwprintf(stderr, L"\33[31m[E]定义派生类时缺少父类名！(位于第%d行)\33[0m\n\33[33m[N]父类名应为标识符\33[0m\n", lin);
    }
    break;

    case ERR_CLASS_BEHIND_PARENT_KEYWORD_MUST_BE_MAOHAO: {
        fwprintf(stderr, L"\33[31m[E]定义派生类时,关键字“它的父类是”或“parent”后必须是冒号！(位于第%d行)\33[0m\n", lin);
    }
    break;
    }
    return;
}
int initCheckerOutput() {
    if(!(checkerOutput.checker_func)) {
        checkerOutput.checker_func = (CheckerFunction*)calloc(1, sizeof(CheckerFunction));
        if(!(checkerOutput.checker_func)) return -1;
        checkerOutput.func_size = 1;
    }
    if(!(checkerOutput.global_sym)) {
        checkerOutput.global_sym = (CheckerSymbol*)calloc(1, sizeof(CheckerSymbol));
        if(!(checkerOutput.global_sym)) return -1;
        checkerOutput.global_sym_size = 1;
    }
    if(!(checkerOutput.checker_class)) {
        checkerOutput.checker_class = (CheckerClass*)calloc(1, sizeof(CheckerClass));
        if(!(checkerOutput.checker_class)) return -1;
        checkerOutput.checker_class_size = 1;
    }
    return 0;
}
int isFunctionRepeat(CheckerFunction* fun1, CheckerFunction* fun2) {
    if(fun1 == NULL || fun2 == NULL) return 0;
    if(wcsequ(fun1->name, fun2->name)) {
        if(fun1->args_size == fun2->args_size) {
            if(fun1->args == NULL) {
                return 1;
            } else {
                for(int i = 0; i < fun1->args_size; i++) {
                    if(!wcsequ(fun1->args[i].type, fun2->args[i].type)) return 0;
                }
                return 1;
            }
        }
    }
    return 0;
}
#endif