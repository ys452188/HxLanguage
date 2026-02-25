#define HX_DEBUG
#define HXVM_VERSION 0.114f
#define ERR_LABEL L"\33[1;31m[E]\33[0m"
#define LOG_LABEL L"\33[1;33m[LOG]\33[0m"
#define INFO_LABEL L"\33[1;34m[INFO]\33[0m"

#define CALL_DEPTH_MAX ((const unsigned int)2000)  // 允许递归调用的最多次数
typedef unsigned char byte;
void initLocale(void);
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <iostream>
#include <thread>
FILE* errorStream = NULL;

#include "Interpreter.h"
#include "ObjectReader.h"
extern int interpret(ObjectCode&, int& err);
int main(int argc, char** argv) {
    clock_t start, end;
    start = clock();
    initLocale();
    wprintf(INFO_LABEL L"开始\n");
    errorStream = stdout;
    ObjectCode objCode = {};
    if (readObjectCode("../out.hxo", objCode)) {
        fwprintf(errorStream, ERR_LABEL L"打开文件时发生错误！\n");
        return -1;
    }
#ifdef HX_DEBUG
    wprintf(LOG_LABEL L"读取%d个过程\n", objCode.procedureSize);
    wprintf(LOG_LABEL L"入口索引：%d\n", objCode.start);
#endif

    int err = 0;
    std::thread mainThread(interpret, std::ref(objCode), std::ref(err));
    if (mainThread.joinable()) {
        mainThread.join();
    }
    if (err) {
        end = clock();
        fwprintf(errorStream, ERR_LABEL L"运行时发生异常！共存活%lfs\n",
                 (double)(end - start) / CLOCKS_PER_SEC);
        return -1;
    }

    end = clock();
    wprintf(INFO_LABEL L"结束 耗时%lfs\n",
            (double)(end - start) / CLOCKS_PER_SEC);
    return 0;
}
void initLocale(void) {
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