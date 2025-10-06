#ifndef HX_LEXER_H
#define HX_LEXER_H
#include <wchar.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wctype.h>
#include "Error.h"

typedef enum TokenType {
    TOK_ID,            //标识符
    TOK_KW,            //关键字
    TOK_VAL,           //表面值
    TOK_OPR,           //操作符
    TOK_END,           //终结符
} TokenType;
typedef struct Token {
    TokenType type;
    wchar_t* value;
#define CH  0
#define STR 1
    char mark;  //标记类型
    int line;
} Token;
typedef struct Tokens {  //Token流
    int size;
    int count;
    Token* tokens;
} Tokens;
wchar_t* keyword[] = {   //关键字
    L"var\0", L"con\0", L"fun\0", L"class", L"定义变量\0", L"定义常量\0", L"定义函数\0", L"定义类",
    L"公有成员", L"私有成员",L"受保护成员", L"public", L"private", L"proctected",
    L"它的类型是", L"它的父类是", L"parent", NULL
};

static bool isKeyword(wchar_t* str);   //判断是否是关键字
static bool isOperator(wchar_t ch);  //判断是否是操作符
Tokens* lex(wchar_t* src, int* err);   //词法分析
void freeTokens(Tokens** tokens);      //释放

Tokens* lex(wchar_t* src, int* err) {
#define MEM_FAIL  {*err=-1;return NULL;}   //内存错误
#define ERR       {*err=255;return NULL;}  //语法错误
    if(err == NULL) return NULL;
    if(src == NULL) MEM_FAIL;
    Tokens* tokens = (Tokens*)calloc(1, sizeof(Tokens));
    if(!tokens) MEM_FAIL;
    int index_src = 0;
    int length_src = wcslen(src);
    int line = 1;           //当前行数
    int token_index = 0;
    tokens->tokens = (Token*)calloc(1, sizeof(Token));
    if(!(tokens->tokens)) MEM_FAIL;
    tokens->size = 1;
    for(index_src = 0; index_src < length_src; index_src++) {
        //printf("%lc\n", src[index_src]);
        if(src[index_src] == L'#') {   //处理注释
            while(index_src < length_src) {
                if(src[index_src] == L'\n') break;
                index_src++;
            }
            line++;
            continue;
        }
        if(token_index >= tokens->size) {
            int old_size = tokens->size;
            tokens->size = token_index+4;
            int new_alloc_size = tokens->size - old_size;
            void* temp = realloc(tokens->tokens, (tokens->size)*sizeof(Token));
            if(!temp) MEM_FAIL;
            tokens->tokens = (Token*)temp;
            //初始化新分配部分
            memset((tokens->tokens+old_size), 0, new_alloc_size*sizeof(Token));
#ifdef HX_DEBUG
            log(L"tokens大小增加了");
#endif
        }
        if(src[index_src] == L'\n') line++;
        if(iswspace(src[index_src])) continue;
        if(src[index_src] == L'\'' || src[index_src] == L'‘' || src[index_src] == L'’') {  //字符
            int start_index = index_src;
            int end_index = 0;
            while(index_src < length_src) {
                index_src++;
                if(src[index_src] == L'\n') line++;
                if(src[index_src] == L'\'' || src[index_src] == L'‘' || src[index_src] == L'’') {
                    end_index = index_src;
                    break;
                }
            }
            //字符没结尾
            if(end_index == 0) {
                wchar_t errCode[3] = {0};
                errCode[0] = src[start_index];
                if(start_index+1 < length_src) errCode[1] = src[start_index+1];
                setError(ERR_CH_NO_END, line, errCode);
                ERR;
            }
            start_index++;
            int len = end_index - start_index;
            tokens->tokens[token_index].mark = CH;
            tokens->tokens[token_index].type = TOK_VAL;
            tokens->tokens[token_index].value = (wchar_t*)calloc(len+1, sizeof(wchar_t));
            if(!(tokens->tokens[token_index].value)) MEM_FAIL;
            if(len != 0) wcsncpy(tokens->tokens[token_index].value, &(src[start_index]), len);
            tokens->tokens[token_index].line = line;
#ifdef HX_DEBUG
            log(L"已创建一个新的字符型Token");
#endif
            tokens->count++;
            token_index++;
        } else if(src[index_src] == L'\"' || src[index_src] == L'“' || src[index_src] == L'”') {   //字符串
            int start_index = index_src;
            int end_index = 0;
            while(index_src < length_src) {
                index_src++;
                if(src[index_src] == L'\n') line++;
                if(src[index_src] == L'\"' || src[index_src] == L'“' || src[index_src] == L'”') {
                    end_index = index_src;
                    break;
                }
            }
            //字符串没结尾
            if(end_index == 0) {
                wchar_t errCode[3] = {0};
                errCode[0] = src[start_index];
                if(start_index+1 < length_src) errCode[1] = src[start_index+1];
                setError(ERR_STR_NO_END, line, errCode);
                ERR;
            }
            start_index++;
            int len = end_index - start_index;
            tokens->tokens[token_index].mark = STR;
            tokens->tokens[token_index].type = TOK_VAL;
            tokens->tokens[token_index].value = (wchar_t*)calloc(len+1, sizeof(wchar_t));
            if(!(tokens->tokens[token_index].value)) MEM_FAIL;
            if(len != 0) wcsncpy(tokens->tokens[token_index].value, &(src[start_index]), len);
            tokens->tokens[token_index].line = line;
#ifdef HX_DEBUG
            log(L"已创建一个新的字符串型Token");
#endif
            token_index++;
            tokens->count++;
        } else if(src[index_src]==L';'||src[index_src]==L'；'||src[index_src]==L'。') {   //结束符
            tokens->tokens[token_index].type = TOK_END;
            tokens->tokens[token_index].value = (wchar_t*)calloc(2, sizeof(wchar_t));
            if(!(tokens->tokens[token_index].value)) MEM_FAIL;
            tokens->tokens[token_index].value[0] = src[index_src];
            tokens->tokens[token_index].line = line;
#ifdef HX_DEBUG
            log(L"已创建一个新的终结符Token");
#endif
            token_index++;
            tokens->count++;
        } else if(isOperator(src[index_src])) {   //操作符
            //printf("%lc\n", src[index_src]);
            //偷看是否为两个字符组成的操作符
            if(index_src+1 < length_src) {
                if((src[index_src]==L'>'&&src[index_src+1]==L'=') ||   //>=
                        (src[index_src]==L'<'&&src[index_src+1]==L'=') ||   //<=
                        (src[index_src]==L'='&&src[index_src+1]==L'=') ||   //==
                        (src[index_src]==L'+'&&src[index_src+1]==L'+') ||   //++
                        (src[index_src]==L'-'&&src[index_src+1]==L'-') ||    //--
                        (src[index_src]==L'-'&&src[index_src+1]==L'>')       //->
                  ) {
                    tokens->tokens[token_index].type = TOK_OPR;
                    tokens->tokens[token_index].value = (wchar_t*)calloc(3, sizeof(wchar_t));
                    if(!(tokens->tokens[token_index].value)) MEM_FAIL;
                    wcsncpy(tokens->tokens[token_index].value, &(src[index_src]), 2);
                    tokens->tokens[token_index].line = line;
                    index_src++;
#ifdef HX_DEBUG
                    log(L"已创建一个新的操作符Token");
#endif
                } else {
                    tokens->tokens[token_index].type = TOK_OPR;
                    tokens->tokens[token_index].value = (wchar_t*)calloc(2, sizeof(wchar_t));
                    if(!(tokens->tokens[token_index].value)) MEM_FAIL;
                    tokens->tokens[token_index].value[0] = src[index_src];
                    tokens->tokens[token_index].line = line;
#ifdef HX_DEBUG
                    log(L"已创建一个新的操作符Token");
#endif
                }
            } else {
                tokens->tokens[token_index].type = TOK_OPR;
                tokens->tokens[token_index].value = (wchar_t*)calloc(2, sizeof(wchar_t));
                if(!(tokens->tokens[token_index].value)) MEM_FAIL;
                tokens->tokens[token_index].value[0] = src[index_src];
                tokens->tokens[token_index].line = line;
#ifdef HX_DEBUG
                log(L"已创建一个新的操作符Token");
#endif
            }
            token_index++;
            tokens->count++;
        } else if(iswdigit(src[index_src])) {      //数字
            int start = index_src;
            while((src[index_src]!=L'\0')&&((!isOperator(src[index_src]))&&(!iswspace(src[index_src])))) {
                index_src++;
            }
            if(src[index_src]==L'.') {   //小数
                if(!iswdigit(src[index_src+1])) {   //114.  ->错误
                    int len = index_src+1 - start;
                    wchar_t* errCode = (wchar_t*)calloc(len+1, sizeof(wchar_t));
                    if(!errCode) MEM_FAIL;
                    wcsncpy(errCode, &(src[start]), len);
                    setError(ERR_VAL, line, errCode);
                    free(errCode);
                    errCode = NULL;
                    ERR;
                }
                index_src++;
                while((src[index_src]!=L'\0')&&((!isOperator(src[index_src]))&&(!iswspace(src[index_src])))) {
                    index_src++;
                }
            }
            int end = index_src;
            int len = end - start;
            tokens->tokens[token_index].type = TOK_VAL;
            tokens->tokens[token_index].value = (wchar_t*)calloc(len+1, sizeof(wchar_t));
            if(!(tokens->tokens[token_index].value)) MEM_FAIL;
            if(len != 0) wcsncpy(tokens->tokens[token_index].value, &(src[start]), len);
            tokens->tokens[token_index].line = line;
            //printf("%ls\n", tokens->tokens[token_index].value);
#ifdef HX_DEBUG
            log(L"已创建一个新的字面量Token");
#endif
            token_index++;
            //此时src[index_src]不为数字和字母,但在这次循环后index++,将导致忽略该Token,因此最后应index--;
            index_src--;
            tokens->count++;
        } else {   //标识符或关键字
            int start = index_src;
            while((src[index_src]!=L'\0')&&((!isOperator(src[index_src]))&&(!iswspace(src[index_src])))) {
                index_src++;
            }
            int end = index_src;
            int len = end - start;
            tokens->tokens[token_index].value = (wchar_t*)calloc(len+1, sizeof(wchar_t));
            if(!(tokens->tokens[token_index].value)) MEM_FAIL;
            if(len != 0) wcsncpy(tokens->tokens[token_index].value, &(src[start]), len);
            tokens->tokens[token_index].line = line;
            tokens->tokens[token_index].type = TOK_ID;
            if(isKeyword(tokens->tokens[token_index].value)) {
                tokens->tokens[token_index].type = TOK_KW;
            }
            //printf("%ls\n", tokens->tokens[token_index].value);
#ifdef HX_DEBUG
            if(isKeyword(tokens->tokens[token_index].value)) {
                log(L"已创建一个新的关键字Token");
            } else {
                log(L"已创建一个新的标识符Token");
            }
#endif
            token_index++;
            tokens->count++;
            index_src--;
        }
    }
    return tokens;
}

