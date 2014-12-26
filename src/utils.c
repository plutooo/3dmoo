#include "util.h"

void f24to32(u32 data, void *dataa)
{
    u32 sign = data & 0x800000;
    int exp = (int)((data & 0x7F0000) >> 16);
    u32 fraction = (data & 0xFFFF) << (23 - 16);

    u32 out = 0;
    out |= (sign != 0) ? 0x80000000 : 0;

    if((data & ~0x800000) == 0)
    {
        exp = 0;
    }
    else
    {
        exp = exp - 63 + 127;
    }

    // Make sure faction doesn't overflow
    out |= fraction & 0x007FFFFF;
    out |= ((u32)exp & 0xFF) << 23;

    if(dataa != NULL)
        *(u32*)dataa = out;
}
