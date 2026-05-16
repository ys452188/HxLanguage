#ifndef HXHLANG_SRC_HXC_ERROR_H
#define HXHLANG_SRC_HXC_ERROR_H
#include <locale.h>
#include <stdio.h>
#include <wchar.h>
#define ERROR_BUF_SIZE 1024
wchar_t errorMessageBuffer[ERROR_BUF_SIZE];  // 错误信息缓冲区
typedef enum ErrorType {
    ERR_GLOBAL_UNKOWN,        // 未知的全局定义
    ERR_NO_END,               // 语句没结尾
    ERR_CH_NO_END,            // 字符没结尾
    ERR_STR_NO_END,           // 字符串没结尾
    ERR_VAL,                  // 字面量写错了
    ERR_HUAKUOHAO_NOT_CLOSE,  // 花括号末正确闭合
    ERR_DEF_VAR,              // 定义变量语法错误
    ERR_VAR_REPEATED,         // 变量重复定义
    ERR_DEF_CLASS,
    ERR_DEF_CLASS_ACCESS,              // 定义类时访问权限修饰符使用错误
    ERR_DEF_CLASS_DOUBLE_DEFINED_SYM,  // 定义类时重复声明符号
    ERR_FUN,
    ERR_FUN_ARG,
    ERR_FUN_REPEATED,  // 函数重复定义
    ERR_TYPE,
    ERR_MAIN,
    ERR_COUNLD_NOT_FIND_PARENT,
    ERR_UNKOWN_TYPE,
    ERR_NO_MAIN,
    ERR_CANNOT_FIND_SYMBOL,
    ERR_EXP,
    ERR_OUT_OF_VALUE,    // 数值溢出
    ERR_CLASS_REPEATED,  // 类重复定义
    ERR_RET,             // 返回值错误 （语法错误）
    ERR_RET_VAL,         // 返回值错误
    ERR_UNKNOWN_TYPE,    // 未知类型
    ERR_REPEAT,
    ERR_IF,
    ERROR_UNCOMPLETED_CLASS,  // 类相互包含
} ErrorType;

