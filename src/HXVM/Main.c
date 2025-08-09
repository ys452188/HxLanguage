#include <stdio.h>
#include "HXVM.h"
#include "hxLocale.h"
int main(int argc, char** argv) {
    initLocale();
    loadObjectFile("test.hxe");
    freeObjectCode(&hsmCode);
    printf("%d\n", hsmCode.obj_fun_size);
    printf("end.");
    return 0;
}