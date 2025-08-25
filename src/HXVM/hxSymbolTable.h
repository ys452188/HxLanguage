#ifndef HX_SYMBOL_TABLE_H
#define HX_SYMBOL_TABLE_H
#include <stdlib.h>
#include <wchar.h>
#include <stdint.h>
#include "hxLocale.h"

#define HASH_VALUE 2166136261U
#define PRIME 16777619U
#define SYMBOL_TABLE_INITIAL_SIZE 16  //符号表初始值

typedef struct Symbol {
    unsigned int hash;
    wchar* name;
    wchar* type;
    bool isOnlyRead;
    bool isAllocHeap;  //address是否是解释中分配的内存
    void* address;       //地址
} Symbol;
typedef struct SymbolTable {
    Symbol* symbol;
    int size;
    int sym_count;    //元素数量
} SymbolTable;

int initSymbolTable(SymbolTable*);
unsigned int getHashValue(const wchar*);    //获取字符串的哈希值(FNV-1a算法)
int resize(SymbolTable*);                                 //扩容
int insert(Symbol*, SymbolTable*);                        //插入操作,插入元素是引用objCode的,由freeObjectCode释放,不用再写一个freeSymbleTable
void delete(Symbol*, SymbolTable*);                       //删除操作
int findSymbol(wchar* name, SymbolTable* table);          //查找
void freeSymbolTable(SymbolTable* table) {
    if(!table) return;
    if(!(table->symbol)||table->sym_count==0) return;
    for(int i = 0; i < table->size; i++) {
        if(table->symbol[i].name) {
            free(table->symbol[i].name);
            table->symbol[i].name = NULL;
        }
        if(table->symbol[i].type) {
            free(table->symbol[i].type);
            table->symbol[i].type = NULL;
        }
        if(table->symbol[i].isAllocHeap) {
            free(table->symbol[i].address);
        }
        table->symbol[i].address = NULL;
    }
    free(table->symbol);
    return;
}
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
    float loadFactor = (float)table->sym_count / (float)table->size;
    if(loadFactor >= 0.75) {   //利用率过高
        int err = resize(table);
        if(err) return err;
    }
    sym->hash = getHashValue(sym->name);
    //printf("%u\n", sym->hash);
    int sym_index = (sym->hash)%(table->size);
    if(table->symbol[sym_index].name != NULL) {   //发生哈希碰撞
        int err = resize(table);
        if(err) return err;
    }

    table->sym_count++;
    table->symbol[sym_index].hash = sym->hash;
    table->symbol[sym_index].name = sym->name;
    table->symbol[sym_index].type = sym->type;
    table->symbol[sym_index].address = sym->address;
    table->symbol[sym_index].isOnlyRead = sym->isOnlyRead;
#ifdef SHOW_HX_DEBUG_DETAIL
    wprintf(L"\33[33m[DEG]\33[0m已将符号\33[36m[%ls(%ls)]\33[0m插入符号表\n", table->symbol[sym_index].name, table->symbol[sym_index].type);
#endif
    return 0;
}
int resize(SymbolTable* table) {
    if(table == NULL) return -1;
    int new_size = table->size+SYMBOL_TABLE_INITIAL_SIZE;
    Symbol* temp = (Symbol*)calloc(new_size, sizeof(Symbol));
    if(!temp) return -1;
    for(int i = 0; i < table->size; i++) {
        if(table->symbol[i].name) {
            //printf("%ls\n", table->symbol[i].name);
            //printf("hash:%u\n", table->symbol[i].hash);
            int new_index = (table->symbol[i].hash)%new_size;
            //printf("index:%d\n",new_index);

            temp[new_index].name = table->symbol[i].name;
            temp[new_index].type = table->symbol[i].type;
            temp[new_index].address = table->symbol[i].address;
            temp[new_index].isOnlyRead = table->symbol[i].isOnlyRead;
            temp[new_index].hash = table->symbol[i].hash;
        }
    }
    free(table->symbol);
    table->symbol = (Symbol*)temp;
    table->size = new_size;
#ifdef SHOW_HX_DEBUG_DETAIL
    wprintf(L"\33[33m[DEG]\33[0m符号表已扩容,当前容量：%d,已存储元素%d个\n",table->size, table->sym_count);
#endif
    return 0;
}
int findSymbol(wchar* name, SymbolTable* table) {
    if(table == NULL) return -1;
    if(table->sym_count == 0) return -1;
    if(name == NULL) return -1;
    int index = (int)(getHashValue(name)%table->size);
    if(table->symbol[index].name == NULL || table->symbol[index].type == NULL) return -1;
    return index;
}
#endif