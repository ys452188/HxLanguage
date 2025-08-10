#ifndef HXLANG_LEXER_H
#define HXLANG_LEXER_H
#include <wchar.h>
#include <wctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include "output.h"
typedef wchar_t wchar;
typedef enum LexerErrorType {
    ERR_WSTR_NOT_END,
    ERR_STR_NOT_END,
    ERR_WC_NOT_END,
    ERR_CH_NOT_END,
} LexerErrorType;
typedef enum TokenType {
    TOK_ID,
    TOK_OP,
    TOK_KW,
    TOK_VAL,
} TokenType;
typedef struct Token {
    wchar* value;
    TokenType type;
#define STR 0
#define CH 1
    int mark;  //标记,用于区分宽窄字符(串)
    int lin;
} Token;

wchar_t* keyword[] = {L"var",L"定义变量",L"con",L"定义常量",L"fun",
                      L"定义函数",L"if", L"如果",L"else",L"否则",L"void",L"无参数",
                      L"无返回值",L"pub",L"公有成员",L"pri",L"私有成员",L"pro",L"受保护成员",
                      L"using",L"使用头文件",L"它的父类是",L"parent",L"定义类",L"class",
                      L"type", L"它的类型是",NULL
                     };
wchar* src = NULL;     //源代码
Token* tokens = NULL;
int tokenSize = 0;
int getNextToken_index = 0;
Token* tokensPtr = NULL;   //指向tokens的指针
int lin = 1;     //行

