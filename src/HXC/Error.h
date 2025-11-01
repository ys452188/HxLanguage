#ifndef HX_ERROR_H
#define HX_ERROR_H
#include <wchar.h>
#include <stdio.h>
#include <locale.h>
#define ERROR_BUF_SIZE 1024
wchar_t errorMessageBuffer[ERROR_BUF_SIZE];     //错误信息缓冲区
typedef enum ErrorType {
    ERR_NO_END,       //语句没结尾
    ERR_CH_NO_END,    //字符没结尾
    ERR_STR_NO_END,   //字符串没结尾
    ERR_VAL,          //字面量写错了
    ERR_BEHIND_CLASS_SHOULD_BE_ID,   //class关键字后应为标识符
    ERR_BEHIND_CLASS_NAME_SHOULD_BE_HUAKUOHAO,  //类名后应为花括号
    ERR_HUAKUOHAO_NOT_CLOSE,         //花括号末正确闭合
    ERR_NO_SYM_NAME,          //定义变量时缺变量名
    ERR_DEF_VAR,             //定义变量语法错误
    ERR_DEF_CLASS,
    ERR_NO_SYM_TYPE,         //定义变量时缺少类型
    ERR_DEF_CLASS_ACCESS,    //定义类时访问权限修饰符使用错误
    ERR_DEF_CLASS_DOUBLE_DEFINED_SYM,     //定义类时重复声明符号
    ERR_NO_FUN_NAME,
    ERR_FUN,
    ERR_FUN_ARG,
    ERR_ARR_TYPE,
    ERR_MAIN,
    ERR_COUNLD_NOT_FIND_PARENT,
    ERR_UNKOWN_TYPE,
    ERR_NO_MAIN,
} ErrorType;

