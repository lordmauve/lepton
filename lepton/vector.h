/* Vector and color support
 *
 * $Id$ 
 */

#include <Python.h>
#include <math.h>

#ifndef _VECTOR_H_
#define _VECTOR_H_

#ifdef _MSC_VER
#define inline
#define __restrict__
#endif

/* 3D vector */
typedef struct {
	float x; float y; float z;
	float _pad; /* for 16-byte alignment so we're SIMD-friendly*/
} Vec3;

/* RGBA Color */
typedef struct {
	float r; float g; float b; float a;
} Color;

/* General constants and useful math routines */

#define EPSILON 0.00001f

/* The illustrious fast inverse sqrt of Quake 3 fame 
 * This algorithm is very fast, but at the cost of accuracy
 */
static inline float InvSqrt (float x) {
    float xhalf = 0.5f*x;
    int i = *(int*)&x;
    i = 0x5f3759df - (i>>1);
    x = *(float*)&i;
    x = x*(1.5f - xhalf*x*x);
    return x;
}

#define clamp(n, min, max) \
	((n) < (min) ? (min) : ((n) > (max) ? (max) : (n)))

/* vector math routines 
 *
 * Use these instead of coding arithmetic by hand to allow for
 * optimized versions later on.
*/

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
	(result)->x = ((v0)->y * (v1)->z) - ((v0)->z * (v1)->y); \
	(result)->y = ((v0)->z * (v1)->x) - ((v0)->x * (v1)->z); \
	(result)->z = ((v0)->x * (v1)->y) - ((v0)->y * (v1)->x)

#define Vec3_len_sq(v) ((v)->x*(v)->x + (v)->y*(v)->y + (v)->z*(v)->z)

/* Fast vector normalize at the expense of accuracy */
#define Vec3_normalize_fast(result, v) \
	Vec3_scalar_mul(result, v, InvSqrt(Vec3_len_sq(v)))

/* Slower but more precise normalize */
static inline void Vec3_normalize(Vec3 *result, Vec3 *v)
{
	float len;
	len = Vec3_len_sq(v);
	if (len > EPSILON) {
		len = 1.0f / sqrtf(len);
		Vec3_scalar_mul(result, v, len);
	} else {
		/* Maybe we should just clamp to zero here
		   But I erred on the side of less data loss
		   Callers concerned about this should pre or post-check
		   the vector length.
		*/
		result->x = v->x;
		result->y = v->y;
		result->z = v->z;
	}
}

/* Populate a color from a Python sequence of 3 or 4 numbers */
static inline int Color_FromSequence(Color *dest, PyObject *sequence);

static inline float Vec3_len(Vec3 *v) 
{
	float len;
	len = Vec3_len_sq(v);
	return len ? (1.0f / InvSqrt(len)) : 0.0f;
}

static inline void Vec3_lerp(Vec3 *result, float t, const Vec3 *v0, const Vec3 *v1)
{
    Vec3 tmp0, tmp1;
	Vec3_sub(&tmp0, v1, v0);
	Vec3_scalar_mul(&tmp1, &tmp0, t);
	Vec3_add(result, v0, &tmp1);
}

/* Populate a vector from a Python sequence of 3 numbers 
 * Return true on success, false on failure with exception set
 */
static inline int Vec3_FromSequence(Vec3 *dest, PyObject *sequence)
{
	PyObject *tuple;
	int result;

	tuple = PySequence_Tuple(sequence);
	if (tuple == NULL)
		return 0;
	result = PyArg_ParseTuple(tuple, "fff;expected 3 floats for vector",
		&dest->x, &dest->y, &dest->z);
	Py_DECREF(tuple);
	return result;
}

static inline int Color_FromSequence(Color *dest, PyObject *sequence)
{
	PyObject *tuple;
	int result;

	tuple = PySequence_Tuple(sequence);
	if (tuple == NULL)
		return 0;
	dest->a = 1.0f;
	result = PyArg_ParseTuple(tuple, "fff|f;expected 3 or 4 floats for color",
		&dest->r, &dest->g, &dest->b, &dest->a);
	Py_DECREF(tuple);
	return result;
}

static inline void Vec3_copy(Vec3 * __restrict__ dest, Vec3 * __restrict__ src)
{
	dest->x = src->x;
	dest->y = src->y;
	dest->z = src->z;
}


/* Return the closest point along a line segment to the given point */
static inline void Vec3_closest_pt_to_line(Vec3 * __restrict__ dest, Vec3 * __restrict__ pt, 
	Vec3 * __restrict__ lstart, Vec3 * __restrict__ lend)
{
	Vec3 tp, lv;
	float mag2;

	Vec3_sub(&lv, lend, lstart);
	Vec3_sub(&tp, pt, lstart);
	mag2 = Vec3_len_sq(&lv);
	if (mag2 > EPSILON) {
		float t = Vec3_dot(&tp, &lv) / mag2;
		t = clamp(t, 0.0f, 1.0f);
		Vec3_scalar_muli(&lv, t);
		Vec3_add(dest, lstart, &lv);
	} else {
		/* zero-length line */
		Vec3_copy(dest, lstart);
	}
}

/*
static static inline void vmathV3Slerp( VmathVector3 *result, float t, const VmathVector3 *unitVec0, const VmathVector3 *unitVec1 )
{
    VmathVector3 tmpV3_0, tmpV3_1;
    float recipSinAngle, scale0, scale1, cosAngle, angle;
    cosAngle = vmathV3Dot( unitVec0, unitVec1 );
    if ( cosAngle < _VECTORMATH_SLERP_TOL ) {
        angle = acosf( cosAngle );
        recipSinAngle = ( 1.0f / sinf( angle ) );
        scale0 = ( sinf( ( ( 1.0f - t ) * angle ) ) * recipSinAngle );
        scale1 = ( sinf( ( t * angle ) ) * recipSinAngle );
    } else {
        scale0 = ( 1.0f - t );
        scale1 = t;
    }
    vmathV3ScalarMul( &tmpV3_0, unitVec0, scale0 );
    vmathV3ScalarMul( &tmpV3_1, unitVec1, scale1 );
    vmathV3Add( result, &tmpV3_0, &tmpV3_1 );
}
*/

static inline void Color_copy(Color * __restrict__ dest, Color * __restrict__ src) 
{
	dest->r = src->r;
	dest->g = src->g;
	dest->b = src->b;
	dest->a = src->a;
}


#endif