static bool isKeyword(wchar_t* str) {
    if(str == NULL) return false;
    int index = 0;
    while(keyword[index] != NULL) {
        if(wcscmp(keyword[index], str) == 0) return true;
        index++;
    }
    return false;
}
static bool isOperator(wchar_t ch) {
    return (ch==L'+' || ch==L'-' || ch==L'*' || ch==L',' ||ch==L'，' ||
            ch==L'/' || ch==L'=' || ch==L'\"' || ch==L'.' ||
            ch==L'\'' || ch==L'“' || ch==L'‘' || ch==L'’' ||
            ch==L'”' || ch==L'|' || ch==L'&' || ch==L'^' ||
            ch==L'%' || ch==L'!' || ch==L'！' || ch==L'{' ||
            ch==L'}' || ch==L'(' || ch == L')' || ch==L'（' || ch==L'）' ||
            ch==L';' || ch==L'；' || ch==L'。' || ch==L'[' ||
            ch==L'【' || ch==L']' || ch==L'】' || ch == L':'||
            ch == L'：');
}
void freeTokens(Tokens** tokens) {
#ifdef HX_DEBUG
    log(L"释放tokens");
#endif
    if(tokens==NULL||*tokens == NULL) return;
    if((*tokens)->tokens) {
        for(int i = 0; i < (*tokens)->size; i++) {
            if((*tokens)->tokens[i].value) {
                free((*tokens)->tokens[i].value);
                (*tokens)->tokens[i].value = NULL;
            }
        }
        free((*tokens)->tokens);
        (*tokens)->tokens = NULL;
    }
    free((*tokens));
    *tokens = NULL;
    return;
}
#ifdef HX_DEBUG
void showTokens(Tokens* tokens) {
    log(L"tokens的图形化展现：");
    if(tokens==NULL) return;
    if(tokens->tokens) {
        fwprintf(logStream, L"\33[33m\t");
        for(int i = 0; i < (tokens)->count; i++) {
            switch(tokens->tokens[i].type) {
            case TOK_ID: {
                fwprintf(logStream, L"[ID]");
                break;
            }
            case TOK_END: {
                fwprintf(logStream, L"[END]");
                break;
            }
            case TOK_KW: {
                fwprintf(logStream, L"[KW]");
                break;
            }
            case TOK_OPR: {
                fwprintf(logStream, L"[OPR]");
                break;
            }
            case TOK_VAL: {
                fwprintf(logStream, L"[VAL]");
                break;
            }
            }
            fwprintf(logStream, L"(\"%ls\")", tokens->tokens[i].value? tokens->tokens[i].value:L"(NULL)\0");
            if(tokens->tokens[i].type == TOK_END) {
                fwprintf(logStream, L"\n\t");
            } else {
                fwprintf(logStream, L" ");
            }
        }
        fwprintf(logStream, L"\33[0m\n");
    } else log(L"(NULL)\n");
    return;
}
#endif
#undef MEM_FAIL
#undef ERR
#endif