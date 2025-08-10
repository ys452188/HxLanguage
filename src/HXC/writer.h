#ifndef HX_WRITER_H
#define HX_WRITER_H
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdbool.h>
#include "compiler.h"
#define hsmCode objCode

// 写入 wchar* 字符串（长度 + 内容）
static void write_wstring(FILE* fp, const wchar* str) {
    if (!str) {
        i32 len = 0;
        fwrite(&len, sizeof(i32), 1, fp);
        return;
    }
    i32 len = (i32)wcslen(str);
    fwrite(&len, sizeof(i32), 1, fp);
    if (len > 0) {
        fwrite(str, sizeof(wchar), len, fp);
    }
}

// 写入函数（可用于全局函数或类方法）
static void write_function(FILE* fp, const ObjFunction* fn) {
    write_wstring(fp, fn->name);
    write_wstring(fp, fn->ret_type);

    // 写参数表
    fwrite(&fn->args_size, sizeof(i32), 1, fp);
    for (i32 a = 0; a < fn->args_size; a++) {
        write_wstring(fp, fn->args[a].name);
        write_wstring(fp, fn->args[a].type);
        fwrite(&fn->args[a].isOnlyRead, sizeof(bool), 1, fp);
    }

    // 写指令体
    fwrite(&fn->body_size, sizeof(i32), 1, fp);
    for (i32 b = 0; b < fn->body_size; b++) {
        fwrite(&fn->body[b].op, sizeof(OPCode), 1, fp);
        fwrite(&fn->body[b].op_value_size, sizeof(i32), 1, fp);
        for (i32 v = 0; v < fn->body[b].op_value_size; v++) {
            fwrite(&fn->body[b].op_value[v].type, sizeof(int), 1, fp);
            if (fn->body[b].op_value[v].type == TYPE_STR) {
                write_wstring(fp, (wchar*)fn->body[b].op_value[v].value.ptr_val);
            }
        }
    }
}

// 写入类成员（公有/私有/保护通用）
static void write_class_member(FILE* fp, const ClassMember* mem, int count) {
    fwrite(&count, sizeof(int), 1, fp);
    for (int m = 0; m < count; m++) {
        write_wstring(fp, mem[m].name);
        write_wstring(fp, mem[m].type);
        fwrite(&mem[m].isOnlyRead, sizeof(bool), 1, fp);
        fwrite(&mem[m].offest, sizeof(int), 1, fp);
    }
}

int writeObjectFile(const char* file_name) {
    FILE* fp = fopen(file_name, "wb");
    if (!fp) return -1;

    // 写 start_fun
    fwrite(&hsmCode.start_fun, sizeof(i32), 1, fp);

    // 写全局符号表
    fwrite(&hsmCode.obj_sym_size, sizeof(i32), 1, fp);
    for (i32 i = 0; i < hsmCode.obj_sym_size; i++) {
        write_wstring(fp, hsmCode.obj_sym[i].name);
        write_wstring(fp, hsmCode.obj_sym[i].type);
        fwrite(&hsmCode.obj_sym[i].isOnlyRead, sizeof(bool), 1, fp);
    }

    // 写全局函数表
    fwrite(&hsmCode.obj_fun_size, sizeof(i32), 1, fp);
    for (i32 i = 0; i < hsmCode.obj_fun_size; i++) {
        write_function(fp, &hsmCode.obj_fun[i]);
    }

    // 写类表
    fwrite(&hsmCode.obj_class_size, sizeof(i32), 1, fp);
    for (i32 i = 0; i < hsmCode.obj_class_size; i++) {
        ObjClass* cls = &hsmCode.obj_class[i];
        write_wstring(fp, cls->name);

        // 写成员
        write_class_member(fp, cls->pub_sym, cls->pub_sym_size);
        write_class_member(fp, cls->pri_sym, cls->pri_sym_size);
        write_class_member(fp, cls->pro_sym, cls->pro_sym_size);

        // 写公有方法
        fwrite(&cls->pub_fun_size, sizeof(int), 1, fp);
        for (int f = 0; f < cls->pub_fun_size; f++) {
            write_function(fp, &cls->pub_fun[f]);
        }

        // 写私有方法
        fwrite(&cls->pri_fun_size, sizeof(int), 1, fp);
        for (int f = 0; f < cls->pri_fun_size; f++) {
            write_function(fp, &cls->pri_fun[f]);
        }

        // 写保护方法
        fwrite(&cls->pro_fun_suze, sizeof(int), 1, fp);
        for (int f = 0; f < cls->pro_fun_suze; f++) {
            write_function(fp, &cls->pro_fun[f]);
        }
    }

    fclose(fp);
    return 0;
}
#endif