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

