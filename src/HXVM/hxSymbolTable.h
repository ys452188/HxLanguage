#ifndef HX_SYMBOL_TABLE_H
#define HX_SYMBOL_TABLE_H
#include <stdlib.h>
#include <wchar.h>
#include <stdint.h>
#include "hxLocale.h"

#define HASH_VALUE 2166136261U
#define PRIME 16777619U
#define SYMBOL_TABLE_INITIAL_SIZE 64  //符号表初始值

typedef struct Symbol {
    wchar* name;
    wchar* type;
    bool isOnlyRead;
    void* address;       //地址
} Symbol;
typedef struct SymbolTable {
    Symbol* symbol;
    int size;
    int sym_count;    //元素数量
} SymbolTable;

int initSymbolTable(SymbolTable*);
unsigned int getHashValue(const wchar*);    //获取字符串的哈希值(FNV-1a算法)
int insert(Symbol*, SymbolTable*);                        //插入操作,插入元素是引用objCode的,由freeObjectCode释放,不用再写一个freeSymbleTable
void delete(Symbol*, SymbolTable*);                       //删除操作

unsigned int getHashValue(const wchar* str) {
    if(str==NULL) return -1;
    int index = 0;
    int hash = HASH_VALUE;
    while(str[index] != L'\0') {
        hash = hash ^ (int)(str[index]);
        hash = hash * PRIME;
        index++;
    }
    return hash;
}
int initSymbolTable(SymbolTable* table) {
    if(table == NULL) return -1;
    memset(table, 0, sizeof(SymbolTable));
    table->size = SYMBOL_TABLE_INITIAL_SIZE;
    table->symbol = (Symbol*)calloc(table->size, sizeof(Symbol));
    if(!(table->symbol)) return -1;
#ifdef SHOW_HX_DEBUG_DETAIL
    wprintf(L"\33[33m[DEG]\33[0m已分配符号表内存, 大小为%d*sizeof(Symbol)\n", table->size);
#endif
    return 0;
}
int insert(Symbol* sym, SymbolTable* table) {
    if(sym == NULL) return -1;
    if(sym->name == NULL || sym->type == NULL) return -1;
    //计算负载因子
    float loadFactor = (table->sym_count)/(table->size);
    if(loadFactor >= 0.75) {   //利用率过高

    }
    unsigned int sym_hash = getHashValue(sym->name);
    int sym_index = sym_hash%(table->size);
    if(table->symbol[sym_index].name != NULL) return -1;

    table->symbol[sym_index].name = sym->name;
    table->symbol[sym_index].type = sym->type;
    table->symbol[sym_index].address = sym->address;
    table->symbol[sym_index].isOnlyRead = sym->isOnlyRead;
#ifdef SHOW_HX_DEBUG_DETAIL
    wprintf(L"\33[33m[DEG]\33[0m已将符号%ls(%ls)插入符号表\n", table->symbol[sym_index].name, table->symbol[sym_index].type);
#endif
    return 0;
}
#endif