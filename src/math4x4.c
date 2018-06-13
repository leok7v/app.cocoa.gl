#include "math4x4.h"
#include <math.h>
#include <memory.h>

BEGIN_C

const float identity_4x4f[16] = IDENTITY_MATRIX_4x4F;

// there is hundreds of good NEON and SSE optimizations of multiply_4x4f when it is needed

void multiply_4x4f(mat4x4f_t r, const mat4x4f_t a, const mat4x4f_t b) {
    for (int i = 0; i <= 12; i += 4) {
        for (int j = 0; j < 4; j++) {
            r[i + j] = b[i] * a[j] + b[i + 1] * a[j + 4] + b[i + 2] * a[j + 8] + b[i + 3] * a[j + 12];
        }
    }
}

END_C
