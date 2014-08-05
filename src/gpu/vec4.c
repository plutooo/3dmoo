#include "vec4.h"

struct vec4 _vec4(scalar x, scalar y, scalar z, scalar w)
{
struct vec4 v;
v.x = x;
v.y = y;
v.z = z;
v.w = w;
return v;
}

struct vec4 vec4_zero()
{
struct vec4 v;
v.x = 0;
v.y = 0;
v.z = 0;
v.w = 0;
return v;
}

struct vec4 vec4_add(struct vec4 v, struct vec4 u)
{
struct vec4 t;
t.x = v.x + u.x;
t.y = v.y + u.y;
t.z = v.z + u.z;
t.w = v.w + u.w;
return t;
}

struct vec4 vec4_sub(struct vec4 v, struct vec4 u)
{
struct vec4 t;
t.x = v.x - u.x;
t.y = v.y - u.y;
t.z = v.z - u.z;
t.w = v.w - u.w;
return t;
}

struct vec4 vec4_mul(struct vec4 v, struct vec4 u)
{
struct vec4 t;
t.x = v.x * u.x;
t.y = v.y * u.y;
t.z = v.z * u.z;
t.w = v.w * u.w;
return t;
}

struct vec4 vec4_muls(scalar s, struct vec4 v)
{
struct vec4 t;
t.x = s * v.x;
t.y = s * v.y;
t.z = s * v.z;
t.w = s * v.w;
return t;
}

struct vec4 vec4_set(int num, scalar val, struct vec4 v)
{
    switch (num)
    {
    case 0:
        v.x = val;
        break;
    case 1:
        v.y = val;
        break;
    case 2:
        v.z = val;
        break;
    case 3:
        v.w = val;
        break;
    default:
        //DEBUG("vec4_set out of range");
        break;

    }
    return v;
}

float vec4_get(int num, struct vec4 v)
{
    switch (num)
    {
    case 0:
        return v.x;
    case 1:
        return v.y;
    case 2:
        return v.z;
    case 3:
        return v.w;
    default:
        //DEBUG("vec4_get out of range");
        return 0;

    }
}
float *vec4_getp(int num, struct vec4 v)
{
    switch (num)
    {
    case 0:
        return &v.x;
    case 1:
        return &v.y;
    case 2:
        return &v.z;
    case 3:
        return &v.w;
    default:
        //DEBUG("vec4_get out of range");
        return (float*)0;

    }
}