void initLocale(void) {
    //设置Locale
    if (!setlocale(LC_ALL, "zh_CN.UTF-8")) {
        if (!setlocale(LC_ALL, "en_US.UTF-8")) {
            setlocale(LC_ALL, "C.UTF-8");
        }
    }
    //设置宽字符流的定向
    fwide(stdout, 1); // 1 = 宽字符定向
    return;
}
void setError(ErrorType e, int errorLine, wchar_t* errCode) {
    initLocale();
    switch(e) {
    case ERR_NO_END: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]\33[0m语句缺少分号结尾！(位于第%d行)\n \33[36m[NOTE]\33[0m后面应该是分号->\33[4m%ls\33[0m\n \33[36m[NOTE]\33[0m类体中变量或常量符号只能声明,不能赋值。", errorLine, errCode? errCode:L" ");
        break;
    }
    case ERR_CH_NO_END: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]\33[0m字符缺少结尾！(位于第%d行)\n \33[36m[NOTE]\33[0m这个字符缺结尾->%ls\n", errorLine, errCode? errCode:L" ");
        break;
    }
    case ERR_STR_NO_END: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]\33[0m字符串缺少结尾！(位于第%d行)\n \33[36m[NOTE]\33[0m这个字符串缺结尾->%ls\n", errorLine, errCode? errCode:L" ");
        break;
    }

    case ERR_VAL: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]\33[0m字面量错误！(位于第%d行)\n \33[36m[NOTE]\33[0m这个字面量写错了->%ls\n", errorLine, errCode? errCode:L" ");
        break;
    }

    case ERR_BEHIND_CLASS_SHOULD_BE_ID: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]\33[0m“class”或“定义类”关键字后应为类名(标识符)！(位于第%d行)\n \33[36m[NOTE]\33[0m看这儿->%ls\n", errorLine, errCode? errCode:L" ");
        break;
    }

    case ERR_BEHIND_CLASS_NAME_SHOULD_BE_HUAKUOHAO: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]\33[0m类名后应为花括号或逗号！(位于第%d行)\n \33[36m[NOTE]\33[0m看这里->%ls\n", errorLine, errCode? errCode:L" ");
        break;
    }

    case ERR_HUAKUOHAO_NOT_CLOSE: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]花括号未正确闭合\33[0m！(位于第%d行)\n \33[36m[NOTE]\33[0m这个花括号没有对应的闭花括号->%ls\n", errorLine, errCode? errCode:L" ");
        break;
    }

    case ERR_NO_SYM_NAME: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]定义变量或常量时缺少名字！\33[0m(位于第%d行)\n\33[36m[NOTE]\33[0m变量名或常量名必须是标识符。\n", errorLine);
        break;
    }

    case ERR_DEF_VAR: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]定义变量语法错误！\33[0m(位于第%d行)\n\33[36m[NOTE]\33[0mDefineVariable::= <\"var\"><\":\"><id><\"->\"><kw|id>\n     定义变量::= <\"定义变量\"><\":\"><标识符><\",\"><\"它的类型是\"><\":\"><标识符|关键字>\n", errorLine);
        break;
    }

    case ERR_NO_SYM_TYPE: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]定义变量或常量时缺少类型名！\33[0m(位于第%d行)\n\33[36m[NOTE]\33[0m类型名应为标识符或关键字\n", errorLine);
        break;
    }

    case ERR_DEF_CLASS_ACCESS: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]定义类时访问权限修饰符使用错误！\33[0m(位于第%d行)\n\33[36m[NOTE]\33[0m 声明类成员::= \"public\"|\"private\"|\"protected\"|\"公有成员\"|\"私有成员\"|\"受保护成员\" <\"{\"> ... <\"}\">\n", errorLine);
        break;
    }

    case ERR_DEF_CLASS_DOUBLE_DEFINED_SYM: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]定义类时重复声明符号！\33[0m(位于第%d行)\n\33[36m[NOTE]\33[0m 此符号被重复声明-> %ls\n", errorLine, errCode? errCode:L" ");
        break;
    }

    case ERR_DEF_CLASS: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]定义类的语法有误！\33[0m(位于第%d行)\n\33[36m[NOTE]\33[0m 定义类::= <\"class\" | \"定义类\"> <\":\"> <id> [<\",\"> <\"parent\"|\"它的父类是\"> <\":\"> <id>] <\"{\"> ... <\"}\">\n", errorLine);
        break;
    }

    case ERR_NO_FUN_NAME: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]缺少函数名！\33[0m(位于第%d行)\n\33[36m[NOTE]\33[0m DefineFunction::= <\"fun\"><\":\"><id><\"(\">...<\")\">[<\"->\"><id|kw>]<\"{\">...<\"}\">\n定义函数::= <\"定义函数\"><\"：\"><标识符><\"(\">...<\")\">[<\",\"><\"它的返回值是\"><\"：\"><标识符|关键字>]<\"{\">...<\"}\">\n", errorLine);
        break;
    }

    case ERR_FUN: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]定义函数的语法错误！\33[0m(位于第%d行)\n\33[36m[NOTE]\33[0m DefineFunction::= <\"fun\"><\":\"><id><\"(\">...<\")\">[<\"->\"><id|kw>]<\"{\">...<\"}\">\n定义函数::= <\"定义函数\"><\"：\"><标识符><\"(\">...<\")\">[<\",\"><\"它的返回值是\"><\"：\"><标识符|关键字>]<\"{\">...<\"}\">\n\33[36m[NOTE]\33[0m函数体内不可定义函数！\n", errorLine);
        break;
    }

    case ERR_FUN_ARG: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]定义函数的参数的语法错误！\33[0m(位于第%d行)\n\33[36m[NOTE]\33[0m Argument::= <id><\":\"><id|kw>\n参数::= <标识符><\":\"><标识符|关键字>", errorLine);
        break;
    }

    case ERR_ARR_TYPE: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]数组类型拼写错误！\33[0m(位于第%d行)\n\33[36m[NOTE]\33[0m ArrayType::= <id|kw>[<\"[\"><\"]\">...]\n参数::= <标识符|关键字>[<\"[\"><\"]\">...]", errorLine);
        break;
    }

    case ERR_MAIN: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]主函数拼写错误！\33[0m(位于第%d行)\n\33[36m[NOTE]\33[0m 主函数不能重载！\n", errorLine);
        break;
    }

    case ERR_COUNLD_NOT_FIND_PARENT: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]找不到父类！\33[0m(位于第%d行)\n\33[36m[NOTE]\33[0m 这个父类找不到->%ls\n", errorLine,errCode);
        break;
    }

    case ERR_UNKOWN_TYPE: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]未知类型！\33[0m(位于第%d行)\n\33[36m[NOTE]\33[0m 似乎没有这个类型->%ls\n", errorLine,errCode);
        break;
    }

    case ERR_NO_MAIN: {
        swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]缺少主函数！\33[0m\n");
        break;
    }
    }
    return;
}
#endif