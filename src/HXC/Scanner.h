#ifndef HXLANG_SCANNER_H
#define HXLANG_SCANNER_H
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>

int readSourceFile(char* path, wchar_t** src) {
    if (!path || !src) return -1;
    FILE* fp = fopen(path, "r, css=UTF-8");
    if (fp == NULL) return -1;

    unsigned int index = 0;
    unsigned int size = 16;
    *src = (wchar_t*)malloc(size * sizeof(wchar_t));
    if (!*src) {
        fclose(fp);
        return -1;
    }
    wchar_t ch;
    while ((ch = fgetwc(fp)) != WEOF) {
        if (index + 1 >= size) { // +1 留出 '\0'
            size *= 2;
            wchar_t* temp = (wchar_t*)realloc(*src, size * sizeof(wchar_t));
            if (!temp) {
                free(*src);
                fclose(fp);
                return -1;
            }
            *src = temp;
        }
        (*src)[index++] = ch;
    }

    (*src)[index] = L'\0';
    fclose(fp);
    return 0;
}
#endif