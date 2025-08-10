#ifndef HXVM_HX_FILE_LOADER_H
#define HXVM_HX_FILE_LOADER_H
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <limits.h>
#include <stdbool.h>
#include "HXVM.h"

typedef uint32_t i32;

typedef enum OPCode {   //操作码
    OP_PUT_STR,         //输出wchar_t*
} OPCode;
typedef struct ObjValue {
    union {
        void* ptr_val;
    } value;
    enum {
        TYPE_STR,
    } type;
} ObjValue;
typedef struct Command {
    OPCode op;
    ObjValue* op_value;
    i32 op_value_size;
} Command;
typedef struct ClassMember {
    wchar* name;
    wchar* type;
    bool isOnlyRead;
    int offest;   //偏移量
} ClassMember;

typedef struct ObjSymbol {
    wchar* name;
    wchar* type;
    bool isOnlyRead;
} ObjSymbol;
typedef struct ObjFunction {
    wchar* name;
    wchar* ret_type;
    ObjSymbol* args;
    i32 args_size;
    Command* body;
    i32 body_size;
} ObjFunction;
typedef struct ObjClass {
    wchar* name;
    ClassMember* pub_sym;
    int pub_sym_size;
    ClassMember* pri_sym;
    int pri_sym_size;
    ClassMember* pro_sym;
    int pro_sym_size;

    ObjFunction* pub_fun;
    int pub_fun_size;
    ObjFunction* pri_fun;
    int pri_fun_size;
    ObjFunction* pro_fun;
    int pro_fun_suze;
} ObjClass;
typedef struct ObjectCode {
    i32 start_fun;    //入口点,main函数的索引
    ObjFunction* obj_fun;
    i32 obj_fun_size;

    ObjSymbol* obj_sym;
    i32 obj_sym_size;

    ObjClass* obj_class;
    i32 obj_class_size;
} ObjectCode;

ObjectCode hsmCode = {0};

// 读取 wchar* 字符串（先读长度，再读内容，自动加 \0）
static wchar* read_wstring(FILE* fp) {
    i32 len;
    if (fread(&len, sizeof(i32), 1, fp) != 1) return NULL;
    if (len <= 0) return NULL;

    wchar* str = (wchar*)malloc((len + 1) * sizeof(wchar));
    if (!str) return NULL;
    if (fread(str, sizeof(wchar), len, fp) != (size_t)len) {
        free(str);
        return NULL;
    }
    str[len] = L'\0';
    return str;
}

// 读取一个函数（全局或类方法）
static void read_function(FILE* fp, ObjFunction* fn) {
    fn->name = read_wstring(fp);
    fn->ret_type = read_wstring(fp);

    // 读取参数
    fread(&fn->args_size, sizeof(i32), 1, fp);
    if (fn->args_size > 0) {
        fn->args = (ObjSymbol*)calloc(fn->args_size, sizeof(ObjSymbol));
        for (i32 a = 0; a < fn->args_size; a++) {
            fn->args[a].name = read_wstring(fp);
            fn->args[a].type = read_wstring(fp);
            fread(&fn->args[a].isOnlyRead, sizeof(bool), 1, fp);
        }
    } else {
        fn->args = NULL;
    }

    // 读取指令体
    fread(&fn->body_size, sizeof(i32), 1, fp);
    if (fn->body_size > 0) {
        fn->body = (Command*)calloc(fn->body_size, sizeof(Command));
        for (i32 b = 0; b < fn->body_size; b++) {
            fread(&fn->body[b].op, sizeof(OPCode), 1, fp);
            fread(&fn->body[b].op_value_size, sizeof(i32), 1, fp);
            if (fn->body[b].op_value_size > 0) {
                fn->body[b].op_value = (ObjValue*)calloc(fn->body[b].op_value_size, sizeof(ObjValue));
                for (i32 v = 0; v < fn->body[b].op_value_size; v++) {
                    fread(&fn->body[b].op_value[v].type, sizeof(int), 1, fp);
                    if (fn->body[b].op_value[v].type == TYPE_STR) {
                        fn->body[b].op_value[v].value.ptr_val = read_wstring(fp);
                    }
                }
            } else {
                fn->body[b].op_value = NULL;
            }
        }
    } else {
        fn->body = NULL;
    }
}

// 读取类成员数组
static ClassMember* read_class_member(FILE* fp, int* count) {
    fread(count, sizeof(int), 1, fp);
    if (*count <= 0) return NULL;

    ClassMember* arr = (ClassMember*)calloc(*count, sizeof(ClassMember));
    for (int m = 0; m < *count; m++) {
        arr[m].name = read_wstring(fp);
        arr[m].type = read_wstring(fp);
        fread(&arr[m].isOnlyRead, sizeof(bool), 1, fp);
        fread(&arr[m].offest, sizeof(int), 1, fp);
    }
    return arr;
}

