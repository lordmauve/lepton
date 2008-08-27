/* Vector and color support
 *
 * $Id$ 
 */

#include <Python.h>
#include <math.h>

#ifndef _VECTOR_H_
#define _VECTOR_H_

/* 3D vector */
typedef struct {
	float x; float y; float z;
	float _pad; /* for 16-byte alignment so we're SIMD-friendly*/
} Vec3;

/* RGBA Color */
typedef struct {
	float r; float g; float b; float a;
} Color;


/* vector math routines 
 *
 * Use these instead of coding arithmetic by hand to allow for
 * optimized versions later on.
*/

/* Fast inverse sqrt */
inline float InvSqrt (float x);

#define Vec3_dot(v0, v1) ((v0)->x*(v1)->x + (v0)->y*(v1)->y + (v0)->z*(v1)->z)

#define Vec3_add(result, v0, v1) \
	(result)->x = (v0)->x + (v1)->x; \
	(result)->y = (v0)->y + (v1)->y; \
	(result)->z = (v0)->z + (v1)->z

#define Vec3_addi(result, v) \
	(result)->x += (v)->x; \
	(result)->y += (v)->y; \
	(result)->z += (v)->z

#define Vec3_sub(result, v0, v1) \
	(result)->x = (v0)->x - (v1)->x; \
	(result)->y = (v0)->y - (v1)->y; \
	(result)->z = (v0)->z - (v1)->z

#define Vec3_subi(result, v) \
	(result)->x -= (v)->x; \
	(result)->y -= (v)->y; \
	(result)->z -= (v)->z

#define Vec3_mul(result, v0, v1) \
	(result)->x = (v0)->x * (v1)->x; \
	(result)->y = (v0)->y * (v1)->y; \
	(result)->z = (v0)->z * (v1)->z

#define Vec3_muli(result, v) \
	(result)->x *= (v)->x; \
	(result)->y *= (v)->y; \
	(result)->z *= (v)->z

#define Vec3_scalar_mul(result, v0, s) \
	(result)->x = (v0)->x * s; \
	(result)->y = (v0)->y * s; \
	(result)->z = (v0)->z * s

#define Vec3_scalar_muli(result, s) \
	(result)->x *= s; \
	(result)->y *= s; \
	(result)->z *= s

#define Vec3_div(result, v0, v1) \
	(result)->x = (v0)->x / (v1)->x; \
	(result)->y = (v0)->y / (v1)->y; \
	(result)->z = (v0)->z / (v1)->z

#define Vec3_scalar_div(result, v, s) \
	(result)->x = (v)->x * (1.0f / s); \
	(result)->y = (v)->y * (1.0f / s); \
	(result)->z = (v)->z * (1.0f / s)

#define Vec3_neg(result, v) \
	(result)->x = -(v)->x; \
	(result)->y = -(v)->y; \
	(result)->z = -(v)->z

#define Vec3_cross(result, v0, v1) \
	(result)->x = ((v0)->y * (v1)->z) - ((v0)->z * (v1)->y) \
	(result)->y = ((v0)->z * (v1)->x) - ((v0)->x * (v1)->z) \
	(result)->x = ((v0)->x * (v1)->y) - ((v0)->y * (v1)->x)

#define Vec3_len_sq(v) ((v)->x*(v)->x + (v)->y*(v)->y + (v)->z*(v)->z)

inline float Vec3_len(Vec3 *v);

#define Vec3_normalize(result, v) \
	Vec3_scalar_mul(result, v, InvSqrt(Vec3_len_sq(v)))

inline void Vec3_lerp(Vec3 *result, float t, const Vec3 *v0, const Vec3 *v1);

/* Populate a vector from a Python sequence of 3 numbers */
inline int Vec3_FromSequence(Vec3 *dest, PyObject *sequence);

inline void Vec3_copy(Vec3 * __restrict__ dest, Vec3 * __restrict__ src); 

inline void Vec3_closest_pt_to_line(Vec3 * __restrict__ dest, Vec3 * __restrict__ pt, Vec3 * __restrict__ lstart, Vec3 * __restrict__ lend);

inline void Color_copy(Color * __restrict__ dest, Color * __restrict__ src); 

/* Populate a color from a Python sequence of 3 or 4 numbers */
inline int Color_FromSequence(Color *dest, PyObject *sequence);

#endif
