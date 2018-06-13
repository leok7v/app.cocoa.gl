/*
 * Tomas Moller and Ben Trumbore.
 * Fast, minimum storage ray-triangle intersection.
 * Journal of graphics tools, 2(1):21-28, 1997.
 * https://en.wikipedia.org/wiki/Möller–Trumbore_intersection_algorithm
 * http://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/code/
 */
#include "intersect_triangle.h"

#define EPSILON 0.000001

#define CROSS(dest, v1, v2) {               \
   dest[0] = v1[1] * v2[2] - v1[2] * v2[1]; \
   dest[1] = v1[2] * v2[0] - v1[0] * v2[2]; \
   dest[2] = v1[0] * v2[1] - v1[1] * v2[0]; \
}

#define DOT(v1,v2) (v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2])

#define SUB(dest, v1, v2) { \
   dest[0] = v1[0] - v2[0]; \
   dest[1] = v1[1] - v2[1]; \
   dest[2] = v1[2] - v2[2]; \
}

#ifndef TEST_CULL /* define TEST_CULL if culling is desired */

int intersect_triangle(double orig[3], double dir[3],
                       double vert0[3], double vert1[3], double vert2[3],
                       double *t, double *u, double *v) {
    double edge1[3], edge2[3], tvec[3], pvec[3], qvec[3];
    double det,inv_det;
    SUB(edge1, vert1, vert0); /* find vectors for two edges sharing vert0 */
    SUB(edge2, vert2, vert0);
    CROSS(pvec, dir, edge2); /* begin calculating determinant - also used to calculate U parameter */
    det = DOT(edge1, pvec); /* if determinant is near zero, ray lies in plane of triangle */
    if (det > -EPSILON && det < EPSILON) {
        return 0;
    }
    inv_det = 1.0 / det;
    SUB(tvec, orig, vert0); /* calculate distance from vert0 to ray origin */
    *u = DOT(tvec, pvec) * inv_det; /* calculate U parameter and test bounds */
    if (*u < 0.0 || *u > 1.0) {
        return 0;
    }
    CROSS(qvec, tvec, edge1); /* prepare to test V parameter */
    *v = DOT(dir, qvec) * inv_det; /* calculate V parameter and test bounds */
    if (*v < 0.0 || *u + *v > 1.0) {
        return 0;
    }
    *t = DOT(edge2, qvec) * inv_det; /* calculate t, ray intersects triangle */
    return 1;
}

#else /* the culling branch */

int intersect_triangle(double orig[3], double dir[3],
                       double vert0[3], double vert1[3], double vert2[3],
                       double *t, double *u, double *v) {
    double edge1[3], edge2[3], tvec[3], pvec[3], qvec[3];
    double det,inv_det;
    SUB(edge1, vert1, vert0); /* find vectors for two edges sharing vert0 */
    SUB(edge2, vert2, vert0);
    CROSS(pvec, dir, edge2); /* begin calculating determinant - also used to calculate U parameter */
    det = DOT(edge1, pvec); /* if determinant is near zero, ray lies in plane of triangle */
    if (det < EPSILON) {
        return 0;
    }
    SUB(tvec, orig, vert0); /* calculate distance from vert0 to ray origin */
    *u = DOT(tvec, pvec); /* calculate U parameter and test bounds */
    if (*u < 0.0 || *u > det) {
        return 0;
    }
    CROSS(qvec, tvec, edge1); /* prepare to test V parameter */
    *v = DOT(dir, qvec); /* calculate V parameter and test bounds */
    if (*v < 0.0 || *u + *v > det) {
        return 0;
    }
    *t = DOT(edge2, qvec); /* calculate t, scale parameters, ray intersects triangle */
    inv_det = 1.0 / det;
    *t *= inv_det;
    *u *= inv_det;
    *v *= inv_det;
    return 1;
}

/*
 "One advantage of this method is that the plane equation need not be computed on the fly
 nor be stored, which can amount to significant memory savings for triangle meshes.
 As we found our method to be comparable in speed to previous methods,
 we believe it is the fastest ray-triangle intersection routine for triangles
 that do not have precomputed plane equations." Tomas Moller and Ben Trumbore 1997
 */

#endif
