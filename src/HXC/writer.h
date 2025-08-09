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

/* 辅助写入函数：整型、wchar 字符串、char 字符串、uint8_t */
static int write_int32(FILE* f, int32_t v) {
    if (fwrite(&v, sizeof(int32_t), 1, f) != 1) return -1;
    return 0;
}
static int write_uint32(FILE* f, uint32_t v) {
    if (fwrite(&v, sizeof(uint32_t), 1, f) != 1) return -1;
    return 0;
}
static int write_uint8(FILE* f, uint8_t v) {
    if (fwrite(&v, sizeof(uint8_t), 1, f) != 1) return -1;
    return 0;
}

/* 写入 wchar_t*：格式 int32 length (字符数, 不含终止符, -1 表示 NULL)，随后写 length * sizeof(wchar_t) 字节数据 */
static int write_wcs(FILE* f, const wchar_t* s) {
    if (s == NULL) {
        if (write_int32(f, -1) != 0) return -1;
        return 0;
    }
    size_t len = wcslen(s);
    if (len > (size_t)INT32_MAX) return -1;
    if (write_int32(f, (int32_t)len) != 0) return -1;
    if (len > 0) {
        size_t wrote = fwrite(s, sizeof(wchar_t), len, f);
        if (wrote != len) return -1;
    }
    return 0;
}

/* 写入 char*：格式 int32 length (字节数，不含终止符, -1 表示 NULL)，随后写 length 字节数据 */
static int write_cs(FILE* f, const char* s) {
    if (s == NULL) {
        if (write_int32(f, -1) != 0) return -1;
        return 0;
    }
    size_t len = strlen(s);
    if (len > (size_t)INT32_MAX) return -1;
    if (write_int32(f, (int32_t)len) != 0) return -1;
    if (len > 0) {
        size_t wrote = fwrite(s, 1, len, f);
        if (wrote != len) return -1;
    }
    return 0;
}

/* 主函数：把全局 objCode 写入二进制文件 */
int writeObjectFile(char* file_name) {
    if (file_name == NULL) return -1;
    FILE* f = fopen(file_name, "wb");
    if (!f) return -1;

    /* 写入简单头部：magic + version */
    const char magic[5] = { 'H','X','O','B','J' }; /* 5 字节 */
    if (fwrite(magic, 1, 5, f) != 5) {
        fclose(f);
        return -1;
    }
    uint32_t version = 1;
    if (write_uint32(f, version) != 0) {
        fclose(f);
        return -1;
    }
    /* 写入入口函数索引 */
    if (write_uint32(f, (uint32_t)objCode.start_fun) != 0) {
        fclose(f);
        return -1;
    }
    /* 写入函数表数量 */
    if (write_uint32(f, (uint32_t)objCode.obj_fun_size) != 0) {
        fclose(f);
        return -1;
    }
    /* 每个函数的序列化 */
    for (uint32_t i = 0; i < (uint32_t)objCode.obj_fun_size; ++i) {
        ObjFunction* fun = &objCode.obj_fun[i];
        /* 名字与返回类型都为 wchar_t* */
        if (write_wcs(f, fun->name) != 0) {
            fclose(f);
            return -1;
        }
        if (write_wcs(f, fun->ret_type) != 0) {
            fclose(f);
            return -1;
        }
        /* 参数表 */
        if (write_uint32(f, (uint32_t)fun->args_size) != 0) {
            fclose(f);
            return -1;
        }
        for (uint32_t j = 0; j < (uint32_t)fun->args_size; ++j) {
            ObjSymbol* sym = &fun->args[j];
            if (write_wcs(f, sym->name) != 0) {
                fclose(f);
                return -1;
            }
            if (write_wcs(f, sym->type) != 0) {
                fclose(f);
                return -1;
            }
            uint8_t readonly = sym->isOnlyRead ? 1 : 0;
            if (write_uint8(f, readonly) != 0) {
                fclose(f);
                return -1;
            }
        }
        /* 指令 */
        if (write_uint32(f, (uint32_t)fun->body_size) != 0) {
            fclose(f);
            return -1;
        }
        for (uint32_t k = 0; k < (uint32_t)fun->body_size; ++k) {
            Command* cmd = &fun->body[k];
            /* 写入操作码（以 32 位写入） */
            if (write_uint32(f, (uint32_t)cmd->op) != 0) {
                fclose(f);
                return -1;
            }
            /* 写入 op_value_size */
            if (write_uint32(f, (uint32_t)cmd->op_value_size) != 0) {
                fclose(f);
                return -1;
            }
            /* 每个 op_value 序列化 */
            for (uint32_t v = 0; v < (uint32_t)cmd->op_value_size; ++v) {
                ObjValue* ov = &cmd->op_value[v];
                /* 写入类型（int32） */
                if (write_int32(f, (int32_t)ov->type) != 0) {
                    fclose(f);
                    return -1;
                }
                /* 写入值本体（根据类型） */
                if (ov->type == TYPE_WCS) {
                    wchar_t* w = (wchar_t*)ov->value.ptr_val;
                    if (write_wcs(f, w) != 0) {
                        fclose(f);
                        return -1;
                    }
                } else if (ov->type == TYPE_CS) {
                    char* s = (char*)ov->value.ptr_val;
                    if (write_cs(f, s) != 0) {
                        fclose(f);
                        return -1;
                    }
                } else {
                    /* 未知类型，失败 */
                    fclose(f);
                    return -1;
                }
            }
        }
    }
    /* 写入全局符号表 */
    if (write_uint32(f, (uint32_t)objCode.obj_sym_size) != 0) {
        fclose(f);
        return -1;
    }
    for (uint32_t i = 0; i < (uint32_t)objCode.obj_sym_size; ++i) {
        ObjSymbol* sym = &objCode.obj_sym[i];
        if (write_wcs(f, sym->name) != 0) {
            fclose(f);
            return -1;
        }
        if (write_wcs(f, sym->type) != 0) {
            fclose(f);
            return -1;
        }
        uint8_t readonly = sym->isOnlyRead ? 1 : 0;
        if (write_uint8(f, readonly) != 0) {
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