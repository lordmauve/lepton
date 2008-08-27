/****************************************************************************
*
* Copyright (c) 2008 by Casey Duncan and contributors
* All Rights Reserved.
*
* This software is subject to the provisions of the MIT License
* A copy of the license should accompany this distribution.
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*
****************************************************************************/
/* Vector and color support
 *
 * $Id$ 
 */

#include "vector.h"

typedef union {
	int i;
	float f;
} floatint;

/* The illustrious fast inverse sqrt of Quake 3 fame */
inline float InvSqrt (float x) {
    float xhalf = 0.5f*x;
	int i = ((floatint)x).i;
    i = 0x5f3759df - (i>>1);
	x = ((floatint)i).f;
    x = x*(1.5f - xhalf*x*x);
    return x;
}

inline float Vec3_len(Vec3 *v) {
	float len;
	len = Vec3_len_sq(v);
	return len ? (1.0f / InvSqrt(len)) : 0.0f;
}

inline void Vec3_lerp(Vec3 *result, float t, const Vec3 *v0, const Vec3 *v1)
{
    Vec3 tmp0, tmp1;
	Vec3_sub(&tmp0, v1, v0);
	Vec3_scalar_mul(&tmp1, &tmp0, t);
	Vec3_add(result, v0, &tmp1);
}

/* Populate a vector from a Python sequence of 3 numbers 
 * Return true on success, false on failure with exception set
 */
inline int Vec3_FromSequence(Vec3 *dest, PyObject *sequence)
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

inline int Color_FromSequence(Color *dest, PyObject *sequence)
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

inline void Vec3_copy(Vec3 * __restrict__ dest, Vec3 * __restrict__ src)
{
	dest->x = src->x;
	dest->y = src->y;
	dest->z = src->z;
}

#define EPSILON 0.00001f

/* Return the closest point along a line segment to the given point */
inline void Vec3_closest_pt_to_line(Vec3 * __restrict__ dest, Vec3 * __restrict__ pt, 
	Vec3 * __restrict__ lstart, Vec3 * __restrict__ lend)
{
	Vec3 tp, lv;

	Vec3_sub(&lv, lend, lstart);
	Vec3_sub(&tp, pt, lstart);
	const float mag2 = Vec3_len_sq(&lv);
	if (mag2 > EPSILON) {
		float t = Vec3_dot(&tp, &lv) / mag2;
		t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;
		Vec3_scalar_muli(&lv, t);
		Vec3_add(dest, lstart, &lv);
	} else {
		/* zero-length line */
		Vec3_copy(dest, lstart);
	}
}

/*
static inline void vmathV3Slerp( VmathVector3 *result, float t, const VmathVector3 *unitVec0, const VmathVector3 *unitVec1 )
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

inline void Color_copy(Color * __restrict__ dest, Color * __restrict__ src) 
{
	dest->r = src->r;
	dest->g = src->g;
	dest->b = src->b;
	dest->a = src->a;
}


