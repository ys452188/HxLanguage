#ifndef HXVM_HX_FILE_LOADER_H
#define HXVM_HX_FILE_LOADER_H
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <limits.h>
#include <stdbool.h>

/* 与用户头部保持一致的类型别名 */
typedef uint32_t i32;
typedef enum OPCode {
    OP_PUT_WCS,
    OP_PUT_CS,
} OPCode;
typedef struct ObjValue {
    union {
        void* ptr_val;
    } value;
    enum {
        TYPE_WCS,
        TYPE_CS,
    } type;
} ObjValue;
typedef struct Command {
    OPCode op;
    ObjValue* op_value;
    i32 op_value_size;
} Command;
typedef struct ObjSymbol {
    wchar_t* name;
    wchar_t* type;
    bool isOnlyRead;
} ObjSymbol;
typedef struct ObjFunction {
    wchar_t* name;
    wchar_t* ret_type;
    ObjSymbol* args;
    i32 args_size;
    Command* body;
    i32 body_size;
} ObjFunction;
typedef struct ObjectCode {
    i32 start_fun;
    ObjFunction* obj_fun;
    i32 obj_fun_size;
    ObjSymbol* obj_sym;
    i32 obj_sym_size;
} ObjectCode;

ObjectCode hsmCode = {0};

/* 读取固定尺寸的数据；返回 0 成功，-1 失败 */
static int read_exact(FILE* f, void* buf, size_t size) {
    if (size == 0) return 0;
    size_t r = fread(buf, 1, size, f);
    if (r != size) return -1;
    return 0;
}

/* 读取 int32_t、uint32_t、uint8_t */
static int read_int32(FILE* f, int32_t* out) {
    return read_exact(f, out, sizeof(int32_t));
}
static int read_uint32(FILE* f, uint32_t* out) {
    return read_exact(f, out, sizeof(uint32_t));
}
static int read_uint8(FILE* f, uint8_t* out) {
    return read_exact(f, out, sizeof(uint8_t));
}

/* 读取 wchar_t*：先读 int32 length（-1 表示 NULL），否则分配 length+1，
   读取 length 个 wchar_t 并在末尾加入 L'\0'。 */
static int read_wcs_alloc(FILE* f, wchar_t** out) {
    int32_t len;
    if (read_int32(f, &len) != 0) return -1;
    if (len == -1) {
        *out = NULL;
        return 0;
    }
    if (len < 0) return -1;
    /* 检查是否合法大小 */
    if ((uint32_t)len > (UINT_MAX / sizeof(wchar_t))) return -1;
    wchar_t* buf = (wchar_t*)calloc((size_t)len + 1, sizeof(wchar_t));
    if (!buf) return -1;
    if (len > 0) {
        if (read_exact(f, buf, (size_t)len * sizeof(wchar_t)) != 0) {
            free(buf);
            return -1;
        }
    }
    buf[len] = L'\0';
    *out = buf;
    return 0;
}

/* 读取 char*：先读 int32 length（-1 表示 NULL），否则分配 length+1，
   读取 length 字节并在末尾加入 '\0'。 */
static int read_cs_alloc(FILE* f, char** out) {
    int32_t len;
    if (read_int32(f, &len) != 0) return -1;
    if (len == -1) {
        *out = NULL;
        return 0;
    }
    if (len < 0) return -1;
    if ((uint32_t)len > UINT_MAX) return -1;
    char* buf = (char*)calloc((size_t)len + 1, 1);
    if (!buf) return -1;
    if (len > 0) {
        if (read_exact(f, buf, (size_t)len) != 0) {
            free(buf);
            return -1;
        }
    }
    buf[len] = '\0';
    *out = buf;
    return 0;
}

