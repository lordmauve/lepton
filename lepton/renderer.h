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
/* Renderer shared code
 *
 * $Id$ 
 */

#ifndef _RENDERER_H_
#define _RENDERER_H_

typedef struct {
	PyObject_HEAD
	Py_ssize_t size;
	float *data;
} FloatArrayObject;

/* Return true if o is a bon-a-fide FloatArrayObject */
int FloatArrayObject_Check(FloatArrayObject *o);

FloatArrayObject *
FloatArray_new(Py_ssize_t size);

FloatArrayObject *
generate_default_2D_tex_coords(GroupObject *pgroup);

#endif
