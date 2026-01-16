#define HX_DEBUG
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>

#endif
#define log(msg, ...) \
  fwprintf(logStream, L"\33[33m[DEB]\33[0m" msg L"\n", ##__VA_ARGS__)
FILE* outputStream = NULL;
FILE* logStream = NULL;
FILE* errorStream = NULL;
#include "Error.h"
#include "IR.h"
#include "Lexer.h"
#include "Scanner.h"

int main(int argc, char* argv[]) {
  initLocale();
  if (argc < 2) {
    fwprintf(stderr, L"\33[31m[ERR]\33[0m请提供源文件路径！\n");
    return -1;
  }
  clock_t start, end;
  start = clock();
  outputStream = stdout;
  logStream = stdout;
  errorStream = stderr;
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_U16TEXT);
  _setmode(_fileno(stderr), _O_U16TEXT);
#endif
  // 读取源文件
  wchar_t* src = NULL;
  int scannerError = readSourceFile(argv[1], &src);
  if (scannerError) {
    fwprintf(outputStream, L"\33[31m[ERR]\33[0m在读取源文件时出错了！\n");
    fwprintf(outputStream, L"\33[31m[ERR]\33[0m编译失败。\n");
    return -1;
  }
#ifdef HX_DEBUG
  fwprintf(outputStream, L"源文件：\n%ls\n", src);
#endif
  if (wcslen(src) == 0) {
    fwprintf(outputStream, L"\33[31m[ERR]\33[0m你的源文件里空空如也！\n");
    fwprintf(outputStream, L"\33[31m[ERR]\33[0m编译失败。\n");
    return 255;
  }
  // 词法分析
  int lexerError = 0;
  fwprintf(outputStream, L"\33[34m[INFO]\33[0m正在进行词法分析\n");
  Tokens* tokens = lex(src, &lexerError);
  if (lexerError == 255) {
    fwprintf(errorStream, L"%ls\n", errorMessageBuffer);
    freeTokens(&tokens);
    fwprintf(outputStream, L"\33[31m[ERR]\33[0m编译失败。\n");
    return 255;
  } else if (lexerError == -1) {
    fwprintf(errorStream, L"\33[31m[ERR]\33[0m内存分配失败！\n");
    freeTokens(&tokens);
    return -1;
  }
  free(src);
  src = NULL;
  fwprintf(outputStream, L"\33[34m[INFO]\33[0m词法分析完成\n");
#ifdef HX_DEBUG
  showTokens(tokens);
#endif
  // 中间表示生成
  IR_Program* program = NULL;
  int irError = 0;
  program = generateIR(tokens, &irError);
  freeTokens(&tokens);
  if (irError == 255) {
    fwprintf(errorStream, L"%ls\n", errorMessageBuffer);
    fwprintf(outputStream, L"\33[31m[ERR]\33[0m编译失败。\n");
    return 255;
  } else if (irError == -1) {
    fwprintf(errorStream, L"\33[31m[ERR]\33[0m内存分配失败！\n");
    return -1;
  }
  fwprintf(outputStream, L"\33[34m[INFO]\33[0m中间表示生成完成\n");
#ifdef HX_DEBUG
  showIRProgramInfo(program);
#endif

  freeIRProgram(&program);
  end = clock();
  fwprintf(outputStream, L"\33[34m[INFO]\33[0m编译完成。共耗时%lfs\n",
           (double)(end - start) / CLOCKS_PER_SEC);
  return 0;
}