/* 通用释放函数：释放 ObjectCode 中各字段分配的动态内存 */
static void freeObjectCode(ObjectCode* c) {
    if (!c) return;

    if (c->obj_sym) {
        for (int i = 0; i < c->obj_sym_size; ++i) {
            if (c->obj_sym[i].name) {
                free(c->obj_sym[i].name);
                c->obj_sym[i].name = NULL;
            }
            if (c->obj_sym[i].type) {
                free(c->obj_sym[i].type);
                c->obj_sym[i].type = NULL;
            }
        }
        free(c->obj_sym);
        c->obj_sym = NULL;
        c->obj_sym_size = 0;
    }

    if (c->obj_fun) {
        for (int i = 0; i < c->obj_fun_size; ++i) {
            ObjFunction* f = &c->obj_fun[i];
            printf("freeing fun...\n");
            wprintf(L"[DBG] loaded fun[%d] name='%ls' ret='%ls' args=%d body=%d\n", i, f->name?f->name:L"(null)", f->ret_type?f->ret_type:L"(null)", f->args_size, f->body_size);
            if (f->name) {
                free(f->name);
                f->name = NULL;
            }
            if (f->ret_type) {
                free(f->ret_type);
                f->ret_type = NULL;
            }

            if (f->args) {
                for (int j = 0; j < f->args_size; ++j) {
                    if (f->args[j].name) {
                        free(f->args[j].name);
                        f->args[j].name = NULL;
                    }
                    if (f->args[j].type) {
                        free(f->args[j].type);
                        f->args[j].type = NULL;
                    }
                }
                free(f->args);
                f->args = NULL;
                f->args_size = 0;
            }

            if (f->body) {
                for (int k = 0; k < f->body_size; ++k) {
                    Command* cmd = &f->body[k];
                    if (cmd->op_value) {
                        for (int v = 0; v < cmd->op_value_size; ++v) {
                            ObjValue* ov = &cmd->op_value[v];
                            if (ov->type == TYPE_WCS) {
                                if (ov->value.ptr_val) {
                                    free(ov->value.ptr_val);
                                    ov->value.ptr_val = NULL;
                                }
                            } else if (ov->type == TYPE_CS) {
                                if (ov->value.ptr_val) {
                                    free(ov->value.ptr_val);
                                    ov->value.ptr_val = NULL;
                                }
                            } else {
                                /* 如果将来有新类型，这里也应该释放对应资源 */
                            }
                        }
                        free(cmd->op_value);
                        cmd->op_value = NULL;
                        cmd->op_value_size = 0;
                    }
                }
                free(f->body);
                f->body = NULL;
                f->body_size = 0;
            }
        }
        free(c->obj_fun);
        c->obj_fun = NULL;
        c->obj_fun_size = 0;
    }

    /* 清除入口点 */
    c->start_fun = 0;
}

/* ---------- 主加载函数 ---------- */

