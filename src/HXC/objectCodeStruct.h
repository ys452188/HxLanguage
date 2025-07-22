#ifndef OBJECT_CODE_STRUCT_H
#define OBJECT_CODE_STRUCT_H
#include <stdlib.h>
#include <wchar.h>
#include <stdbool.h>
#include "lexer.h"
/*******************/
typedef uint8_t i8;
typedef struct FileHeader {
    i8 magicNumber;
    float version;
} FileHeader;
typedef enum {
    TOK_ADD,    //加
    TOK_MIN,    //减
    TOK_EQU,    //等
    TOK_DIV,    //除
    TOK_MUL,    //乘
    TOK_GRE,    //大于
    TOK_LES,    //小于
    TOK_LOE,    //小于或等于
    TOK_GOE,    //大于或等于
    TOK_SEI,    //自增
    TOK_SER,    //自减
    TOK_ASG,    //赋值
    TOK_CAL,    //调用
    TOK_VAR,    //定义变量
    TOK_CON,    //定义常量
} Opr;
typedef struct ObjectToken {
    TokenType type;
    union {
        wchar_t* val;         // 值类型使用
        Opr opr;              // 操作符类型（使用枚举值）
    } value;
    bool owns_memory;         // 标记是否需要释放val的内存
} ObjectToken;
typedef struct Variable {
    wchar_t* name;
    wchar_t* type;
    bool isOnlyRead;
} Variable;
//语句
typedef struct Sentence {
    ObjectToken* tokens;
    int length;
} Sentence;
typedef struct SymTable {
    Variable* vars;
    int size;
} SymTable;
// 函数
typedef struct Function {
    wchar_t* name;            // 函数名
    wchar_t* ret_type;
    Variable* args;
    int argc;
    TokenStream body;
    SymTable symTable;
} Function;
typedef struct ObjectFunction {
    wchar_t* name;            // 函数名
    wchar_t* ret_type;
    Variable* args;
    int argc;
    Sentence* sentences;
    int sentence_size;
} ObjectFunction;
typedef struct ObjectVariable {
    wchar_t* name;
    wchar_t* type;
    void* address;
    bool isOnlyRead;
} ObjectVariable;
typedef struct Data {               //数据段,用于存储全局变量及常量
    ObjectVariable* objectVariable;
    int size;
} Data;
// 目标代码结构
typedef struct ObjectCode {
    FileHeader header;              //文件头
    Data data;                      //数据段
    ObjectFunction* functions;      //函数
    int function_index;             //索引
    int function_size;
} ObjectCode;
struct {
    Function* functions;
    int function_size;
    int function_index;
} checker_symTable;
#endif