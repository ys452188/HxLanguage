#include <stdio.h>
#include "lexer.h"
#include "scanner.h"
int main(int argc,char** argv) {
    printf("\033[32m");
    wchar_t* src = getData("main.hxl");
    if(src == NULL) {
        return 255;
    }
    TokenStream t = getToken(src);
    printf("INFO:\nt.size=%ld\n",t.size);
    printf("sizeof(t) = %ld\n",sizeof(t));
    printf("t.address = %p\n**********************\n",&t);
    cleanupToken(&t);
    free(src);
    printf("**********************\n");
    printf("t.address=%p\n",&t);
    printf("sizeof(t) = %ld\n",sizeof(t));
    printf("t.size = %ld\n",t.size);
    printf("\033[0m");
    return 0;
}