int loadObjectFile(const char* file_name) {
    FILE* fp = fopen(file_name, "rb");
    if (!fp) return -1;

    memset(&hsmCode, 0, sizeof(ObjectCode));

    // 读取 start_fun
    fread(&hsmCode.start_fun, sizeof(i32), 1, fp);

    // 读取全局符号
    fread(&hsmCode.obj_sym_size, sizeof(i32), 1, fp);
    if (hsmCode.obj_sym_size > 0) {
        hsmCode.obj_sym = (ObjSymbol*)calloc(hsmCode.obj_sym_size, sizeof(ObjSymbol));
        for (i32 i = 0; i < hsmCode.obj_sym_size; i++) {
            hsmCode.obj_sym[i].name = read_wstring(fp);
            hsmCode.obj_sym[i].type = read_wstring(fp);
            fread(&hsmCode.obj_sym[i].isOnlyRead, sizeof(bool), 1, fp);
        }
    }

    // 读取全局函数
    fread(&hsmCode.obj_fun_size, sizeof(i32), 1, fp);
    if (hsmCode.obj_fun_size > 0) {
        hsmCode.obj_fun = (ObjFunction*)calloc(hsmCode.obj_fun_size, sizeof(ObjFunction));
        for (i32 i = 0; i < hsmCode.obj_fun_size; i++) {
            read_function(fp, &hsmCode.obj_fun[i]);
        }
    }

    // 读取类表
    fread(&hsmCode.obj_class_size, sizeof(i32), 1, fp);
    if (hsmCode.obj_class_size > 0) {
        hsmCode.obj_class = (ObjClass*)calloc(hsmCode.obj_class_size, sizeof(ObjClass));
        for (i32 i = 0; i < hsmCode.obj_class_size; i++) {
            ObjClass* cls = &hsmCode.obj_class[i];
            cls->name = read_wstring(fp);

            // 读取成员
            cls->pub_sym = read_class_member(fp, &cls->pub_sym_size);
            cls->pri_sym = read_class_member(fp, &cls->pri_sym_size);
            cls->pro_sym = read_class_member(fp, &cls->pro_sym_size);

            // 读取公有方法
            fread(&cls->pub_fun_size, sizeof(int), 1, fp);
            if (cls->pub_fun_size > 0) {
                cls->pub_fun = (ObjFunction*)calloc(cls->pub_fun_size, sizeof(ObjFunction));
                for (int f = 0; f < cls->pub_fun_size; f++) {
                    read_function(fp, &cls->pub_fun[f]);
                }
            }

            // 读取私有方法
            fread(&cls->pri_fun_size, sizeof(int), 1, fp);
            if (cls->pri_fun_size > 0) {
                cls->pri_fun = (ObjFunction*)calloc(cls->pri_fun_size, sizeof(ObjFunction));
                for (int f = 0; f < cls->pri_fun_size; f++) {
                    read_function(fp, &cls->pri_fun[f]);
                }
            }

            // 读取保护方法
            fread(&cls->pro_fun_suze, sizeof(int), 1, fp);
            if (cls->pro_fun_suze > 0) {
                cls->pro_fun = (ObjFunction*)calloc(cls->pro_fun_suze, sizeof(ObjFunction));
                for (int f = 0; f < cls->pro_fun_suze; f++) {
                    read_function(fp, &cls->pro_fun[f]);
                }
            }
        }
    }

    fclose(fp);
    return 0;
}
// 释放 wchar* 字符串
static void free_wstring(wchar* s) {
    if (s) free(s);
}

// 释放符号表
static void free_symbol(ObjSymbol* sym, int count) {
    if (!sym) return;
    for (int i = 0; i < count; i++) {
        free_wstring(sym[i].name);
        free_wstring(sym[i].type);
    }
    free(sym);
}

// 释放函数
static void free_function(ObjFunction* fn, int count) {
    if (!fn) return;
    for (int i = 0; i < count; i++) {
        free_wstring(fn[i].name);
        free_wstring(fn[i].ret_type);
        free_symbol(fn[i].args, fn[i].args_size);

        if (fn[i].body) {
            for (int b = 0; b < fn[i].body_size; b++) {
                if (fn[i].body[b].op_value) {
                    for (int v = 0; v < fn[i].body[b].op_value_size; v++) {
                        if (fn[i].body[b].op_value[v].type == TYPE_STR) {
                            free_wstring((wchar*)fn[i].body[b].op_value[v].value.ptr_val);
                        }
                    }
                    free(fn[i].body[b].op_value);
                }
            }
            free(fn[i].body);
        }
    }
    free(fn);
}

// 释放类成员表
static void free_class_member(ClassMember* mem, int count) {
    if (!mem) return;
    for (int i = 0; i < count; i++) {
        free_wstring(mem[i].name);
        free_wstring(mem[i].type);
    }
    free(mem);
}

// 释放类表
static void free_class(ObjClass* cls, int count) {
    if (!cls) return;
    for (int i = 0; i < count; i++) {
        free_wstring(cls[i].name);

        free_class_member(cls[i].pub_sym, cls[i].pub_sym_size);
        free_class_member(cls[i].pri_sym, cls[i].pri_sym_size);
        free_class_member(cls[i].pro_sym, cls[i].pro_sym_size);

        free_function(cls[i].pub_fun, cls[i].pub_fun_size);
        free_function(cls[i].pri_fun, cls[i].pri_fun_size);
        free_function(cls[i].pro_fun, cls[i].pro_fun_suze);
    }
    free(cls);
}

// 释放整个 ObjectCode
void freeObjectCode(ObjectCode* code) {
    if (!code) return;

    free_symbol(code->obj_sym, code->obj_sym_size);
    free_function(code->obj_fun, code->obj_fun_size);
    free_class(code->obj_class, code->obj_class_size);

    memset(code, 0, sizeof(ObjectCode));
}

#endif