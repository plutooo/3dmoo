#ifndef MAT4_H
#define MAT4_H

#include "vec4.h"

struct mat4 {
	struct vec4 a, b, c, d;
};

struct mat4 mat4_identity();
struct mat4 mat4_mul(struct mat4, struct mat4); 
struct vec4 mat4_mulv(struct mat4, struct vec4);

struct mat4 mat4_aangle(struct vec4, scalar);

#endif