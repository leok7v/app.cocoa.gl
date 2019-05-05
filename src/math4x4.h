#pragma once
#include "std.h"

BEGIN_C

typedef float vec3f_t[3];
typedef float vec4f_t[3];
typedef float mat4x4f_t[16];

#define IDENTITY_MATRIX_4x4F { \
    1, 0, 0, 0, \
    0, 1, 0, 0, \
    0, 0, 1, 0, \
    0, 0, 0, 1  }

extern const mat4x4f_t identity_4x4f;

void multiply_4x4f(mat4x4f_t r, const mat4x4f_t a, const mat4x4f_t b);

END_C