int loadObjectFile(const char* file_name) {
    if (file_name == NULL) return -1;
    FILE* f = fopen(file_name, "rb");
    if (!f) return -1;

    /* 验证 magic */
    char magic[5];
    if (read_exact(f, magic, 5) != 0) {
        fclose(f);
        return -1;
    }
    const char expected_magic[5] = { 'H','X','O','B','J' };
    if (memcmp(magic, expected_magic, 5) != 0) {
        fclose(f);
        return -1;
    }

    /* 版本 */
    uint32_t version;
    if (read_uint32(f, &version) != 0) {
        fclose(f);
        return -1;
    }
    if (version != 1) {
        /* 目前只支持 version 1 */ fclose(f);
        return -1;
    }

    /* 读入并构建一个临时 ObjectCode，出错时方便统一释放 */
    ObjectCode tmp = {0};
    tmp.start_fun = 0;
    tmp.obj_fun = NULL;
    tmp.obj_fun_size = 0;
    tmp.obj_sym = NULL;
    tmp.obj_sym_size = 0;

    /* start_fun */
    uint32_t start_fun_u;
    if (read_uint32(f, &start_fun_u) != 0) {
        freeObjectCode(&tmp);
        fclose(f);
        return -1;
    }
    tmp.start_fun = (i32)start_fun_u;

    /* obj_fun_size */
    uint32_t fun_count;
    if (read_uint32(f, &fun_count) != 0) {
        freeObjectCode(&tmp);
        fclose(f);
        return -1;
    }
    if (fun_count > INT_MAX) {
        freeObjectCode(&tmp);
        fclose(f);
        return -1;
    }

    if (fun_count > 0) {
        tmp.obj_fun = (ObjFunction*)calloc((size_t)fun_count, sizeof(ObjFunction));
        if (!tmp.obj_fun) {
            freeObjectCode(&tmp);
            fclose(f);
            return -1;
        }
        tmp.obj_fun_size = (i32)fun_count;

        for (uint32_t i = 0; i < fun_count; ++i) {
            ObjFunction* fun = &tmp.obj_fun[i];
            fun->name = NULL;
            fun->ret_type = NULL;
            fun->args = NULL;
            fun->args_size = 0;
            fun->body = NULL;
            fun->body_size = 0;

            /* name */
            if (read_wcs_alloc(f, &fun->name) != 0) {
                freeObjectCode(&tmp);
                fclose(f);
                return -1;
            }
            /* ret_type */
            if (read_wcs_alloc(f, &fun->ret_type) != 0) {
                freeObjectCode(&tmp);
                fclose(f);
                return -1;
            }

            /* args_size */
            uint32_t args_cnt;
            if (read_uint32(f, &args_cnt) != 0) {
                freeObjectCode(&tmp);
                fclose(f);
                return -1;
            }
            if (args_cnt > INT_MAX) {
                freeObjectCode(&tmp);
                fclose(f);
                return -1;
            }
            if (args_cnt > 0) {
                fun->args = (ObjSymbol*)calloc((size_t)args_cnt, sizeof(ObjSymbol));
                if (!fun->args) {
                    freeObjectCode(&tmp);
                    fclose(f);
                    return -1;
                }
                fun->args_size = (i32)args_cnt;
                for (uint32_t j = 0; j < args_cnt; ++j) {
                    fun->args[j].name = NULL;
                    fun->args[j].type = NULL;
                    fun->args[j].isOnlyRead = false;

                    if (read_wcs_alloc(f, &fun->args[j].name) != 0) {
                        freeObjectCode(&tmp);
                        fclose(f);
                        return -1;
                    }
                    if (read_wcs_alloc(f, &fun->args[j].type) != 0) {
                        freeObjectCode(&tmp);
                        fclose(f);
                        return -1;
                    }
                    uint8_t ro;
                    if (read_uint8(f, &ro) != 0) {
                        freeObjectCode(&tmp);
                        fclose(f);
                        return -1;
                    }
                    fun->args[j].isOnlyRead = (ro != 0);
                }
            }

            /* body_size */
            uint32_t body_cnt;
            if (read_uint32(f, &body_cnt) != 0) {
                freeObjectCode(&tmp);
                fclose(f);
                return -1;
            }
            if (body_cnt > INT_MAX) {
                freeObjectCode(&tmp);
                fclose(f);
                return -1;
            }
            if (body_cnt > 0) {
                fun->body = (Command*)calloc((size_t)body_cnt, sizeof(Command));
                if (!fun->body) {
                    freeObjectCode(&tmp);
                    fclose(f);
                    return -1;
                }
                fun->body_size = (i32)body_cnt;
                for (uint32_t k = 0; k < body_cnt; ++k) {
                    Command* cmd = &fun->body[k];
                    cmd->op_value = NULL;
                    cmd->op_value_size = 0;

                    uint32_t op_u;
                    if (read_uint32(f, &op_u) != 0) {
                        freeObjectCode(&tmp);
                        fclose(f);
                        return -1;
                    }
                    cmd->op = (OPCode)op_u;

                    uint32_t op_val_cnt;
                    if (read_uint32(f, &op_val_cnt) != 0) {
                        freeObjectCode(&tmp);
                        fclose(f);
                        return -1;
                    }
                    if (op_val_cnt > INT_MAX) {
                        freeObjectCode(&tmp);
                        fclose(f);
                        return -1;
                    }
                    cmd->op_value_size = (i32)op_val_cnt;

                    if (op_val_cnt > 0) {
                        cmd->op_value = (ObjValue*)calloc((size_t)op_val_cnt, sizeof(ObjValue));
                        if (!cmd->op_value) {
                            freeObjectCode(&tmp);
                            fclose(f);
                            return -1;
                        }
                        for (uint32_t v = 0; v < op_val_cnt; ++v) {
                            ObjValue* ov = &cmd->op_value[v];
                            ov->value.ptr_val = NULL;
                            int32_t type_i;
                            if (read_int32(f, &type_i) != 0) {
                                freeObjectCode(&tmp);
                                fclose(f);
                                return -1;
                            }
                            ov->type = (int)type_i;
                            if (ov->type == TYPE_WCS) {
                                wchar_t* w;
                                if (read_wcs_alloc(f, &w) != 0) {
                                    freeObjectCode(&tmp);
                                    fclose(f);
                                    return -1;
                                }
                                ov->value.ptr_val = w;
                            } else if (ov->type == TYPE_CS) {
                                char* s;
                                if (read_cs_alloc(f, &s) != 0) {
                                    freeObjectCode(&tmp);
                                    fclose(f);
                                    return -1;
                                }
                                ov->value.ptr_val = s;
                            } else {
                                /* 遇到未知类型，失败 */
                                freeObjectCode(&tmp);
                                fclose(f);
                                return -1;
                            }
                        }
                    }
                }
            }
        }
    }

    /* 全局符号表 */
    uint32_t sym_count;
    if (read_uint32(f, &sym_count) != 0) {
        freeObjectCode(&tmp);
        fclose(f);
        return -1;
    }
    if (sym_count > INT_MAX) {
        freeObjectCode(&tmp);
        fclose(f);
        return -1;
    }
    if (sym_count > 0) {
        tmp.obj_sym = (ObjSymbol*)calloc((size_t)sym_count, sizeof(ObjSymbol));
        if (!tmp.obj_sym) {
            freeObjectCode(&tmp);
            fclose(f);
            return -1;
        }
        tmp.obj_sym_size = (i32)sym_count;
        for (uint32_t i = 0; i < sym_count; ++i) {
            tmp.obj_sym[i].name = NULL;
            tmp.obj_sym[i].type = NULL;
            tmp.obj_sym[i].isOnlyRead = false;

            if (read_wcs_alloc(f, &tmp.obj_sym[i].name) != 0) {
                freeObjectCode(&tmp);
                fclose(f);
                return -1;
            }
            if (read_wcs_alloc(f, &tmp.obj_sym[i].type) != 0) {
                freeObjectCode(&tmp);
                fclose(f);
                return -1;
            }
            uint8_t ro;
            if (read_uint8(f, &ro) != 0) {
                freeObjectCode(&tmp);
                fclose(f);
                return -1;
            }
            tmp.obj_sym[i].isOnlyRead = (ro != 0);
        }
    }

    /* 成功：先释放目标全局变量已有内容，再把 tmp 移交给目标全局变量 hsmCode。 */
    /* 假设 hsmCode 在别处定义为全局变量 */
    freeObjectCode(&hsmCode); /* 释放原有内容（若有）*/
    hsmCode.start_fun = tmp.start_fun;
    hsmCode.obj_fun_size = tmp.obj_fun_size;
    hsmCode.obj_fun = tmp.obj_fun;
    hsmCode.obj_sym_size = tmp.obj_sym_size;
    hsmCode.obj_sym = tmp.obj_sym;

    /* 避免 freeObjectCode 再次释放 tmp 内容（tmp 已转移所有权） */
    tmp.obj_fun = NULL;
    tmp.obj_fun_size = 0;
    tmp.obj_sym = NULL;
    tmp.obj_sym_size = 0;

    fclose(f);
    return 0;
}


#endif