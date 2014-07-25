
#include "util.h"

short ftofix16(float num) {

    short i, f;

    if (fabs(num) > 2047.999f) {
        printf("Error: number out of range (num=%f)\n", num);
    }

    i = (short)num;
    f = (short)(fabs(num * 16)) & 15;
    return (i << 4) | f;
}

float fix16tof(int n)
{
    float s = 1.0f;
    if (n < 0) {
        s = -1.0f;
        n = -n;
    }
    return s * ((float)(n >> 4) + ((n & 15) / 16.0f));
}
