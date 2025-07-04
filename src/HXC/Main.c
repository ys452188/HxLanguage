#include <stdio.h>
#include "lexer.h"
#include "scanner.h"
#include "compiler.h"'
int main(int argc,char** argv) {
    printf("\033[32m");
    wchar_t* src = getData("main.hxl");
    if(src == NULL) {
        return 255;
    }
    TokenStream t = getToken(src);
    free(src);
    ObjectCode obj;
    compile(&t,&obj);
    cleanupToken(&t);
    return 0;
}