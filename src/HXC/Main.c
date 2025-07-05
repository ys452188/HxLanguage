#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "scanner.h"
#include "compiler.h"
int main(void) {
    wchar_t* src = getData("main.hxl");
    if(src == NULL) {
        return 255;
    }
    TokenStream t = getToken(src);
    free(src);
    ObjectCode obj;
    int err = compile(&t,&obj);
    printf("%d\n",err);
    cleanupToken(&t);
    freeObjectCode(&obj);
    return 0;
}