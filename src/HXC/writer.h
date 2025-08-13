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

/* 写入辅助函数：写 int32，写 wchar*（len + 内容），写 bytes */
static int write_int32(FILE* f, int32_t v) {
    if (fwrite(&v, sizeof(int32_t), 1, f) != 1) return -1;
    return 0;
}
static int write_uint32(FILE* f, uint32_t v) {
    if (fwrite(&v, sizeof(uint32_t), 1, f) != 1) return -1;
    return 0;
}

/* 写 wchar*：先写 i32 len（字符数）；如果 str==NULL 或 len==0 写 0 (读取端视 len<=0 为 NULL) */
static int write_wstring(FILE* f, const wchar_t* str) {
    if (str == NULL) {
        if (write_int32(f, 0) != 0) return -1;
        return 0;
    }
    long len = (long)wcslen(str);
    if (len > INT32_MAX) return -1;
    if (write_int32(f, (int32_t)len) != 0) return -1;
    if (len > 0) {
        size_t wrote = fwrite(str, sizeof(wchar_t), (size_t)len, f);
        if (wrote != (size_t)len) return -1;
    }
    return 0;
}

/* 写入 ObjSymbol */
static int write_symbol(FILE* f, const ObjSymbol* s) {
    if (!s) {
        /* 写一个 marker：len=0 表示 NULL 符号（通常不应发生） */
        if (write_int32(f, 0) != 0) return -1;
        return 0;
    }
    if (write_wstring(f, s->name) != 0) return -1;
    if (write_wstring(f, s->type) != 0) return -1;
    if (fwrite(&s->isOnlyRead, sizeof(bool), 1, f) != 1) return -1;
    return 0;
}

/* 写入 Command 与其 op_value（允许 cmd->op_value 为 NULL） */
static int write_command(FILE* f, const Command* cmd) {
    if (!cmd) {
        /* 写默认值：op=0, op_value_size=0 */
        if (write_int32(f, 0) != 0) return -1; /* op 以 int32 写 */
        if (write_int32(f, 0) != 0) return -1; /* op_value_size */
        return 0;
    }

    /* 写入 op （以 int32 写，保证跨平台一致读取） */
    if (write_int32(f, (int32_t)cmd->op) != 0) return -1;

    /* 写入 op_value_size */
    if (cmd->op_value_size < 0) return -1; /* 非法值保护 */
    if (write_int32(f, (int32_t)cmd->op_value_size) != 0) return -1;

    /* 如果 op_value 指针为 NULL，但 op_value_size > 0，则为每个位置写占位（type=-1, raw_size=0） */
    if (cmd->op_value == NULL) {
        for (int i = 0; i < cmd->op_value_size; ++i) {
            /* 写入一个标识空的 type（-1），读取端应将其解释为 value=NULL */
            if (write_int32(f, (int32_t)-1) != 0) return -1;
            /* 对于非字符串类型，写入 raw_size（0 表示没有数据）以便读取端正确跳过 */
            if (write_int32(f, (int32_t)0) != 0) return -1;
        }
        return 0;
    }

    /* 正常情况：有 op_value 数组，逐项写入 */
    for (int i = 0; i < cmd->op_value_size; ++i) {
        const ObjValue* v = &cmd->op_value[i];
        /* 写入 type（int32） */
        if (write_int32(f, (int32_t)v->type) != 0) return -1;

        /* 根据类型写入实际内容；当前有 TYPE_STR 和 TYPE_SYM（两者均为 wchar* ） */
        if (v->type == TYPE_STR || v->type == TYPE_SYM) {
            /* v->value 可能为 NULL，write_wstring 会写 len=0 (读取端视 len<=0 为 NULL) */
            wchar_t* ws = (wchar_t*)(v->value);
            if (write_wstring(f, ws) != 0) return -1;
        } else {
            /* 非字符串类型：写入 raw_size（若 value==NULL 则写 0） */
            if (v->value == NULL) {
                if (write_int32(f, 0) != 0) return -1;
            } else {
                if (v->size < 0) return -1;
                if (write_int32(f, (int32_t)v->size) != 0) return -1;
                if (fwrite(v->value, 1, (size_t)v->size, f) != (size_t)v->size) return -1;
            }
        }
    }

    return 0;
}

/* 写入 ObjFunction（name, ret_type, args, body） */
static int write_function(FILE* f, const ObjFunction* fn) {
    if (!fn) {
        /* 写空函数占位：len=0 name -> 0, ret_type -> 0, args_size=0, body_size=0 */
        if (write_wstring(f, NULL) != 0) return -1;
        if (write_wstring(f, NULL) != 0) return -1;
        if (write_int32(f, 0) != 0) return -1;
        if (write_int32(f, 0) != 0) return -1;
        return 0;
    }
    if (write_wstring(f, fn->name) != 0) return -1;
    if (write_wstring(f, fn->ret_type) != 0) return -1;

    /* args */
    if (write_int32(f, (int32_t)fn->args_size) != 0) return -1;
    for (int i = 0; i < fn->args_size; ++i) {
        if (write_symbol(f, &fn->args[i]) != 0) return -1;
    }

    /* body */
    if (write_int32(f, (int32_t)fn->body_size) != 0) return -1;
    for (int i = 0; i < fn->body_size; ++i) {
        if (write_command(f, &fn->body[i]) != 0) return -1;
    }
    return 0;
}

