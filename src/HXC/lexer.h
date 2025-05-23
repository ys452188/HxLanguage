#ifndef LEXER_H
#define LEXER_H
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <stdlib.h>
#include <stdbool.h>
typedef enum {
    TOKEN_KEYWORD,       /*关键字*/
    TOKEN_VALUE,          /*数据*/
    TOKEN_INDENTIFIER,    /*标识符*/
    TOKEN_OPERATOR,      /*运算符号*/
} TokenType;
typedef struct Token {
    TokenType type;    /*这个Token的类型,用于标识这个Token大概是什么*/
    wchar_t* value;    /*具体代码*/
    size_t length;
} Token;
typedef struct TokenStream {
    Token* tokens;
    long int size;
} TokenStream;
const wchar_t* keywords_2[] = {L"if\0",NULL};
const wchar_t* keywords_3[] = {L"var\0",L"con\0",L"int\0",L"for\0",L"fun\0",NULL};
const wchar_t* keywords_4[] = {L"char\0",L"bool\0",L"true\0",L"else\0",L"goto\0",L"case\0",L"void\0",NULL};
const wchar_t* keywords_5[] = {L"float\0",L"wchar\0",L"false\0",L"break\0",L"class\0",L"using\0",NULL};
const wchar_t* keywords_6[] = {L"double\0",L"sizeOf\0",L"switch\0",L"return\0",NULL};
const wchar_t* keywords_7[] = {L"default\0",NULL};
const wchar_t* keywords_8[] = {L"continue\0",NULL};
bool isKeyword(const wchar_t*);
bool isOperator(const wchar_t);
void cleanupToken(TokenStream*);
TokenStream getToken(const wchar_t*);
TokenStream getToken(const wchar_t* src) {
    TokenStream tokenStream = {0};
    if(src==NULL) return tokenStream;
    tokenStream.size = 1;
    tokenStream.tokens = (Token*)malloc(sizeof(Token));
    if(!tokenStream.tokens) {
        fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
        tokenStream.size = 0;
        return tokenStream;
    }
    int index = 0;
    wchar_t* p = src;
    while(*p!=L'\0') {
        if(iswspace(*p)) {
            p++;
            continue;
        }
        if(*p==L'#') {
            while(*p!=L'\0'&&*p!=L'\n') p++;
            continue;
        }
        if(index >= tokenStream.size) {
            tokenStream.size+=1;
            void* temp = realloc(tokenStream.tokens,tokenStream.size*sizeof(Token));
            if(!temp) {
                fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
                cleanupToken(&tokenStream);
                return tokenStream;
            }
            tokenStream.tokens = (Token*)temp;
        }
        Token newToken = {0};
        if(iswdigit(*p)) {  //数字
            wchar_t* p_end = p;
            while((*p_end!=L'\0')&&(!isOperator(*p_end))) {
                p_end++;
            }
            newToken.length = p_end-p;
            newToken.value = (wchar_t*)malloc((newToken.length+1)*sizeof(wchar_t));
            if(!newToken.value) {
                fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
                cleanupToken(&tokenStream);
                return tokenStream;
            }
            wcsncpy(newToken.value,p,newToken.length);
            newToken.value[newToken.length] = L'\0';
            newToken.type = TOKEN_VALUE;
            p = p_end;
            tokenStream.tokens[index] = newToken;
            index++;
            continue;
        } else if(*p == L'\"'||*p==L'“' ||*p==L'”') {   //字符串
            wchar_t* p_end = p;
            while(*p_end!=L'\0') {
                p_end++;
                if(*p_end == L'\"'||*p_end ==L'“' ||*p_end ==L'”'&&*(p_end-1)!=L'\\') {
                    break;
                }
            }
            if (*p_end != *p || (p_end > p+1 && *(p_end-1) == L'\\')) {
                fprintf(stderr,"\033[31m[E]词法错误：字符串没有结尾！\033[0m\n");
                cleanupToken(&tokenStream);
                return tokenStream;
            }
            newToken.length = p_end-p+1;
            newToken.value = (wchar_t*)malloc((newToken.length + 1) * sizeof(wchar_t));
            if(!newToken.value) {
                fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
                cleanupToken(&tokenStream);
                return tokenStream;
            }
            wcsncpy(newToken.value,p,newToken.length);
            newToken.value[newToken.length] = L'\0';
            newToken.type = TOKEN_VALUE;
            tokenStream.tokens[index] = newToken;
            index++;
            p = p_end+1;
            continue;
        } else if (isOperator(*p)) {  //operator
            // 预检查双字符运算符
            int op_length = 1;
            if (p[1] != L'\0') {
                // 定义支持的双字符运算符
                if ((*p == L'<' && p[1] == L'=') ||  // <=
                        (*p == L'>' && p[1] == L'=') ||  // >=
                        (*p == L'=' && p[1] == L'=') ||  // ==
                        ((*p == L'!'||*p == L'！') && p[1] == L'=') || //!=
                        (*p == L'+' && p[1] == L'+') ||   //++
                        (*p == L'|' && p[1] == L'|') ||   //||
                        (*p == L'&' && p[1] == L'&') ||   //&&
                        (*p == L'-' && p[1] == L'>') ||   //->
                        (*p == L'-' && p[1] == L'-')) {   //--
                    op_length = 2;
                }
            }
            // 创建运算符Token
            newToken.length = op_length;
            newToken.value = (wchar_t*)malloc((op_length + 1) * sizeof(wchar_t));
            if(!newToken.value) {
                fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
                cleanupToken(&tokenStream);
                return tokenStream;
            }
            wcsncpy(newToken.value, p, op_length);
            newToken.value[op_length] = L'\0';
            newToken.type = TOKEN_OPERATOR;
            // 添加到Token流
            tokenStream.tokens[index] = newToken;
            index++;
            // 移动指针（根据运算符长度）
            p += op_length;
            continue;
        } else {   //关键字和标识符
            wchar_t* p_end = p;
            while(*p_end!=L'\0') {
                p_end++;
                if(iswspace(*p_end)||isOperator(*p_end)||*p_end==L'\0') break;
            }
            newToken.length = p_end-p;
            newToken.value = (wchar_t*)malloc((newToken.length+1)*sizeof(wchar_t));
            if(!newToken.value) {
                fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
                cleanupToken(&tokenStream);
                return tokenStream;
            }
            wcsncpy(newToken.value,p,newToken.length);
            newToken.value[newToken.length] = L'\0';
            if(isKeyword(newToken.value)) {
                newToken.type = TOKEN_KEYWORD;
            } else {
                newToken.type = TOKEN_INDENTIFIER;
            }
            p = p_end;
            tokenStream.tokens[index] = newToken;
            index++;
            continue;
        }

    }
    return tokenStream;
}
bool isKeyword(const wchar_t* s) {
    if(s==NULL) return false;
    int len = wcslen(s);
    switch(len)
    {
    case 2:
        for(int i = 0; keywords_2[i]!=NULL; i++) {
            if(wcscmp(s,keywords_2[i])==0) {
                return true;
            }
        }
        return false;
        break;
    case 3:
        for(int i = 0; keywords_3[i]!=NULL; i++) {
            if(wcscmp(s,keywords_3[i])==0) {
                return true;
            }
        }
        return false;
        break;
    case 4:
        for(int i = 0; keywords_4[i]!=NULL; i++) {
            if(wcscmp(s,keywords_4[i])==0) {
                return true;
            }
        }
        return false;
        break;
    case 5:
        for(int i = 0; keywords_5[i]!=NULL; i++) {
            if(wcscmp(s,keywords_5[i])==0) {
                return true;
            }
        }
        return false;
        break;
    case 6:
        for(int i = 0; keywords_6[i]!=NULL; i++) {
            if(wcscmp(s,keywords_6[i])==0) {
                return true;
            }
        }
        return false;
        break;
    case 7:
        for(int i = 0; keywords_7[i]!=NULL; i++) {
            if(wcscmp(s,keywords_7[i])==0) {
                return true;
            }
        }
        return false;
        break;
    case 8:
        for(int i = 0; keywords_8[i]!=NULL; i++) {
            if(wcscmp(s,keywords_8[i])==0) {
                return true;
            }
        }
        return false;
        break;
    default:
        return false;
        break;
    }
    return false;
}
bool isOperator(const wchar_t ch) {
    return(ch==L','||ch==L'。'||ch==L';'||ch==L'；'||ch==L'\"'||ch==L'“'||ch==L'”'||ch==L'\''||ch==L'‘'||ch==L'’'
           ||ch==L'*'||ch==L'+'||ch==L'-'||ch==L'/'||ch==L'\\'||ch==L'|'||ch==L'^'||ch==L'<'||ch==L'>'||ch==L'%'||ch==L'&'
           ||ch==L')'||ch==L'('||ch==L'{'||ch==L'}'||ch==L'!'||ch==L'！'||ch==L'?' || ch == L'？' || ch == L'.'||ch==L':'||ch==L'：'||
           ch==L'='||ch == L'[' || ch == L']' || ch == L'（'|| ch == L'）'|| ch == L'［'|| ch == L'］') ;
}
void cleanupToken(TokenStream* ts) {
    if (!ts || !ts->tokens) return;
    // 遍历实际生成的Token数量（不是预分配的大小）
    for (int i = 0; i < ts->size; i++) {
        if (ts->tokens[i].value) {


            printf("Token%d {\n   value=\"%ls\"\n",i,ts->tokens[i].value);
            printf("   type=%d\n}\n---------------------\n",ts->tokens[i].type);


            free(ts->tokens[i].value);
            ts->tokens[i].value = NULL; // 避免悬空指针
        }
    }
    free(ts->tokens);
    ts->tokens = NULL;
    ts->size = 0;
}
#endif