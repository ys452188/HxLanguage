#define HX_DEBUG
#define OP_STACK_SIZE 512  // 操作数栈大小
#define HXVM_VERSION 0.114f
#define ERR_LABEL L"\33[1;31m[E]\33[0m"
#define LOG_LABEL L"\33[1;33m[LOG]\33[0m"
#define INFO_LABEL L"\33[1;34m[INFO]\33[0m"
#define CALL_DEPTH_MAX ((const unsigned int)2000)  // 允许递归调用的最多次数
typedef unsigned char byte;
inline void initLocale(void);
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "HxVector.h"
#define errorStream stdout

#include <pthread.h>

#include <iostream>

#include "Interpreter.h"
#include "ObjectReader.h"
extern int interpret(ObjectCode&, int& err) noexcept;
typedef struct InterpretData {
    int* err;
    int* ret;
    ObjectCode* obj;
} InterpretData;
void* interpret_packed(void* dataPtr) {
    InterpretData data = *((InterpretData*)dataPtr);
    *(data.ret) = interpret(*(data.obj), *(data.err));
    return NULL;
}
int main(int argc, char** argv) {
    clock_t start, end;
    start = clock();
    initLocale();
    wprintf(INFO_LABEL L"开始\n");

    memoryAllocer;
    ObjectCode objCode = {};
    std::string path = "../out.hxo";
    FILE* file = fopen(path.c_str(), "rb");
    // 读
    if (readObjectCode(file, objCode)) {
        fwprintf(errorStream, ERR_LABEL L"打开文件时发生错误！\n");
        return -1;
    }
#ifdef HX_DEBUG
    wprintf(LOG_LABEL L"读取%d个过程\n", objCode.procedureSize);
    wprintf(LOG_LABEL L"入口索引：%d\n", objCode.start);
#endif

    int err = 0;
    int ret = 0;
    pthread_t mainThread = {};
    InterpretData data = {};
    data.err = &err;
    data.obj = &objCode;
    data.ret = &ret;
    if (pthread_create(&mainThread, NULL, interpret_packed, &data)) {
        fwprintf(errorStream, ERR_LABEL L"寄！线程创建失败！共存活%lfs\n",
                 (double)(end - start) / CLOCKS_PER_SEC);
    }
    pthread_join(mainThread, NULL);
    if (err || ret) {
        end = clock();
        fwprintf(errorStream, ERR_LABEL L"寄！运行时发生异常！共存活%lfs\n",
                 (double)(end - start) / CLOCKS_PER_SEC);
        return -1;
    }
    freeObjectCode(objCode);

    end = clock();
    wprintf(INFO_LABEL L"结束 耗时%lfs\n",
            (double)(end - start) / CLOCKS_PER_SEC);
    return 0;
}
void initLocale(void) {
    // 设置Locale
    setlocale(LC_ALL, "C.UTF-8");
    // 设置宽字符流的定向
    fwide(stdout, 1);  // 1 = 宽字符定向
    return;
}