/* 写入 ClassMember 数组（count 已由调用者传入） */
static int write_class_members(FILE* f, const ClassMember* mem, int count) {
    if (write_int32(f, (int32_t)count) != 0) return -1;
    for (int i = 0; i < count; ++i) {
        if (write_wstring(f, mem[i].name) != 0) return -1;
        if (write_wstring(f, mem[i].type) != 0) return -1;
        if (fwrite(&mem[i].isOnlyRead, sizeof(bool), 1, f) != 1) return -1;
        if (fwrite(&mem[i].offest, sizeof(int), 1, f) != 1) return -1;
    }
    return 0;
}

/* 写入 ObjClass */
static int write_class(FILE* f, const ObjClass* cls) {
    if (!cls) {
        /* 写 0 个字段的占位 */
        if (write_wstring(f, NULL) != 0) return -1;
        if (write_int32(f, 0) != 0) return -1; /* pub_sym_size */
        if (write_int32(f, 0) != 0) return -1; /* pri_sym_size */
        if (write_int32(f, 0) != 0) return -1; /* pro_sym_size */
        if (write_int32(f, 0) != 0) return -1; /* pub_fun_size */
        if (write_int32(f, 0) != 0) return -1; /* pri_fun_size */
        if (write_int32(f, 0) != 0) return -1; /* pro_fun_suze */
        return 0;
    }

    if (write_wstring(f, cls->name) != 0) return -1;

    /* 成员表：公/私/保护 */
    if (write_class_members(f, cls->pub_sym, cls->pub_sym_size) != 0) return -1;
    if (write_class_members(f, cls->pri_sym, cls->pri_sym_size) != 0) return -1;
    if (write_class_members(f, cls->pro_sym, cls->pro_sym_size) != 0) return -1;

    /* 函数表：公有/私有/保护，用 write_function 重复 */
    if (write_int32(f, (int32_t)cls->pub_fun_size) != 0) return -1;
    for (int i = 0; i < cls->pub_fun_size; ++i) {
        if (write_function(f, &cls->pub_fun[i]) != 0) return -1;
    }
    if (write_int32(f, (int32_t)cls->pri_fun_size) != 0) return -1;
    for (int i = 0; i < cls->pri_fun_size; ++i) {
        if (write_function(f, &cls->pri_fun[i]) != 0) return -1;
    }
    if (write_int32(f, (int32_t)cls->pro_fun_suze) != 0) return -1;
    for (int i = 0; i < cls->pro_fun_suze; ++i) {
        if (write_function(f, &cls->pro_fun[i]) != 0) return -1;
    }

    return 0;
}

/* ---------- 主函数：写入 objCode 到文件 ---------- */
int writeObjectFile(char* file_name) {
    if (file_name == NULL) return -1;
    FILE* f = fopen(file_name, "wb");
    if (!f) return -1;

    /* 推荐在头部留出 magic/version/sizeof(wchar_t) 的位置以便扩展 --- 可选
       （这里为简洁，仅写数据本身；如需兼容可自行在前面加写） */

    /* 写入口 */
    if (write_int32(f, (int32_t)objCode.start_fun) != 0) {
        fclose(f);
        return -1;
    }

    /* 写全局函数表 */
    if (write_int32(f, (int32_t)objCode.obj_fun_size) != 0) {
        fclose(f);
        return -1;
    }
    for (int i = 0; i < objCode.obj_fun_size; ++i) {
        if (write_function(f, &objCode.obj_fun[i]) != 0) {
            fclose(f);
            return -1;
        }
    }

    /* 写全局符号表 */
    if (write_int32(f, (int32_t)objCode.obj_sym_size) != 0) {
        fclose(f);
        return -1;
    }
    for (int i = 0; i < objCode.obj_sym_size; ++i) {
        if (write_symbol(f, &objCode.obj_sym[i]) != 0) {
            fclose(f);
            return -1;
        }
    }

    /* 写类表 */
    if (write_int32(f, (int32_t)objCode.obj_class_size) != 0) {
        fclose(f);
        return -1;
    }
    for (int i = 0; i < objCode.obj_class_size; ++i) {
        if (write_class(f, &objCode.obj_class[i]) != 0) {
            fclose(f);
            return -1;
        }
    }

    /* flush & close */
    if (fflush(f) != 0) {
        fclose(f);
        return -1;
    }
    if (fclose(f) != 0) return -1;
    return 0;
}

#endif