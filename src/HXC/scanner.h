#ifndef SCANNER_H
#define SCANNER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>
#include <locale.h>
#include "lexer.h"
wchar_t* getData(const char*);
wchar_t* charToWchar(const char* narrow_str) {
    setlocale(LC_ALL, ""); // 使用系统默认locale
    size_t required_size = mbstowcs(NULL, narrow_str, 0) + 1;
    if(required_size == (size_t)-1) {
        // 转换失败（无效的多字节序列）
        return NULL;
    }
    wchar_t* wide_str = (wchar_t*)malloc(required_size * sizeof(wchar_t));
    if(!wide_str) {
        return NULL;
    }
    if(mbstowcs(wide_str, narrow_str, required_size) == (size_t)-1) {
        free(wide_str);
        return NULL;
    }
    return wide_str;
}
/*从文件获取数据*/
wchar_t* getData(const char* path) {
    /*Windows 下使用 UTF-8 编码*/
#ifdef _WIN32
    FILE* fp = fopen(path, "r,ccs=UTF-8");
    setlocale(LC_ALL,"zh_CN.UTF-8");
#else
    FILE* fp = fopen(path, "r");
#endif
    if (fp == NULL) {
#ifdef _WIN32
        wchar_t* path_wcs = charToWchar(path);
        wchar_t* err_wcs = charToWchar(strerror(errno));
        fwprintf(stderr, L"\033[31m[E]错误：无法打开文件 %ls (%ls)\033[0m\n", path_wcs, err_wcs);
        free(path_wcs);
        free(err_wcs);
#else
        fprintf(stderr, "\033[31m[E]错误：无法打开文件 %s (%s)\033[0m\n", path, strerror(errno));
#endif
        return NULL;
    }

    int capacity = 8;
    int size = capacity;
    wchar_t* data = (wchar_t*)malloc(sizeof(wchar_t) * size);
    if (!data) {
        fclose(fp);
#ifndef _WIN32
        fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
        fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
        return NULL;
    }
    wchar_t ch;
    size_t index = 0;
    while ((ch = fgetwc(fp)) != WEOF) {
        if (index >= size) {
            void* temp = realloc(data, capacity * 2 * sizeof(wchar_t));
            if (!temp) {
                fclose(fp);
                free(data);
#ifndef _WIN32
                fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
                fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
                return NULL;
            }
            data = (wchar_t*)temp;
            size = capacity * 2;
            capacity *= 2;
        }
        data[index++] = ch;
    }
    /*确保终止符空间*/
    if (index >= size) {
        void* temp = realloc(data, (index + 1) * sizeof(wchar_t));
        if (!temp) {
            fclose(fp);
            free(data);
#ifndef _WIN32
            fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
            fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
            return NULL;
        }
        data = (wchar_t*)temp;
    }
    data[index] = L'\0';
    /*处理 BOM（Windows 下已自动跳过 UTF-8 BOM）*/
#ifndef _WIN32
    if (index >= 3 && data[0] == 0xEFBB && data[1] == 0xBF00) { /*UTF-8 BOM 的宽字符表示*/
        memmove(data, data + 3, (index - 3) * sizeof(wchar_t));
        index -= 3;
        data[index] = L'\0';
    }
#endif
    fclose(fp);
    return data;
}
#endif