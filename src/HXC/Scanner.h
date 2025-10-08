#ifndef HXLANG_SCANNER_H
#define HXLANG_SCANNER_H
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <windows.h>
#ifdef _WIN32
int readSourceFile(const char* path, wchar_t** src) {
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    char* buf = malloc(size + 1);
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);

    // 跳过 BOM（如果存在）
    if ((unsigned char)buf[0] == 0xEF &&
            (unsigned char)buf[1] == 0xBB &&
            (unsigned char)buf[2] == 0xBF) {
        buf += 3;
        size -= 3;
    }

    // 使用 MultiByteToWideChar 进行字符编码转换
    int wlen = MultiByteToWideChar(CP_UTF8, 0, buf, size, NULL, 0);
    *src = malloc((wlen + 1) * sizeof(wchar_t));
    if (*src == NULL) {
        free(buf);
        return -1;
    }
    MultiByteToWideChar(CP_UTF8, 0, buf, size, *src, wlen);
    (*src)[wlen] = L'\0'; // 确保字符串以L'\0'结尾
    free(buf);
    return 0;
}
#else
int readSourceFile(char* path, wchar_t** src) {
    if (!path || !src) return -1;
    FILE* fp = fopen(path, "r");
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
#endif