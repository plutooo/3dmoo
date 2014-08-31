#include <stdio.h>
#include <stdlib.h>

#include "../inc/util.h"
#include "../inc/arm11.h"
#include "../inc/loader.h"

#define ASSERT(expr, ...)                                \
    if(!(expr)) {                                        \
        fprintf(stderr, "%s:%d ", __FILE__, __LINE__);   \
        fprintf(stderr, __VA_ARGS__);                    \
        exit(1);                                         \
    }
    
char *codepath = NULL; //???

int main() {
    return 0;


    arm11_Init();

    FILE* fd = fopen("tests/arm_instr.elf", "rb");
    if(fd == NULL) {
        fprintf(stderr, "Failed to open file.\n");
        return 1;
    }

    // Load file.
    if(loader_LoadFile(fd) != 0) {
        fprintf(stderr, "Failed to load file.\n");
        fclose(fd);
        return 1;
    }

    arm11_Step();
    ASSERT(arm11_R(0) == 0x31, "mov1 fail\n");

    arm11_Step();
    ASSERT(arm11_R(1) == 0x80000001, "mov2 fail\n");

    arm11_Step();
    arm11_Step();
    ASSERT(arm11_R(3) == 0x33, "eor1 fail\n");

    arm11_Step();
    ASSERT(arm11_R(4) == 0x40000031, "eor2 fail\n");

    arm11_Step();
    ASSERT(arm11_R(5) == 0xC0000031, "eor3 fail\n");

    arm11_Step();
    ASSERT(arm11_R(6) == 0xC0000031, "eor4 fail\n");

    arm11_Step();
    ASSERT(arm11_R(7) == 0x100, "ldr1 fail\n");

    arm11_Step();
    ASSERT(arm11_R(8) == 0x123, "ldr2 fail\n");

    arm11_Step();
    arm11_Step();
    ASSERT(arm11_R(2) == 0x1, "add pc1 fail\n");

    arm11_Step();
    ASSERT(arm11_R(2) == 0x0, "add pc2 fail\n");

    return 0;
}