void initLocale(void) noexcept {
    // 设置Locale
    if (!setlocale(LC_ALL, "zh_CN.UTF-8")) {
        if (!setlocale(LC_ALL, "en_US.UTF-8")) {
            setlocale(LC_ALL, "C.UTF-8");
        }
    }
    // 设置宽字符流的定向
    fwide(stdout, 1);  // 1 = 宽字符定向
    return;
}
void setError(ErrorType e, int errorLine, const wchar_t* errCode) noexcept {
    initLocale();
    switch (e) {
        case ERR_NO_END: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]\33[0m语句缺少分号结尾！(位于第%"
                     L"d行)\n "
                     L"\33[36m[NOTE]\33[0m后面应该是分号->\33[4m%"
                     L"ls\33[0m\n "
                     L"\33[36m[NOTE]\33["
                     L"0m类体中变量或常量符号只能声明,不能赋值。",
                     errorLine, errCode ? errCode : L" ");
            break;
        }
        case ERR_CH_NO_END: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]\33[0m字符缺少结尾！(位于第%d行)\n "
                     L"\33[36m[NOTE]\33[0m这个字符缺结尾->%ls\n",
                     errorLine, errCode ? errCode : L" ");
            break;
        }
        case ERR_STR_NO_END: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]\33[0m字符串缺少结尾！(位于第%d行)\n "
                     L"\33[36m[NOTE]\33[0m这个字符串缺结尾->%ls\n",
                     errorLine, errCode ? errCode : L" ");
            break;
        }

        case ERR_VAL: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]\33[0m字面量错误！(位于第%d行)\n "
                     L"\33[36m[NOTE]\33[0m这个字面量写错了->%ls\n",
                     errorLine, errCode ? errCode : L" ");
            break;
        }

        case ERR_HUAKUOHAO_NOT_CLOSE: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]花括号未正确闭合\33[0m！(位于第%"
                     L"d行)\n "
                     L"\33[36m[NOTE]\33["
                     L"0m这个花括号没有对应的闭花括号->%ls\n",
                     errorLine, errCode ? errCode : L" ");
            break;
        }

        case ERR_DEF_VAR: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]定义变量语法错误！\33[0m(位于第%"
                     L"d行)\n\33[36m["
                     L"NOTE]\33[0mDefineVariable::= "
                     L"<\"var\"><\":\"><id><\":\"><kw|id>\n     "
                     L"定义变量::= "
                     L"<\"定义变量\"><\":\"><标识符><\",\"><"
                     L"\"它的类型是\"><\":\"><"
                     L"标识符|关键字>\n",
                     errorLine);
            break;
        }

        case ERR_DEF_CLASS_ACCESS: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]定义类时访问权限修饰符使用错误！\33["
                     L"0m(位于第%d行)"
                     L"\n\33[36m[NOTE]\33[0m 声明类成员::= "
                     L"\"[public\"|\"private\"|\"protected\"|"
                     L"\"公有成员\"|"
                     L"\"私有成员\"|\"受保护成员\" <\":\"> ] "
                     L"定义函数|声明变量\n",
                     errorLine);
            break;
        }

        case ERR_DEF_CLASS_DOUBLE_DEFINED_SYM: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]定义类时重复声明符号！\33[0m("
                     L"位于第%d行)\n\33[36m["
                     L"NOTE]\33[0m 此符号被重复声明-> %ls\n",
                     errorLine, errCode ? errCode : L" ");
            break;
        }

        case ERR_DEF_CLASS: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]定义类的语法有误！\33[0m(位于第%"
                     L"d行)\n\33[36m[NOTE]\33["
                     L"0m 定义类::= <\"class\" | \"定义类\"> "
                     L"<\":\"> <id> [<\",\"> "
                     L"<\"parent\"|\"它的父类是\"> <\":\"> <id>] "
                     L"<\"{\"> ... <\"}\">\n",
                     errorLine);
            break;
        }

        case ERR_FUN: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]定义函数的语法错误！\33[0m(位于第%"
                     L"d行)\n\33[36m["
                     L"NOTE]\33[0m DefineFunction::= "
                     L"<\"fun\"><\":\"><id><\"(\"><args><\")\">[<\":\"><"
                     L"id|kw>]<\"->\"><"
                     L"\"{\">."
                     L"..<\"}\">\n定义函数::= "
                     L"<\"定义函数\"><\"：\"><标识符><\"(\"><参数><\")"
                     L"\">[<\",\"><"
                     L"\"它的返回值是\"><\"：\"><数据类型>]|[<\",\"><"
                     L"\"它没有返回类型\">]<"
                     L"\"{\">...<\"}\">"
                     L"\n\33[36m[NOTE]\33[0m函数体内不可定义函数！\n",
                     errorLine);
            break;
        }

        case ERR_FUN_ARG: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]定义函数的参数的语法错误！\33["
                     L"0m(位于第%d行)\n\33["
                     L"36m[NOTE]\33[0m Argument::= "
                     L"<id><\":\"><id|kw>\n参数::= "
                     L"<标识符><\":\"><标识符|关键字>",
                     errorLine);
            break;
        }

        case ERR_TYPE: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]类型拼写错误！\33[0m(位于第%d行)"
                     L"\n\33[36m["
                     L"NOTE]\33[0m ArrayType::= "
                     L"<id|kw>[<\"[\"><\"]\">...]\n参数::= "
                     L"<标识符|关键字>[<\"[\"><\"]\">...]",
                     errorLine);
            break;
        }

        case ERR_MAIN: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]主函数错误！\33[0m(位于第%d行)"
                     L"\n\33[36m[NOTE]"
                     L"\33[0m 主函数不能重载！\n",
                     errorLine);
            break;
        }

        case ERR_COUNLD_NOT_FIND_PARENT: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]找不到父类！\33[0m(位于第%d行)"
                     L"\n\33[36m[NOTE]\33["
                     L"0m 这个父类找不到->%ls\n",
                     errorLine, errCode);
            break;
        }

        case ERR_UNKOWN_TYPE: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]未知类型！\33[0m(位于第%d行)"
                     L"\n\33[36m[NOTE]\33[0m "
                     L"似乎没有这个类型->%ls\n",
                     errorLine, errCode);
            break;
        }

        case ERR_NO_MAIN: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]缺少主函数！\33[0m\n");
            break;
        }

        case ERR_CANNOT_FIND_SYMBOL: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]找不到符号(%ls)！\33[0m\n",
                     errCode ? errCode : L"(null)");
            break;
        }

        case ERR_EXP: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]表达式错误(%ls)！\33[0m\n",
                     errCode ? errCode : L"(null)");
            break;
        }

        case ERR_OUT_OF_VALUE: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]啊♡~数值太......太大了......"
                     L"要溢出来了♡......(%ls)"
                     L"\33[0m\n",
                     errCode ? errCode : L"？？？？");
            break;
        }

        case ERR_GLOBAL_UNKOWN: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]未知的全局定义！\33[0m(位于第%d行)\n", errorLine);
            break;
        }

        case ERR_FUN_REPEATED: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]函数重复定义！\33[0m(位于第%d行)\n", errorLine);
            break;
        }

        case ERR_CLASS_REPEATED: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]类重复定义！\33[0m(位于第%d行)\n", errorLine);
            break;
        }

        case ERR_RET: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]返回语句语法错误！\33[0m(位于第%"
                     L"d行)\n\33[36m["
                     L"NOTE]\33[0m 返回::= ret:exp | 返回：exp\n",
                     errorLine);
            break;
        }
        case ERR_UNKNOWN_TYPE: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]这是什么类型喵？  "
                     L"%ls\33[0m(位于第%d行)\n",
                     errCode ? errCode : L" ", errorLine);
            break;
        }
        case ERR_RET_VAL: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE, L"\33[31m[ERR]返回值错误了喵\33[0m(位于第%d行)\n", errorLine);
            break;
        }
        case ERR_REPEAT: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]循环语句错误了喵\33[0m(位于第%d行)"
                     L"\n\33[36m[NOTE]"
                     L"\33[0m 循环 ::= repeat-> 语句|块 [until(exp)]\n",
                     errorLine);
            break;
        }

        case ERR_IF: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]对条件判断语句犯错误了喵～\33["
                     L"0m(位于第%d行)\n\33["
                     L"36m[NOTE]\33[0m 条件判断 ::= if: 表达式 -> "
                     L"语句|块\n",
                     errorLine);
            break;
        }

        case ERR_VAR_REPEATED: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]变量不能重复定义的喵～笨蛋！\33["
                     L"0m(位于第%d行)\n",
                     errorLine);
            break;
        }
        case ERROR_UNCOMPLETED_CLASS: {
            swprintf(errorMessageBuffer, ERROR_BUF_SIZE,
                     L"\33[31m[ERR]杂鱼~你的类相互包含了喵~("
                     L"如：类A里有类B类型成员，而类B里又有类A类型成员)"
                     L"如果不报错，占的内存就太......太大了。电脑会坏..."
                     L"...坏掉了...."
                     L".\33[0m(位于第%d行)\n",
                     errorLine);
            break;
        }
    }
    return;
}

#endif