#define HX_DEBUG

typedef unsigned char byte;
#define STACK_SIZE 10240      // 栈大小
#define STACK_FRAME_SIZE 256  // 栈帧数量
#define OP_STACK_SIZE 128     // 操作数栈大小
void initLocale(void);
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
FILE* errorStream = NULL;
#include "VM.h"

int main(int argc, char** argv) {
  initLocale();
  errorStream = stdout;
  VM vm = {0};
  int err = startVM(&vm);
  if (err) {
    fwprintf(errorStream, L"\33[31m异常终止。\33[0m\n");
    return err;
  }
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