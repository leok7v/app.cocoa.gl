#pragma once
/*
 * Tomas Moller and Ben Trumbore.
 * Fast, minimum storage ray-triangle intersection.
 * Journal of graphics tools, 2(1):21-28, 1997.
 */

/*
 "Algorithm for determining whether a ray intersects a triangle.
  The algorithm translates the origin of the ray and then changes
  the base to yield a vector (t u v)^T, where t is the distance to
  the plane in which the triangle lies and (u,v) represents the
  [barycentric?] coordinates inside the triangle."
  https://en.wikipedia.org/wiki/Barycentric_coordinate_system
*/

int intersect_triangle(double orig[3], double dir[3],
                       double vert0[3], double vert1[3], double vert2[3],
                       double *t, double *u, double *v);
