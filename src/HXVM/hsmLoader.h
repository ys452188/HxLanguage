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
#include "hxLocale.h"

typedef uint32_t i32;

typedef enum OPCode {   //操作码
    OP_PUT_STR,         //从栈中输出wchar_t*
    OP_DEFINE_VAR,      //定义变量,第一个操作数为变量名,第二个为类型
    OP_PUSH,
    OP_ADD,
} OPCode;
typedef struct ObjValue {
    void* value;
    int size;
    enum {
        TYPE_SYM,
        TYPE_STR,
        TYPE_DOUBLE,
        TYPE_FLOAT,
        TYPE_INT,
        TYPE_CH,
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


/* ---------- 辅助读取函数 ---------- */

// 读取 32 位整数，并将其存储在指针 v 所指向的内存中
int read_int32(FILE* f, int32_t* v) {
    if (fread(v, sizeof(*v), 1, f) != 1) {
        return -1; // 读取失败
    }
    // 注意：这里没有进行字节序转换。如果需要，请在此处添加。
    return 0; // 读取成功
}
wchar_t* read_wstring_alloc(FILE* f) {
    int32_t len;
    if (read_int32(f, &len) != 0) {
        return NULL; // 读取长度失败
    }

    if (len == 0) {
        return NULL;
    }

    // 分配内存以存储 UTF-16 编码的字符
    uint16_t* buf = (uint16_t*)malloc(len * sizeof(uint16_t));
    if (buf == NULL) {
        return NULL;
    }

    // 从文件中读取 UTF-16 字符
    if (fread(buf, sizeof(uint16_t), (size_t)len, f) != (size_t)len) {
        free(buf);
        return NULL;
    }

    // 将 UTF-16 转换回 wchar_t
    wchar_t* wstr = (wchar_t*)calloc(len + 1, sizeof(wchar_t));
    if (wstr == NULL) {
        free(buf);
        return NULL;
    }

    for (int i = 0; i < len; ++i) {
        wstr[i] = (wchar_t)buf[i];
    }

    free(buf);
    return wstr;
}

/* 释放一个 ObjectCode（局部用，安全检查 NULL）——用于出错时清理 tmp */
static void free_objectcode_local(ObjectCode* c) {
    if (!c) return;
    /* 函数表 */
    if (c->obj_fun) {
        for (int fi = 0; fi < (int)c->obj_fun_size; ++fi) {
            ObjFunction* fn = &c->obj_fun[fi];
            if (fn->name) {
                free(fn->name);
                fn->name = NULL;
            }
            if (fn->ret_type) {
                free(fn->ret_type);
                fn->ret_type = NULL;
            }

            if (fn->args) {
                for (int ai = 0; ai < (int)fn->args_size; ++ai) {
                    if (fn->args[ai].name) {
                        free(fn->args[ai].name);
                        fn->args[ai].name = NULL;
                    }
                    if (fn->args[ai].type) {
                        free(fn->args[ai].type);
                        fn->args[ai].type = NULL;
                    }
                }
                free(fn->args);
                fn->args = NULL;
                fn->args_size = 0;
            }

            if (fn->body) {
                for (int bi = 0; bi < (int)fn->body_size; ++bi) {
                    Command* cmd = &fn->body[bi];
                    if (cmd->op_value) {
                        for (int vi = 0; vi < (int)cmd->op_value_size; ++vi) {
                            ObjValue* ov = &cmd->op_value[vi];
                            if (ov->value) {
                                free(ov->value);
                                ov->value = NULL;
                            }
                            ov->size = 0;
                        }
                        free(cmd->op_value);
                        cmd->op_value = NULL;
                        cmd->op_value_size = 0;
                    }
                }
                free(fn->body);
                fn->body = NULL;
                fn->body_size = 0;
            }
        }
        free(c->obj_fun);
        c->obj_fun = NULL;
        c->obj_fun_size = 0;
    }

    /* 全局符号 */
    if (c->obj_sym) {
        for (int si = 0; si < (int)c->obj_sym_size; ++si) {
            if (c->obj_sym[si].name) {
                free(c->obj_sym[si].name);
                c->obj_sym[si].name = NULL;
            }
            if (c->obj_sym[si].type) {
                free(c->obj_sym[si].type);
                c->obj_sym[si].type = NULL;
            }
        }
        free(c->obj_sym);
        c->obj_sym = NULL;
        c->obj_sym_size = 0;
    }

    /* 类表 */
    if (c->obj_class) {
        for (int ci = 0; ci < (int)c->obj_class_size; ++ci) {
            ObjClass* cls = &c->obj_class[ci];
            if (cls->name) {
                free(cls->name);
                cls->name = NULL;
            }

            if (cls->pub_sym) {
                for (int m = 0; m < cls->pub_sym_size; ++m) {
                    if (cls->pub_sym[m].name) {
                        free(cls->pub_sym[m].name);
                        cls->pub_sym[m].name = NULL;
                    }
                    if (cls->pub_sym[m].type) {
                        free(cls->pub_sym[m].type);
                        cls->pub_sym[m].type = NULL;
                    }
                }
                free(cls->pub_sym);
                cls->pub_sym = NULL;
                cls->pub_sym_size = 0;
            }
            if (cls->pri_sym) {
                for (int m = 0; m < cls->pri_sym_size; ++m) {
                    if (cls->pri_sym[m].name) {
                        free(cls->pri_sym[m].name);
                        cls->pri_sym[m].name = NULL;
                    }
                    if (cls->pri_sym[m].type) {
                        free(cls->pri_sym[m].type);
                        cls->pri_sym[m].type = NULL;
                    }
                }
                free(cls->pri_sym);
                cls->pri_sym = NULL;
                cls->pri_sym_size = 0;
            }
            if (cls->pro_sym) {
                for (int m = 0; m < cls->pro_sym_size; ++m) {
                    if (cls->pro_sym[m].name) {
                        free(cls->pro_sym[m].name);
                        cls->pro_sym[m].name = NULL;
                    }
                    if (cls->pro_sym[m].type) {
                        free(cls->pro_sym[m].type);
                        cls->pro_sym[m].type = NULL;
                    }
                }
                free(cls->pro_sym);
                cls->pro_sym = NULL;
                cls->pro_sym_size = 0;
            }

            /* 类函数：复用函数释放逻辑 */
            if (cls->pub_fun) {
                for (int f = 0; f < cls->pub_fun_size; ++f) {
                    ObjFunction* fn = &cls->pub_fun[f];
                    if (fn->name) {
                        free(fn->name);
                        fn->name = NULL;
                    }
                    if (fn->ret_type) {
                        free(fn->ret_type);
                        fn->ret_type = NULL;
                    }
                    if (fn->args) {
                        for (int a = 0; a < fn->args_size; ++a) {
                            if (fn->args[a].name) {
                                free(fn->args[a].name);
                                fn->args[a].name = NULL;
                            }
                            if (fn->args[a].type) {
                                free(fn->args[a].type);
                                fn->args[a].type = NULL;
                            }
                        }
                        free(fn->args);
                        fn->args = NULL;
                    }
                    if (fn->body) {
                        for (int b = 0; b < fn->body_size; ++b) {
                            if (fn->body[b].op_value) {
                                for (int v = 0; v < fn->body[b].op_value_size; ++v) {
                                    if (fn->body[b].op_value[v].value) {
                                        free(fn->body[b].op_value[v].value);
                                        fn->body[b].op_value[v].value = NULL;
                                    }
                                }
                                free(fn->body[b].op_value);
                                fn->body[b].op_value = NULL;
                            }
                        }
                        free(fn->body);
                        fn->body = NULL;
                    }
                }
                free(cls->pub_fun);
                cls->pub_fun = NULL;
                cls->pub_fun_size = 0;
            }

            if (cls->pri_fun) {
                for (int f = 0; f < cls->pri_fun_size; ++f) {
                    ObjFunction* fn = &cls->pri_fun[f];
                    if (fn->name) {
                        free(fn->name);
                        fn->name = NULL;
                    }
                    if (fn->ret_type) {
                        free(fn->ret_type);
                        fn->ret_type = NULL;
                    }
                    if (fn->args) {
                        for (int a = 0; a < fn->args_size; ++a) {
                            if (fn->args[a].name) {
                                free(fn->args[a].name);
                                fn->args[a].name = NULL;
                            }
                            if (fn->args[a].type) {
                                free(fn->args[a].type);
                                fn->args[a].type = NULL;
                            }
                        }
                        free(fn->args);
                        fn->args = NULL;
                    }
                    if (fn->body) {
                        for (int b = 0; b < fn->body_size; ++b) {
                            if (fn->body[b].op_value) {
                                for (int v = 0; v < fn->body[b].op_value_size; ++v) {
                                    if (fn->body[b].op_value[v].value) {
                                        free(fn->body[b].op_value[v].value);
                                        fn->body[b].op_value[v].value = NULL;
                                    }
                                }
                                free(fn->body[b].op_value);
                                fn->body[b].op_value = NULL;
                            }
                        }
                        free(fn->body);
                        fn->body = NULL;
                    }
                }
                free(cls->pri_fun);
                cls->pri_fun = NULL;
                cls->pri_fun_size = 0;
            }

            if (cls->pro_fun) {
                for (int f = 0; f < cls->pro_fun_suze; ++f) {
                    ObjFunction* fn = &cls->pro_fun[f];
                    if (fn->name) {
                        free(fn->name);
                        fn->name = NULL;
                    }
                    if (fn->ret_type) {
                        free(fn->ret_type);
                        fn->ret_type = NULL;
                    }
                    if (fn->args) {
                        for (int a = 0; a < fn->args_size; ++a) {
                            if (fn->args[a].name) {
                                free(fn->args[a].name);
                                fn->args[a].name = NULL;
                            }
                            if (fn->args[a].type) {
                                free(fn->args[a].type);
                                fn->args[a].type = NULL;
                            }
                        }
                        free(fn->args);
                        fn->args = NULL;
                    }
                    if (fn->body) {
                        for (int b = 0; b < fn->body_size; ++b) {
                            if (fn->body[b].op_value) {
                                for (int v = 0; v < fn->body[b].op_value_size; ++v) {
                                    if (fn->body[b].op_value[v].value) {
                                        free(fn->body[b].op_value[v].value);
                                        fn->body[b].op_value[v].value = NULL;
                                    }
                                }
                                free(fn->body[b].op_value);
                                fn->body[b].op_value = NULL;
                            }
                        }
                        free(fn->body);
                        fn->body = NULL;
                    }
                }
                free(cls->pro_fun);
                cls->pro_fun = NULL;
                cls->pro_fun_suze = 0;
            }
        }
        free(c->obj_class);
        c->obj_class = NULL;
        c->obj_class_size = 0;
    }

    c->start_fun = 0;
}

/* ---------- 读取辅助：逐项读取并构造 tmp ---------- */

static int read_symbol_into(FILE* f, ObjSymbol* dst) {
    dst->name = read_wstring_alloc(f);
    /* if name==NULL that's acceptable (writer uses 0 len for NULL) */
    dst->type = read_wstring_alloc(f);
    if (fread(&dst->isOnlyRead, sizeof(bool), 1, f) != 1) return -1;
    return 0;
}

static int read_command_into(FILE* f, Command* cmd) {
    int32_t op;
    if (read_int32(f, &op) != 0) return -1;
    cmd->op = (OPCode)op;

    int32_t op_val_cnt;
    if (read_int32(f, &op_val_cnt) != 0) return -1;
    cmd->op_value_size = (i32)op_val_cnt;
    cmd->op_value = NULL;

    if (op_val_cnt > 0) {
        cmd->op_value = (ObjValue*)calloc((size_t)op_val_cnt, sizeof(ObjValue));
        if (!cmd->op_value) return -1;
        for (int i = 0; i < op_val_cnt; ++i) {
            int32_t type;
            if (read_int32(f, &type) != 0) return -1;
            cmd->op_value[i].type = (int)type;
            cmd->op_value[i].value = NULL;
            cmd->op_value[i].size = 0;

            if (cmd->op_value[i].type == TYPE_STR || cmd->op_value[i].type == TYPE_SYM) {
                wchar_t* s = read_wstring_alloc(f);
                if (s == NULL) {
                    /* writer used len=0 for NULL -> interpret as NULL */
                    cmd->op_value[i].value = NULL;
                    cmd->op_value[i].size = 0;
                } else {
                    cmd->op_value[i].value = (void*)s;
                    /* size: 字符串按 wchar 存储，记录字节数（含结尾） */
                    size_t slen = wcslen(s);
                    cmd->op_value[i].size = (int)((slen + 1) * sizeof(wchar_t));
                }
            } else {
                /* 非字符串类型：先读 size (int32)，若 size>0 则分配并读 raw bytes */
                int32_t raw_size;
                if (read_int32(f, &raw_size) != 0) return -1;
                if (raw_size > 0) {
                    void* buf = malloc((size_t)raw_size);
                    if (!buf) return -1;
                    if (fread(buf, 1, (size_t)raw_size, f) != (size_t)raw_size) {
                        free(buf);
                        return -1;
                    }
                    cmd->op_value[i].value = buf;
                    cmd->op_value[i].size = raw_size;
                } else {
                    cmd->op_value[i].value = NULL;
                    cmd->op_value[i].size = 0;
                }
            }
        }
    }
    return 0;
}

static int read_function_into(FILE* f, ObjFunction* fn) {
    fn->name = read_wstring_alloc(f);
    fn->ret_type = read_wstring_alloc(f);

    int32_t args_cnt;
    if (read_int32(f, &args_cnt) != 0) return -1;
    fn->args_size = (i32)args_cnt;
    fn->args = NULL;
    if (args_cnt > 0) {
        fn->args = (ObjSymbol*)calloc((size_t)args_cnt, sizeof(ObjSymbol));
        if (!fn->args) return -1;
        for (int i = 0; i < args_cnt; ++i) {
            if (read_symbol_into(f, &fn->args[i]) != 0) return -1;
        }
    }

    int32_t body_cnt;
    if (read_int32(f, &body_cnt) != 0) return -1;
    fn->body_size = (i32)body_cnt;
    fn->body = NULL;
    if (body_cnt > 0) {
        fn->body = (Command*)calloc((size_t)body_cnt, sizeof(Command));
        if (!fn->body) return -1;
        for (int i = 0; i < body_cnt; ++i) {
            if (read_command_into(f, &fn->body[i]) != 0) return -1;
        }
    }
    return 0;
}

static ClassMember* read_class_members_alloc(FILE* f, int* out_count) {
    int32_t cnt;
    if (read_int32(f, &cnt) != 0) return NULL;
    *out_count = cnt;
    if (cnt <= 0) return NULL;
    ClassMember* arr = (ClassMember*)calloc((size_t)cnt, sizeof(ClassMember));
    if (!arr) return NULL;
    for (int i = 0; i < cnt; ++i) {
        arr[i].name = read_wstring_alloc(f);
        arr[i].type = read_wstring_alloc(f);
        if (fread(&arr[i].isOnlyRead, sizeof(bool), 1, f) != 1) {
            free(arr);
            return NULL;
        }
        if (fread(&arr[i].offest, sizeof(int), 1, f) != 1) {
            free(arr);
            return NULL;
        }
    }
    return arr;
}

static int read_class_into(FILE* f, ObjClass* cls) {
    cls->name = read_wstring_alloc(f);

    cls->pub_sym = read_class_members_alloc(f, &cls->pub_sym_size);
    cls->pri_sym = read_class_members_alloc(f, &cls->pri_sym_size);
    cls->pro_sym = read_class_members_alloc(f, &cls->pro_sym_size);

    /* pub_fun */
    int32_t pub_fun_cnt;
    if (read_int32(f, &pub_fun_cnt) != 0) return -1;
    cls->pub_fun_size = pub_fun_cnt;
    cls->pub_fun = NULL;
    if (pub_fun_cnt > 0) {
        cls->pub_fun = (ObjFunction*)calloc((size_t)pub_fun_cnt, sizeof(ObjFunction));
        if (!cls->pub_fun) return -1;
        for (int i = 0; i < pub_fun_cnt; ++i) {
            if (read_function_into(f, &cls->pub_fun[i]) != 0) return -1;
        }
    }

    /* pri_fun */
    int32_t pri_fun_cnt;
    if (read_int32(f, &pri_fun_cnt) != 0) return -1;
    cls->pri_fun_size = pri_fun_cnt;
    cls->pri_fun = NULL;
    if (pri_fun_cnt > 0) {
        cls->pri_fun = (ObjFunction*)calloc((size_t)pri_fun_cnt, sizeof(ObjFunction));
        if (!cls->pri_fun) return -1;
        for (int i = 0; i < pri_fun_cnt; ++i) {
            if (read_function_into(f, &cls->pri_fun[i]) != 0) return -1;
        }
    }

    /* pro_fun */
    int32_t pro_fun_cnt;
    if (read_int32(f, &pro_fun_cnt) != 0) return -1;
    cls->pro_fun_suze = pro_fun_cnt;
    cls->pro_fun = NULL;
    if (pro_fun_cnt > 0) {
        cls->pro_fun = (ObjFunction*)calloc((size_t)pro_fun_cnt, sizeof(ObjFunction));
        if (!cls->pro_fun) return -1;
        for (int i = 0; i < pro_fun_cnt; ++i) {
            if (read_function_into(f, &cls->pro_fun[i]) != 0) return -1;
        }
    }
    return 0;
}

/* ---------- 主函数：loadObjectCode ---------- */
/* 读取失败返回 -1；成功返回 0，并把数据移动到全局 hsmCode（释放原 hsmCode 的内容） */
int loadObjectFile(const char* file_name) {
    if (file_name == NULL) return -1;
    FILE* f = fopen(file_name, "rb");
    if (!f) return -1;

    ObjectCode tmp;
    memset(&tmp, 0, sizeof(ObjectCode));

    /* start_fun */
    int32_t start;
    if (read_int32(f, &start) != 0) {
        fclose(f);
        return -1;
    }
    tmp.start_fun = (i32)start;

    /* obj_fun */
    int32_t fun_count;
    if (read_int32(f, &fun_count) != 0) {
        fclose(f);
        return -1;
    }
    tmp.obj_fun_size = (i32)fun_count;
    tmp.obj_fun = NULL;
    if (fun_count > 0) {
        tmp.obj_fun = (ObjFunction*)calloc((size_t)fun_count, sizeof(ObjFunction));
        if (!tmp.obj_fun) {
            fclose(f);
            return -1;
        }
        for (int i = 0; i < fun_count; ++i) {
            if (read_function_into(f, &tmp.obj_fun[i]) != 0) {
                free_objectcode_local(&tmp);
                fclose(f);
                return -1;
            }
        }
    }

    /* obj_sym */
    int32_t sym_count;
    if (read_int32(f, &sym_count) != 0) {
        free_objectcode_local(&tmp);
        fclose(f);
        return -1;
    }
    tmp.obj_sym_size = (i32)sym_count;
    tmp.obj_sym = NULL;
    if (sym_count > 0) {
        tmp.obj_sym = (ObjSymbol*)calloc((size_t)sym_count, sizeof(ObjSymbol));
        if (!tmp.obj_sym) {
            free_objectcode_local(&tmp);
            fclose(f);
            return -1;
        }
        for (int i = 0; i < sym_count; ++i) {
            if (read_symbol_into(f, &tmp.obj_sym[i]) != 0) {
                free_objectcode_local(&tmp);
                fclose(f);
                return -1;
            }
        }
    }

    /* obj_class */
    int32_t class_count;
    if (read_int32(f, &class_count) != 0) {
        free_objectcode_local(&tmp);
        fclose(f);
        return -1;
    }
    tmp.obj_class_size = (i32)class_count;
    tmp.obj_class = NULL;
    if (class_count > 0) {
        tmp.obj_class = (ObjClass*)calloc((size_t)class_count, sizeof(ObjClass));
        if (!tmp.obj_class) {
            free_objectcode_local(&tmp);
            fclose(f);
            return -1;
        }
        for (int i = 0; i < class_count; ++i) {
            if (read_class_into(f, &tmp.obj_class[i]) != 0) {
                free_objectcode_local(&tmp);
                fclose(f);
                return -1;
            }
        }
    }

    /* 成功：释放旧全局 hsmCode 的内容（若需要）并把 tmp 移交 */
    free_objectcode_local(&hsmCode); /* 假定 hsmCode 已声明为全局并可能已有内容 */
    hsmCode = tmp; /* 结构体赋值：移动所有权 */

    fclose(f);
    return 0;
}
void freeObjectCode(ObjectCode *code) {
    if (code == NULL) return;
    /* ---------- 释放全局函数表 ---------- */
    if (code->obj_fun) {
        /* obj_fun_size 是 i32 (typedef uint32_t i32)，用 unsigned 循环更稳妥 */
        for (uint32_t fi = 0; fi < (uint32_t)code->obj_fun_size; ++fi) {
            ObjFunction *fn = &code->obj_fun[fi];
            /* 释放函数名与返回类型 */
            if (fn->name) {
                free(fn->name);
                fn->name = NULL;
            }
            if (fn->ret_type) {
                free(fn->ret_type);
                fn->ret_type = NULL;
            }
            /* 释放参数表 */
            if (fn->args) {
                for (uint32_t ai = 0; ai < (uint32_t)fn->args_size; ++ai) {
                    if (fn->args[ai].name) {
                        free(fn->args[ai].name);
                        fn->args[ai].name = NULL;
                    }
                    if (fn->args[ai].type) {
                        free(fn->args[ai].type);
                        fn->args[ai].type = NULL;
                    }
                }
                free(fn->args);
                fn->args = NULL;
                fn->args_size = 0;
            }

            if (fn->body) {
                for (uint32_t bi = 0; bi < (uint32_t)fn->body_size; ++bi) {
                    Command *cmd = &fn->body[bi];
                    if (cmd->op_value) {
                        for (uint32_t vi = 0; vi < (uint32_t)cmd->op_value_size; ++vi) {
                            ObjValue *ov = &cmd->op_value[vi];
                            if (ov->value) {
                                free(ov->value);
                                ov->value = NULL;
                            }
                            ov->size = 0;
                        }
                        free(cmd->op_value);
                        cmd->op_value = NULL;
                        cmd->op_value_size = 0;
                    }
                }
                free(fn->body);
                fn->body = NULL;
                fn->body_size = 0;
            }
        }
        free(code->obj_fun);
        code->obj_fun = NULL;
        code->obj_fun_size = 0;
    }
    /* ---------- 释放全局符号表 ---------- */
    if (code->obj_sym) {
        for (uint32_t si = 0; si < (uint32_t)code->obj_sym_size; ++si) {
            if (code->obj_sym[si].name) {
                free(code->obj_sym[si].name);
                code->obj_sym[si].name = NULL;
            }
            if (code->obj_sym[si].type) {
                free(code->obj_sym[si].type);
                code->obj_sym[si].type = NULL;
            }
        }
        free(code->obj_sym);
        code->obj_sym = NULL;
        code->obj_sym_size = 0;
    }

    /* ---------- 释放类表及类内部内容 ---------- */
    if (code->obj_class) {
        for (uint32_t ci = 0; ci < (uint32_t)code->obj_class_size; ++ci) {
            ObjClass *cls = &code->obj_class[ci];

            /* 类名 */
            if (cls->name) {
                free(cls->name);
                cls->name = NULL;
            }

            /* 成员数组（公/私/保护） */
            if (cls->pub_sym) {
                for (int m = 0; m < cls->pub_sym_size; ++m) {
                    if (cls->pub_sym[m].name) {
                        free(cls->pub_sym[m].name);
                        cls->pub_sym[m].name = NULL;
                    }
                    if (cls->pub_sym[m].type) {
                        free(cls->pub_sym[m].type);
                        cls->pub_sym[m].type = NULL;
                    }
                }
                free(cls->pub_sym);
                cls->pub_sym = NULL;
                cls->pub_sym_size = 0;
            }

            if (cls->pri_sym) {
                for (int m = 0; m < cls->pri_sym_size; ++m) {
                    if (cls->pri_sym[m].name) {
                        free(cls->pri_sym[m].name);
                        cls->pri_sym[m].name = NULL;
                    }
                    if (cls->pri_sym[m].type) {
                        free(cls->pri_sym[m].type);
                        cls->pri_sym[m].type = NULL;
                    }
                }
                free(cls->pri_sym);
                cls->pri_sym = NULL;
                cls->pri_sym_size = 0;
            }

            if (cls->pro_sym) {
                for (int m = 0; m < cls->pro_sym_size; ++m) {
                    if (cls->pro_sym[m].name) {
                        free(cls->pro_sym[m].name);
                        cls->pro_sym[m].name = NULL;
                    }
                    if (cls->pro_sym[m].type) {
                        free(cls->pro_sym[m].type);
                        cls->pro_sym[m].type = NULL;
                    }
                }
                free(cls->pro_sym);
                cls->pro_sym = NULL;
                cls->pro_sym_size = 0;
            }

            /* 类函数：pub_fun, pri_fun, pro_fun —— 释放逻辑同全局函数 */
            if (cls->pub_fun) {
                for (int f = 0; f < cls->pub_fun_size; ++f) {
                    ObjFunction *fn = &cls->pub_fun[f];
                    if (fn->name) {
                        free(fn->name);
                        fn->name = NULL;
                    }
                    if (fn->ret_type) {
                        free(fn->ret_type);
                        fn->ret_type = NULL;
                    }

                    if (fn->args) {
                        for (int a = 0; a < fn->args_size; ++a) {
                            if (fn->args[a].name) {
                                free(fn->args[a].name);
                                fn->args[a].name = NULL;
                            }
                            if (fn->args[a].type) {
                                free(fn->args[a].type);
                                fn->args[a].type = NULL;
                            }
                        }
                        free(fn->args);
                        fn->args = NULL;
                        fn->args_size = 0;
                    }

                    if (fn->body) {
                        for (int b = 0; b < fn->body_size; ++b) {
                            Command *cmd = &fn->body[b];
                            if (cmd->op_value) {
                                for (int v = 0; v < cmd->op_value_size; ++v) {
                                    if (cmd->op_value[v].value) {
                                        free(cmd->op_value[v].value);
                                        cmd->op_value[v].value = NULL;
                                    }
                                    cmd->op_value[v].size = 0;
                                }
                                free(cmd->op_value);
                                cmd->op_value = NULL;
                                cmd->op_value_size = 0;
                            }
                        }
                        free(fn->body);
                        fn->body = NULL;
                        fn->body_size = 0;
                    }
                }
                free(cls->pub_fun);
                cls->pub_fun = NULL;
                cls->pub_fun_size = 0;
            }

            if (cls->pri_fun) {
                for (int f = 0; f < cls->pri_fun_size; ++f) {
                    ObjFunction *fn = &cls->pri_fun[f];
                    if (fn->name) {
                        free(fn->name);
                        fn->name = NULL;
                    }
                    if (fn->ret_type) {
                        free(fn->ret_type);
                        fn->ret_type = NULL;
                    }

                    if (fn->args) {
                        for (int a = 0; a < fn->args_size; ++a) {
                            if (fn->args[a].name) {
                                free(fn->args[a].name);
                                fn->args[a].name = NULL;
                            }
                            if (fn->args[a].type) {
                                free(fn->args[a].type);
                                fn->args[a].type = NULL;
                            }
                        }
                        free(fn->args);
                        fn->args = NULL;
                        fn->args_size = 0;
                    }

                    if (fn->body) {
                        for (int b = 0; b < fn->body_size; ++b) {
                            Command *cmd = &fn->body[b];
                            if (cmd->op_value) {
                                for (int v = 0; v < cmd->op_value_size; ++v) {
                                    if (cmd->op_value[v].value) {
                                        free(cmd->op_value[v].value);
                                        cmd->op_value[v].value = NULL;
                                    }
                                    cmd->op_value[v].size = 0;
                                }
                                free(cmd->op_value);
                                cmd->op_value = NULL;
                                cmd->op_value_size = 0;
                            }
                        }
                        free(fn->body);
                        fn->body = NULL;
                        fn->body_size = 0;
                    }
                }
                free(cls->pri_fun);
                cls->pri_fun = NULL;
                cls->pri_fun_size = 0;
            }
            if (cls->pro_fun) {
                for (int f = 0; f < cls->pro_fun_suze; ++f) {
                    ObjFunction *fn = &cls->pro_fun[f];
                    if (fn->name) {
                        free(fn->name);
                        fn->name = NULL;
                    }
                    if (fn->ret_type) {
                        free(fn->ret_type);
                        fn->ret_type = NULL;
                    }

                    if (fn->args) {
                        for (int a = 0; a < fn->args_size; ++a) {
                            if (fn->args[a].name) {
                                free(fn->args[a].name);
                                fn->args[a].name = NULL;
                            }
                            if (fn->args[a].type) {
                                free(fn->args[a].type);
                                fn->args[a].type = NULL;
                            }
                        }
                        free(fn->args);
                        fn->args = NULL;
                        fn->args_size = 0;
                    }

                    if (fn->body) {
                        for (int b = 0; b < fn->body_size; ++b) {
                            Command *cmd = &fn->body[b];
                            if (cmd->op_value) {
                                for (int v = 0; v < cmd->op_value_size; ++v) {
                                    if (cmd->op_value[v].value) {
                                        free(cmd->op_value[v].value);
                                        cmd->op_value[v].value = NULL;
                                    }
                                    cmd->op_value[v].size = 0;
                                }
                                free(cmd->op_value);
                                cmd->op_value = NULL;
                                cmd->op_value_size = 0;
                            }
                        }
                        free(fn->body);
                        fn->body = NULL;
                        fn->body_size = 0;
                    }
                }
                free(cls->pro_fun);
                cls->pro_fun = NULL;
                cls->pro_fun_suze = 0;
            }
        }
        free(code->obj_class);
        code->obj_class = NULL;
        code->obj_class_size = 0;
    }
    code->start_fun = 0;
    /* 把剩余内存清零（确保所有计数字段为0） */
    memset(code, 0, sizeof(ObjectCode));
}
#endif