int getNextToken(void) {
    if(getNextToken_index >= tokenSize-1) {
        //printf("out");
        return 1;
    }
    if(tokens[getNextToken_index].value == NULL) return 1;
    tokensPtr = &(tokens[getNextToken_index]);
    getNextToken_index++;
    return 0;
}
bool isOperator(const wchar ch);
void lexerError(LexerErrorType err, int lin);
int wcsequ(wchar* str1, wchar* str2);
int isKeyword(wchar_t* value);
int setSrc(wchar* str);
void freeSrc(void);
void freeTokens(void);
int setTokens(void) {
    if(!src) return -1;
    tokenSize = 1;
    tokens = (Token*)calloc(1, sizeof(Token));
    int tokenIndex = 0;
    if(!tokens) return -1;
    wchar* p = src;
    while(*p != L'\0') {
        if(*p == L'\n') lin++;
        if(tokenIndex >= tokenSize) {
            tokenSize  = tokenIndex + 1;
            void* temp = realloc(tokens, tokenSize*sizeof(Token));
            if(!temp) return -1;
            tokens = (Token*)temp;
            tokens[tokenIndex].value = NULL;
            tokens[tokenIndex].lin = lin;
        }
        if(*p == L'#') {   //处理注释
            while(*p != L'\0') {
                if(*p == L'\n') {
                    break;
                } else {
                    p++;
                }
            }
        }
        if(iswdigit(*p)) {           //数字
            wchar* start = p;
            while(*p != L'\0') {
                p++;
                if(isOperator(*p) || iswspace(*p)) break;
            }
            int valueLength = p - start;
            tokens[tokenIndex].value = (wchar*)calloc(valueLength + 1, sizeof(wchar));
            if(!(tokens[tokenIndex].value)) return -1;
            wcsncpy(tokens[tokenIndex].value,start,valueLength);
            tokens[tokenIndex].type = TOK_VAL;
            tokens[tokenIndex].lin = lin;
            //printf("%ls\n",tokens[tokenIndex].value);
            tokenIndex++;
        } else if(*p == L'\"' || *p == L'“' ||*p == L'”') {    //字符串
            p++;
            wchar* start = p;
            while(*p != L'\0') {
                if(*p == L'\n') lin++;
                if((*p == L'\"' || *p == L'“' || *p == L'”') && ((*(p-1) == L'\\' && *(p-2) == L'\\') || *(p - 1) != L'\\')) break;
                p++;
            }
            if(*p != L'\"' && *p != L'“' && *p != L'”') {
                lexerError(ERR_STR_NOT_END, lin);
                return 255;
            }
            int length = p - start;
            tokens[tokenIndex].value = (wchar*)calloc(length + 1, sizeof(wchar));
            if(!(tokens[tokenIndex].value)) return -1;
            wcsncpy(tokens[tokenIndex].value,start,length);
            tokens[tokenIndex].type = TOK_VAL;
            tokens[tokenIndex].mark = STR;
            tokens[tokenIndex].lin = lin;
            tokenIndex++;
            p++;
        } else if(*p == L'\'' || *p == L'’' || *p == L'‘') {     //字符
            p++;
            wchar* start = p;
            while(*p != L'\0') {
                if(*p == L'\n') lin++;
                if((*p == L'\'' || *p==L'‘' || *p == L'’') && ((*(p-1) == L'\\' && *(p-2) == L'\\') || *(p-1) != L'\\')) break;
                p++;
            }
            if(*p != L'\'' && *p != L'‘' && *p != L'’') {
                lexerError(ERR_CH_NOT_END, lin);
                return 255;
            }
            int length = p - start;
            tokens[tokenIndex].value = (wchar*)calloc(length + 1, sizeof(wchar));
            if(!(tokens[tokenIndex].value)) return -1;
            wcsncpy(tokens[tokenIndex].value,start,length);
            tokens[tokenIndex].type = TOK_VAL;
            tokens[tokenIndex].mark = CH;
            tokens[tokenIndex].lin = lin;
            tokenIndex++;
            p++;
        } else if(isOperator(*p)) {        //运算符
            int length = 1;
            wchar* start = p;
            if((*p == L'=' && *(p+1) == L'=') //==
                    || (*p == L'>' && *(p+1) == L'=') //>=
                    || (*p == L'<' && *(p+1) == L'=') //<=
                    || (*p == L'+' && *(p+1) == L'+') //++
                    || (*p == L'-' && *(p+1) == L'-') //--
                    || (*p == L'>' && *(p+1) == L'>') //>>
                    || (*p == L'<' && *(p+1) == L'<') //<<
                    || (*p == L'-' && *(p+1) == L'>') //->
              ) {
                length = 2;
                p++;
                p++;
            } else {
                length = 1;
                p++;
            }
            tokens[tokenIndex].value = (wchar*)calloc(length + 1, sizeof(wchar));
            if(!(tokens[tokenIndex].value)) return -1;
            wcsncpy(tokens[tokenIndex].value,start,length);
            tokens[tokenIndex].type = TOK_OP;
            tokens[tokenIndex].mark = -1;
            tokens[tokenIndex].lin = lin;
            tokenIndex++;
        } else if(iswspace(*p)) {
            p++;
        } else {  //关键字或标识符
            wchar* start = p;
            while(*p != L'\0') {
                p++;
                if(iswspace(*p) || isOperator(*p)) break;
            }
            int length = p - start;
            tokens[tokenIndex].value = (wchar*)calloc(length + 1, sizeof(wchar));
            if(!(tokens[tokenIndex].value)) return -1;
            wcsncpy(tokens[tokenIndex].value,start,length);
            if(isKeyword(tokens[tokenIndex].value)) {
                tokens[tokenIndex].type = TOK_KW;
            } else {
                tokens[tokenIndex].type = TOK_ID;
            }
            tokens[tokenIndex].mark = -1;
            tokens[tokenIndex].lin = lin;
            tokenIndex++;
        }
    }
    if(tokenIndex == tokenSize) {
        tokenSize++;
        void* temp = realloc(tokens, tokenSize*sizeof(Token));
        if(!temp) return -1;
        tokens = (Token*)temp;
    }
    return 0;
}
int setSrc(wchar* str) {
    src = (wchar*)calloc(wcslen(str) + 1, sizeof(wchar));
    if(!src) return -1;
    wcscpy(src, str);
    return 0;
}
int wcsequ(wchar* str1, wchar* str2) {
    if(!str1 || !str2) return 0;
    int temp1 = 0, temp2 = 0;
    temp1 = wcslen(str1);
    temp2 = wcslen(str2);
    if(temp1 != temp2) return 0;
    int len = temp1;
    for(int i = 0; i < len; i++) {
        if(str1[i] != str2[i]) return 0;
    }
    return 1;
}
int isKeyword(wchar_t* value) {
    int i = 0;
    while(keyword[i] != NULL) {
        if(wcsequ(keyword[i],value) == 1) return 1;
        i++;
    }
    return 0;
}
bool isOperator(const wchar ch) {
    return(ch==L','||ch==L'。'||ch==L';'||ch==L'；'||ch==L'\"'||ch==L'“'||ch==L'”'||ch==L'\''||ch==L'‘'||ch==L'’'
           ||ch==L'*'||ch==L'+'||ch==L'-'||ch==L'/'||ch==L'\\'||ch==L'|'||ch==L'^'||ch==L'<'||ch==L'>'||ch==L'%'||ch==L'&'
           ||ch==L')'||ch==L'('||ch==L'{'||ch==L'}'||ch==L'!'||ch==L'！'||ch==L'?' || ch == L'？' || ch == L'.'||ch==L':'||ch==L'：'||
           ch==L'='||ch == L'[' || ch == L']' || ch == L'（'|| ch == L'）'|| ch == L'［'|| ch == L'］') ;
}
void freeSrc(void) {
    if(src) {
        free(src);
        src = NULL;
    }
    return;
}
void lexerError(LexerErrorType err, int lin) {
    initLocale();
    switch(err) {
    case ERR_WSTR_NOT_END: {
        fwprintf(stderr, L"\33[31m[E]宽字符串缺少结尾！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_STR_NOT_END: {
        fwprintf(stderr, L"\33[31m[E]字符串缺少结尾！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_WC_NOT_END: {
        fwprintf(stderr, L"\33[31m[E]宽字符缺少结尾！(位于第%d行)\33[0m\n", lin);
    }
    break;

    case ERR_CH_NOT_END: {
        fwprintf(stderr, L"\33[31m[E]字符缺少结尾！(位于第%d行)\33[0m\n", lin);
    }
    break;

    }
    return;
}
void freeTokens(void) {
    if(!tokens) return;
    for(int i = 0; i < tokenSize-1; i++) {
        if(tokens[i].value) {
            //printf("%ls\n",tokens[i].value);
            free(tokens[i].value);
        }
    }
    free(tokens);
    return;
}
#endif