#ifndef VEC4_H
#define VEC4_H

typedef float scalar;

struct vec4 {
scalar x, y, z, w;
};

struct vec4 _vec4(scalar, scalar, scalar, scalar);
struct vec4 vec4_zero();
struct vec4 vec4_add(struct vec4, struct vec4);
struct vec4 vec4_sub(struct vec4, struct vec4);
struct vec4 vec4_mul(struct vec4, struct vec4);
struct vec4 vec4_muls(scalar, struct vec4);

#endif