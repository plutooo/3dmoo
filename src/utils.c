#include "util.h"

float f24to32(u32 data, void *dataa) //this is not 100% correct fix it
{
    u32 dataout = (data << 8) &0x80000000; //Sign bit
    dataout |= (data << 7)&~0xFF800000;
    u32 exponent = (data >> 16) & 0x7F;
    if (exponent != 0) {
        exponent += 0x40;
    }
    dataout |= exponent << 23;
    *(u32*)dataa = dataout;
    return *(float*)&dataout;
}
