#ifndef HXLANG_HX_LOCALE_H
#define HXLANG_HX_LOCALE_H
#include <stdio.h>
#include <wchar.h>
#include <locale.h>
typedef wchar_t wchar;
int wcsequ(wchar* str1, wchar* str2) {
    if(!str1 || !str2) return 0;
    int temp1 = 0, temp2 = 0;
    temp1 = wcslen(str1);
    temp2 = wcslen(str2);
    if(temp1 != temp2) return 0;
    int len = temp1;
    for(int i = 0; i < len; i++) {
        if(str1[i] != str2[i]) return 0;
    }
    return 1;
}
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