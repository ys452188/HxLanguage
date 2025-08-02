#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "vm.h"
int main(int argc, char** argv) {
    HXVM vm;
    initHXVM(&vm);
    closeHXVM(&vm);
    return 0;
}