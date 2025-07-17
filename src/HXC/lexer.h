#ifndef LEXER_H
#define LEXER_H
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <locale.h>
#include <wchar.h>
// 释放指针ptr指向的内存空间
void hxFree(void** ptr) {
    // 如果ptr为空，则直接返回
    if(*ptr == NULL) return;
    //调试输出
    printf("freeing %p\n",*ptr);
    // 释放ptr指向的内存空间
    free(*ptr);
    *ptr = NULL;
    return;
}
wchar_t* charToWchar(const char* narrow_str) {
    setlocale(LC_ALL, ""); // 使用系统默认locale
    size_t required_size = mbstowcs(NULL, narrow_str, 0) + 1;
    if(required_size == (size_t)-1) {
        // 转换失败（无效的多字节序列）
        return NULL;
    }
    wchar_t* wide_str = (wchar_t*)malloc(required_size * sizeof(wchar_t));
    if(!wide_str) {
        return NULL;
    }
    if(mbstowcs(wide_str, narrow_str, required_size) == (size_t)-1) {
        free(wide_str);
        return NULL;
    }
    return wide_str;
}
typedef enum {
    TOKEN_KEYWORD=0,       /*关键字*/
    TOKEN_VALUE,          /*数据*/
    TOKEN_INDENTIFIER,    /*标识符*/
    TOKEN_OPERATOR,      /*运算符号*/
} TokenType;
typedef struct Token {
    TokenType type;    /*这个Token的类型,用于标识这个Token大概是什么*/
    wchar_t* value;    /*具体代码*/
    size_t length;
    int lin;
    int col;
} Token;
typedef struct TokenStream {
    Token* tokens;
    long int size;
} TokenStream;
typedef struct LexerStatus {
    int lin;     //行
    int col;    //列
} LexerStatus;
LexerStatus lexerStatus;       //词法分析器状态
const wchar_t* keywords_2[] = {L"if\0",L"如果\0",L"整型\0",L"否则\0",NULL};
const wchar_t* keywords_3[] = {L"var\0",L"浮点型\0",L"字符型\0",L"布尔型",L"con\0",L"int\0",L"for\0",L"fun\0",NULL};
const wchar_t* keywords_4[] = {L"char\0",L"定义变量\0",L"定义常量\0",L"定义函数\0",L"bool\0",L"true\0",L"else\0",L"goto\0",L"case\0",L"void\0",NULL};
const wchar_t* keywords_5[] = {L"精确浮点型",L"float\0",L"wchar\0",L"false\0",L"break\0",L"class\0",L"using\0",NULL};
const wchar_t* keywords_6[] = {L"double\0",L"switch\0",L"return\0",NULL};
const wchar_t* keywords_7[] = {L"default\0",NULL};
const wchar_t* keywords_8[] = {L"continue\0",NULL};
bool isKeyword(const wchar_t*);
bool isOperator(const wchar_t);
void cleanupToken(TokenStream*);
TokenStream getToken(const wchar_t*);
TokenStream getToken(const wchar_t* src) {
#ifdef _WIN32
    setlocale(LC_ALL,"zh_CN.UTF-8");
#endif
    lexerStatus.lin=1;
    TokenStream tokenStream = {0};
    if(src==NULL) return tokenStream;
    tokenStream.size = 1;
    tokenStream.tokens = (Token*)malloc(sizeof(Token));
    if(!tokenStream.tokens) {
#ifndef _WIN32
        fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
        fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
        tokenStream.size = 0;
        return tokenStream;
    }
    int index = 0;
    wchar_t* p = src;
    while(*p!=L'\0') {
        if(*p==L'\n') {
            lexerStatus.lin++;
            lexerStatus.col = 0;
        }
        if(iswspace(*p)) {
            p++;
            lexerStatus.col++;
            continue;
        }
        if(*p==L'#') {              //处理注释
            while(*p!=L'\0'&&*p!=L'\n') p++;
            continue;
        }
        if(index >= tokenStream.size) {
            tokenStream.size+=1;
            void* temp = realloc(tokenStream.tokens,tokenStream.size*sizeof(Token));
            if(!temp) {
#ifndef _WIN32
                fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
                fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
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
#ifndef _WIN32
                fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
                fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
                cleanupToken(&tokenStream);
                return tokenStream;
            }
            wcsncpy(newToken.value,p,newToken.length);
            newToken.value[newToken.length] = L'\0';
            newToken.type = TOKEN_VALUE;
            newToken.lin = lexerStatus.lin;
            newToken.col = lexerStatus.col;
            p = p_end;
            tokenStream.tokens[index] = newToken;
            index++;
            continue;
        } else if((*p == L'L') && (*(p+1) == L'\"'||*(p+1)==L'“' ||*(p+1)==L'”')) {    //宽字符串
            const wchar_t start_quote = *(p+1); // 记录开始引号类型
            const wchar_t* p_end = p+2; // 跳过 L 和引号

            // 查找结束引号
            int escape = 0;
            while(*p_end != L'\0') {
                if(!escape && *p_end == start_quote) {
                    break; // 找到匹配的结束引号
                }
                escape = (*p_end == L'\\') ? 1 : 0; // 处理转义字符
                p_end++;
            }

            // 检查字符串是否正常结束
            if(*p_end != start_quote) {
#ifndef _WIN32
                fprintf(stderr, "\033[31m[E]词法错误：宽字符串没有结尾！(位于第%d行,%d列)\033[0m\n",lexerStatus.lin,lexerStatus.col);
#else
                fwprintf(stderr, L"\033[31m[E]词法错误：宽字符串没有结尾！(位于第%d行,%d列)\033[0m\n",lexerStatus.lin,lexerStatus.col);
#endif
                cleanupToken(&tokenStream);
                return tokenStream;
            }

            // 创建Token (包含L前缀和引号)
            newToken.length = p_end - p + 1;
            newToken.value = malloc((newToken.length + 1) * sizeof(wchar_t));
            wcsncpy(newToken.value, p, newToken.length);
            newToken.value[newToken.length] = L'\0';
            newToken.type = TOKEN_VALUE;
            newToken.lin = lexerStatus.lin;
            newToken.col = lexerStatus.col;
            tokenStream.tokens[index++] = newToken;
            p = p_end + 1; // 移动到结束引号之后
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
#ifndef _WIN32
                fprintf(stderr, "\033[31m[E]词法错误：字符串没有结尾！(位于第%d行,%d列)\033[0m\n",lexerStatus.lin,lexerStatus.col);
#else
                fwprintf(stderr, L"\033[31m[E]词法错误：字符串没有结尾！(位于第%d行,%d列)\033[0m\n",lexerStatus.lin,lexerStatus.col);
#endif
                cleanupToken(&tokenStream);
                return tokenStream;
            }
            newToken.length = p_end-p+1;
            newToken.value = (wchar_t*)malloc((newToken.length + 1) * sizeof(wchar_t));
            if(!newToken.value) {
#ifndef _WIN32
                fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
                fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
                cleanupToken(&tokenStream);
                return tokenStream;
            }
            wcsncpy(newToken.value,p,newToken.length);
            newToken.value[newToken.length] = L'\0';
            newToken.type = TOKEN_VALUE;
            newToken.lin = lexerStatus.lin;
            newToken.col = lexerStatus.col;
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
#ifndef _WIN32
                fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
                fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
                cleanupToken(&tokenStream);
                return tokenStream;
            }
            wcsncpy(newToken.value, p, op_length);
            newToken.value[op_length] = L'\0';
            newToken.type = TOKEN_OPERATOR;
            newToken.lin = lexerStatus.lin;
            newToken.col = lexerStatus.col;
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
#ifndef _WIN32
                fprintf(stderr,"\033[31m[E]内存分配失败！\033[0m\n");
#else
                fwprintf(stderr,L"\033[31m[E]内存分配失败！\033[0m\n");
#endif
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
            newToken.lin = lexerStatus.lin;
            newToken.col = lexerStatus.col;
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

            //printf("Token%d {\n\tvalue= %ls\n\ttype=%d\n}\n",i,ts->tokens[i].value,ts->tokens[i].type);

            hxFree(&(ts->tokens[i].value));
            ts->tokens[i].value = NULL; // 避免悬空指针
        }
    }
    free(ts->tokens);
    ts->tokens = NULL;
    ts->size = 0;
}
#endif