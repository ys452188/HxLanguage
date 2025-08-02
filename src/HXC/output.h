#ifndef HXLANG_OUTPUT_H
#define HXLANG_OUTPUT_H
#include <stdio.h>
#include <wchar.h>
#include <locale.h>
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
#endif