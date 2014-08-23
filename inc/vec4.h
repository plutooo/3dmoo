#ifndef VEC4_H
#define VEC4_H

typedef float scalar;

struct vec4 {
    scalar v[4];//x, y, z, w;
};
struct vec3 {
    scalar v[3];//x, y, z;
};
struct vec2 {
    scalar v[2];//u, v;
};

struct vec4 _vec4(scalar, scalar, scalar, scalar);
struct vec4 vec4_zero();
struct vec4 vec4_add(struct vec4, struct vec4);
struct vec4 vec4_sub(struct vec4, struct vec4);
struct vec4 vec4_mul(struct vec4, struct vec4);
struct vec4 vec4_muls(scalar, struct vec4);

struct vec4 vec4_set(int num, scalar val, struct vec4 v);
float vec4_get(int num, struct vec4 v);
float *vec4_getp(int num, struct vec4 v);


#endif