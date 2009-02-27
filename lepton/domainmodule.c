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
/* Native-code domains
 *
 * $Id$ */

#include <Python.h>
#include <structmember.h>
#include <math.h>
#include <float.h>
#include "vector.h"
#include "fastrng.h"
#include "group.h"

/* Base domain methods and helper functions */

static void
Domain_dealloc(PyObject *self) 
{
	PyObject_Del(self);
}

static int
Domain_never_contains(PyObject *self, PyObject *o) 
{
	return 0;
}

static PyObject *NO_INTERSECTION = NULL;

static PyObject *
Domain_never_intersects(PyObject *self, PyObject *args) 
{
	Py_INCREF(NO_INTERSECTION);
	return NO_INTERSECTION;
}

/* Return a 2-tuple of 3-vector tuples from 2 input Vec3s */
static inline PyObject *
pack_vectors(Vec3 *pt, Vec3 *norm)
{
	return Py_BuildValue("((fff)(fff))",
		pt->x, pt->y, pt->z, norm->x, norm->y, norm->z);
}

/* --------------------------------------------------------------------- */

static PyTypeObject LineDomain_Type;

typedef struct {
	PyObject_HEAD
	Vec3 start_point;
	Vec3 end_point;
} LineDomainObject;

static PyObject * start_point_str;
static PyObject * end_point_str;

static int
LineDomain_init(LineDomainObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, "(fff)(fff):__init__",
		&self->start_point.x, &self->start_point.y, &self->start_point.z,
		&self->end_point.x, &self->end_point.y, &self->end_point.z))
		return -1;
	return 0;
}

static PyObject *
LineDomain_generate(LineDomainObject *self) 
{
	float d;
	PyObject *x, *y, *z, *pt;
	Vec3 direction;

	Vec3_sub(&direction, &self->end_point, &self->start_point);
	d = rand_uni();
	x = PyFloat_FromDouble(self->start_point.x + direction.x * d);
	y = PyFloat_FromDouble(self->start_point.y + direction.y * d);
	z = PyFloat_FromDouble(self->start_point.z + direction.z * d);
	if (x == NULL || y == NULL || z == NULL) {
		Py_XDECREF(x);
		Py_XDECREF(y);
		Py_XDECREF(z);
		return NULL;
	}

	pt = PyTuple_Pack(3, x, y, z);
	Py_DECREF(x);
	Py_DECREF(y);
	Py_DECREF(z);
	return pt;
}

static PyMethodDef LineDomain_methods[] = {
	{"generate", (PyCFunction)LineDomain_generate, METH_NOARGS,
		PyDoc_STR("generate() -> Vector\n"
			"Return a random point along the line segment domain")},
	{"intersect", (PyCFunction)Domain_never_intersects, METH_VARARGS,
		PyDoc_STR("intersect() -> point, normal\n"
			"You cannot intersect a line segment")},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
LineDomain_getattr(LineDomainObject *self, PyObject *name_str)
{
	if (name_str == start_point_str) {
		return (PyObject *)Vector_new((PyObject *)self, &self->start_point, 3);	
	} else if (name_str == end_point_str) {
		return (PyObject *)Vector_new((PyObject *)self, &self->end_point, 3);	
	} else {
		return Py_FindMethod(LineDomain_methods, 
			(PyObject *)self, PyString_AS_STRING(name_str));
	}
}

static int
LineDomain_setattr(LineDomainObject *self, PyObject *name_str, PyObject *v)
{
	Vec3 *point;
	int result;

	if (name_str == start_point_str) {
		point = &self->start_point;
	} else if (name_str == end_point_str) {
		point = &self->end_point;
	} else {
		return -1;
	}
	v = PySequence_Tuple(v);
	if (v == NULL)
		return -1;
	result = PyArg_ParseTuple(v, "fff;3 floats expected",
		&point->x, &point->y, &point->z) - 1;
	Py_DECREF(v);
	return result;
}

static PySequenceMethods LineDomain_as_sequence = {
	0,		/* sq_length */
	0,		/* sq_concat */
	0,		/* sq_repeat */
	0,	    /* sq_item */
	0,		/* sq_slice */
	0,		/* sq_ass_item */
	0,	    /* sq_ass_slice */
	(objobjproc)Domain_never_contains,	/* sq_contains */
};

PyDoc_STRVAR(LineDomain__doc__, 
	"line segment domain\n\n"
	"Line(start_point, endpoint)\n\n"
	"Define the line segment domain between the specified start and\n"
	"end points (as a sequence of 3 numbers)");

static PyTypeObject LineDomain_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"domain.Line",		/*tp_name*/
	sizeof(LineDomainObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)Domain_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	&LineDomain_as_sequence, /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	(getattrofunc)LineDomain_getattr, /*tp_getattro*/
	(setattrofunc)LineDomain_setattr, /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	LineDomain__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	LineDomain_methods,  /*tp_methods*/
	0,                      /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)LineDomain_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject PlaneDomain_Type;

typedef struct {
	PyObject_HEAD
	Vec3 point;
	Vec3 normal;
	float d;
} PlaneDomainObject;

static PyObject * point_str;
static PyObject * normal_str;

static int
PlaneDomain_init(PlaneDomainObject *self, PyObject *args)
{
	double len;

	if (!PyArg_ParseTuple(args, "(fff)(fff):__init__",
		&self->point.x, &self->point.y, &self->point.z,
		&self->normal.x, &self->normal.y, &self->normal.z))
		return -1;
	
	len = Vec3_len_sq(&self->normal);
	if (len != 1.0f) {
		if (len > EPSILON) {
			Vec3_normalize(&self->normal, &self->normal);
		} else {
			PyErr_SetString(PyExc_ValueError, 
				"PlaneDomain: zero-length normal vector");
			return -1;
		}
	}
	self->d = Vec3_dot(&self->point, &self->normal);
	return 0;
}

static PyObject *
PlaneDomain_generate(PlaneDomainObject *self) 
{
	PyObject *x, *y, *z, *pt;

	x = PyFloat_FromDouble(self->point.x);
	y = PyFloat_FromDouble(self->point.y);
	z = PyFloat_FromDouble(self->point.z);
	if (x == NULL || y == NULL || z == NULL) {
		Py_XDECREF(x);
		Py_XDECREF(y);
		Py_XDECREF(z);
		return NULL;
	}
	
	pt = PyTuple_Pack(3, x, y, z);
	Py_DECREF(x);
	Py_DECREF(y);
	Py_DECREF(z);
	return pt;
}

static PyObject *
PlaneDomain_intersect(PlaneDomainObject *self, PyObject *args) 
{
	Vec3 norm, start, end, vec;
	float ndotv, t, dist;

	if (!PyArg_ParseTuple(args, "(fff)(fff):intersect",
		&start.x, &start.y, &start.z,
		&end.x, &end.y, &end.z))
		return NULL;
	
	Vec3_copy(&norm, &self->normal);
	Vec3_sub(&vec, &end, &start);
	ndotv = Vec3_dot(&norm, &vec);
	if (ndotv) {
		t = (self->d - norm.x*start.x - norm.y*start.y - norm.z*start.z) / ndotv;
		if (t >= 0.0f && t <= 1.0f) {
			/* calculate intersection point */
			Vec3_scalar_muli(&vec, t);
			Vec3_add(&end, &start, &vec);
			/* Calculate the distance from the plane to the start point */
			dist = Vec3_dot(&norm, &vec);
			if (dist > 0.0f) {
				/* start point is on opposite side of normal */
				Vec3_neg(&norm, &norm);
			}
			return pack_vectors(&end, &norm);
		}
	}
	Py_INCREF(NO_INTERSECTION);
	return NO_INTERSECTION;
}

static PyMethodDef PlaneDomain_methods[] = {
	{"generate", (PyCFunction)PlaneDomain_generate, METH_NOARGS,
		PyDoc_STR("generate() -> Vector\n"
			"Aways return the provided point on the plane")},
	{"intersect", (PyCFunction)PlaneDomain_intersect, METH_VARARGS,
		PyDoc_STR("intersect() -> point, normal\n"
			"Intersect the line segment with the plane return the intersection\n"
			"point and normal vector pointing into space on the same side of the\n"
			"plane as the start point.\n\n"
			"If the line does not intersect, or lies completely in the plane\n"
			"return (None, None)")},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
PlaneDomain_getattr(PlaneDomainObject *self, PyObject *name_str)
{
	if (name_str == point_str) {
		return (PyObject *)Vector_new((PyObject *)self, &self->point, 3);	
	} else if (name_str == normal_str) {
		return (PyObject *)Vector_new((PyObject *)self, &self->normal, 3);	
	} else {
		return Py_FindMethod(PlaneDomain_methods, 
			(PyObject *)self, PyString_AS_STRING(name_str));
	}
}

static int
PlaneDomain_setattr(PlaneDomainObject *self, PyObject *name_str, PyObject *v)
{
	Vec3 *point;
	int result;

	if (name_str == point_str) {
		point = &self->point;
	} else if (name_str == normal_str) {
		point = &self->normal;
	} else {
		return -1;
	}
	v = PySequence_Tuple(v);
	if (v == NULL)
		return -1;
	result = PyArg_ParseTuple(v, "fff;3 floats expected",
		&point->x, &point->y, &point->z) - 1;
	Py_DECREF(v);
	return result;
}

static int
PlaneDomain_contains(PlaneDomainObject *self, PyObject *pt)
{
	Vec3 point, from_plane;

	pt = PySequence_Tuple(pt);
	if (pt == NULL)
		return -1;
	if (!PyArg_ParseTuple(pt, "fff:__contains__", &point.x, &point.y, &point.z)) {
		Py_DECREF(pt);
		return -1;
	}
	Py_DECREF(pt);

	Vec3_sub(&from_plane, &point, &self->point);
	return Vec3_dot(&from_plane, &self->normal) < EPSILON;
}


static PySequenceMethods PlaneDomain_as_sequence = {
	0,		/* sq_length */
	0,		/* sq_concat */
	0,		/* sq_repeat */
	0,	    /* sq_item */
	0,		/* sq_slice */
	0,		/* sq_ass_item */
	0,	    /* sq_ass_slice */
	(objobjproc)PlaneDomain_contains,	/* sq_contains */
};

PyDoc_STRVAR(PlaneDomain__doc__, 
	"Infinite planar domain\n\n"
	"Plane(point, normal, half_space=False)\n\n"
	"point -- Any point in the plane (3-number sequence)\n"
	"normal -- Normal vector perpendicular to the plane. This need not\n"
	"be a unit vector. The half-space contained by the plane is opposite\n"
	"the direction of the normal.");

static PyTypeObject PlaneDomain_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"domain.Plane",		/*tp_name*/
	sizeof(PlaneDomainObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)Domain_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	&PlaneDomain_as_sequence, /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	(getattrofunc)PlaneDomain_getattr, /*tp_getattro*/
	(setattrofunc)PlaneDomain_setattr, /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	PlaneDomain__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	PlaneDomain_methods,  /*tp_methods*/
	0,                      /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)PlaneDomain_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject AABoxDomain_Type;

typedef struct {
	PyObject_HEAD
	Vec3 min;
	Vec3 max;
} AABoxDomainObject;

static PyObject * max_point_str;
static PyObject * min_point_str;

#define min(a, b) ((a) <= (b) ? (a) : (b))
#define max(a, b) ((a) >= (b) ? (a) : (b))

static int
AABoxDomain_init(AABoxDomainObject *self, PyObject *args)
{
	float x1, y1, z1, x2, y2, z2;

	if (!PyArg_ParseTuple(args, "(fff)(fff):__init__",
		&x1, &y1, &z1, &x2, &y2, &z2))
		return -1;
	
	self->min.x = min(x1, x2);
	self->min.y = min(y1, y2);
	self->min.z = min(z1, z2);
	self->max.x = max(x1, x2);
	self->max.y = max(y1, y2);
	self->max.z = max(z1, z2);
	return 0;
}

static PyObject *
AABoxDomain_generate(AABoxDomainObject *self) 
{
	PyObject *x, *y, *z, *pt;
	Vec3 size;

	Vec3_sub(&size, &self->max, &self->min);

	x = PyFloat_FromDouble(self->min.x + size.x * rand_uni());
	y = PyFloat_FromDouble(self->min.y + size.y * rand_uni());
	z = PyFloat_FromDouble(self->min.z + size.z * rand_uni());
	if (x == NULL || y == NULL || z == NULL) {
		Py_XDECREF(x);
		Py_XDECREF(y);
		Py_XDECREF(z);
		return NULL;
	}
	
	pt = PyTuple_Pack(3, x, y, z);
	Py_DECREF(x);
	Py_DECREF(y);
	Py_DECREF(z);
	return pt;
}

#define pt_in_box(box, px, py, pz) \
	(((px) >= (box)->min.x) & ((px) <= (box)->max.x) \
	 & ((py) >= (box)->min.y) & ((py) <= (box)->max.y) \
	 & ((pz) >= (box)->min.z) & ((pz) <= (box)->max.z))

static int
AABoxDomain_contains(AABoxDomainObject *self, PyObject *pt)
{
	float x, y, z;

	pt = PySequence_Tuple(pt);
	if (pt == NULL)
		return -1;
	if (!PyArg_ParseTuple(pt, "fff:__contains__", &x, &y, &z)) {
		Py_DECREF(pt);
		return -1;
	}
	Py_DECREF(pt);

	return pt_in_box(self, x, y, z);
}
	
static PyObject *
AABoxDomain_intersect(AABoxDomainObject *self, PyObject *args) 
{
	Vec3 start, end;
	float t, ix, iy, iz;
	int start_in, end_in;
	char* buf;

	if (!PyArg_ParseTuple(args, "(fff)(fff):intersect",
		&start.x, &start.y, &start.z,
		&end.x, &end.y, &end.z))
		return NULL;

	start_in = pt_in_box(self, start.x, start.y, start.z);
	end_in = pt_in_box(self, end.x, end.y, end.z);
	if (!(start_in | end_in)) {
		/* both points outside, check for grazing intersection */
		Vec3 center;
		center.x = self->min.x + (self->max.x - self->min.x) * 0.5f;
		center.y = self->min.y + (self->max.y - self->min.y) * 0.5f;
		center.z = self->min.z + (self->max.z - self->min.z) * 0.5f;
		Vec3_closest_pt_to_line(&end, &center, &start, &end);
		end_in = pt_in_box(self, end.x, end.y, end.z);
	}

	if (start_in == end_in) {
		Py_INCREF(NO_INTERSECTION);
		return NO_INTERSECTION;
	}

	/* top face */
	if ((start.y > self->max.y) | (end.y > self->max.y)) {
		t = (self->max.y - start.y) / (end.y - start.y);
		ix = (end.x - start.x) * t + start.x;
		iy = self->max.y;
		iz = (end.z - start.z) * t + start.z;
		// printf("top (%f, %f, %f) (%f, %f, %f)\n", start.x, start.y, start.z, ix, iy, iz);
		if (pt_in_box(self, ix, iy, iz))
			return Py_BuildValue("((fff)(fff))", 
				ix, iy, iz, 0.0, (start.y > self->max.y) ? 1.0 : -1.0, 0.0);
	}
	/* right face */
	if ((start.x > self->max.x) | (end.x > self->max.x)) {
		t = (self->max.x - start.x) / (end.x - start.x);
		ix = self->max.x;
		iy = (end.y - start.y) * t + start.y;
		iz = (end.z - start.z) * t + start.z;
		// printf("right (%f, %f, %f) (%f, %f, %f)\n", start.x, start.y, start.z, ix, iy, iz);
		if (pt_in_box(self, ix, iy, iz))
			return Py_BuildValue("((fff)(fff))", 
				ix, iy, iz, (start.x > self->max.x) ? 1.0 : -1.0, 0.0, 0.0);
	}
	/* bottom face */
	if ((start.y < self->min.y) | (end.y < self->min.y)) {
		t = (self->min.y - start.y) / (end.y - start.y);
		ix = (end.x - start.x) * t + start.x;
		iy = self->min.y;
		iz = (end.z - start.z) * t + start.z;
		// printf("bottom (%f, %f, %f) (%f, %f, %f)\n", start.x, start.y, start.z, ix, iy, iz);
		if (pt_in_box(self, ix, iy, iz))
			return Py_BuildValue("((fff)(fff))", 
				ix, iy, iz, 0.0, (start.y < self->min.y) ? -1.0 : 1.0, 0.0);
	}
	/* left face */
	if ((start.x < self->min.x) | (end.x < self->min.x)) {
		t = (self->min.x - start.x) / (end.x - start.x);
		ix = self->min.x;
		iy = (end.y - start.y) * t + start.y;
		iz = (end.z - start.z) * t + start.z;
		// printf("left (%f, %f, %f) (%f, %f, %f)\n", start.x, start.y, start.z, ix, iy, iz);
		if (pt_in_box(self, ix, iy, iz))
			return Py_BuildValue("((fff)(fff))", 
				ix, iy, iz, (start.x < self->min.x) ? -1.0 : 1.0, 0.0, 0.0);
	}
	/* far face */
	if ((start.z < self->min.z) | (end.z < self->min.z)) {
		t = (self->min.z - start.z) / (end.z - start.z);
		ix = (end.x - start.x) * t + start.x;
		iy = (end.y - start.y) * t + start.y;
		iz = self->min.z;
		// printf("far (%f, %f, %f) (%f, %f, %f)\n", start.x, start.y, start.z, ix, iy, iz);
		if (pt_in_box(self, ix, iy, iz))
			return Py_BuildValue("((fff)(fff))", 
				ix, iy, iz, 0.0, 0.0, (start.z < self->min.z) ? -1.0 : 1.0);
	}
	/* near face */
	if ((start.z > self->max.z) | (end.z > self->max.z)) {
		t = (self->max.z - start.z) / (end.z - start.z);
		ix = (end.x - start.x) * t + start.x;
		iy = (end.y - start.y) * t + start.y;
		iz = self->max.z;
		// printf("near (%f, %f, %f) (%f, %f, %f)\n", start.x, start.y, start.z, ix, iy, iz);
		if (pt_in_box(self, ix, iy, iz))
			return Py_BuildValue("((fff)(fff))", 
				ix, iy, iz, 0.0, 0.0, (start.z > self->max.z) ? 1.0 : -1.0);
	}

	/* We should never get here */
	buf = PyMem_Malloc(256 * sizeof(char));
	PyOS_snprintf(buf, 256, "AABox.intersect BUG: Intersect face not identified "
		"min=(%f, %f, %f) max=(%f, %f, %f) start=(%f, %f, %f) end=(%f, %f, %f)",
		self->min.x, self->min.y, self->min.z, self->max.x, self->max.y, self->max.z,
		start.x, start.y, start.z, end.x, end.y, end.z);
	PyErr_SetString(PyExc_RuntimeError, buf);
	PyMem_Free(buf);
	return NULL;
}

static PyMethodDef AABoxDomain_methods[] = {
	{"generate", (PyCFunction)AABoxDomain_generate, METH_NOARGS,
		PyDoc_STR("generate() -> Vector\n"
			"Return a random point inside the box")},
	{"intersect", (PyCFunction)AABoxDomain_intersect, METH_VARARGS,
		PyDoc_STR("intersect() -> point, normal\n"
			"Intersect the line segment with the box return the first\n"
			"intersection point and normal vector pointing into space from\n"
			"the box side intersected.\n\n"
			"If the line does not intersect, or lies completely in one side\n"
			"of the box return (None, None)")},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
AABoxDomain_getattr(AABoxDomainObject *self, PyObject *name_str)
{
	if (name_str == min_point_str) {
		return (PyObject *)Vector_new((PyObject *)self, &self->min, 3);	
	} else if (name_str == max_point_str) {
		return (PyObject *)Vector_new((PyObject *)self, &self->max, 3);	
	} else {
		return Py_FindMethod(AABoxDomain_methods, 
			(PyObject *)self, PyString_AS_STRING(name_str));
	}
}

static int
AABoxDomain_setattr(AABoxDomainObject *self, PyObject *name_str, PyObject *v)
{
	Vec3 *point;
	int result;

	if (name_str == min_point_str) {
		point = &self->min;
	} else if (name_str == max_point_str) {
		point = &self->max;
	} else {
		return -1;
	}
	v = PySequence_Tuple(v);
	if (v == NULL)
		return -1;
	result = PyArg_ParseTuple(v, "fff;3 floats expected",
		&point->x, &point->y, &point->z) - 1;
	Py_DECREF(v);
	return result;
}

static PySequenceMethods AABoxDomain_as_sequence = {
	0,		/* sq_length */
	0,		/* sq_concat */
	0,		/* sq_repeat */
	0,	    /* sq_item */
	0,		/* sq_slice */
	0,		/* sq_ass_item */
	0,	    /* sq_ass_slice */
	(objobjproc)AABoxDomain_contains,	/* sq_contains */
};

PyDoc_STRVAR(AABoxDomain__doc__, 
	"Axis aligned rectangular prism\n\n"
	"AABox(point1, point2)\n\n"
	"point1 and point2 define any two opposite corners of the box");

static PyTypeObject AABoxDomain_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"domain.AABox",		/*tp_name*/
	sizeof(AABoxDomainObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)Domain_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	&AABoxDomain_as_sequence, /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	(getattrofunc)AABoxDomain_getattr, /*tp_getattro*/
	(setattrofunc)AABoxDomain_setattr, /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	AABoxDomain__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	AABoxDomain_methods,  /*tp_methods*/
	0,                      /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)AABoxDomain_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject SphereDomain_Type;

typedef struct {
	PyObject_HEAD
	Vec3 center;
	float outer_radius;
	float inner_radius;
} SphereDomainObject;

static PyObject * center_str;
static PyObject * radius_str;
static PyObject * outer_radius_str;
static PyObject * inner_radius_str;

static int
SphereDomain_init(SphereDomainObject *self, PyObject *args)
{
	self->inner_radius = 0;
	if (!PyArg_ParseTuple(args, "(fff)f|f:__init__",
		&self->center.x, &self->center.y, &self->center.z,
		&self->outer_radius, &self->inner_radius))
		return -1;
	
	if (self->outer_radius < self->inner_radius) {
		PyErr_SetString(PyExc_ValueError, 
			"Sphere: Expected outer_radius >= inner_radius");
		return -1;
	}

	return 0;
}

static PyObject *
SphereDomain_generate(SphereDomainObject *self) 
{
	PyObject *x, *y, *z, *pt;
	float dist, mag2;
	Vec3 point;

	/* Generate a random unit vector */
	do {
		point.x = rand_norm(0.0f, 1.0f);
		point.y = rand_norm(0.0f, 1.0f);
		point.z = rand_norm(0.0f, 1.0f);
		mag2 = Vec3_len_sq(&point);
	} while (mag2 < EPSILON);
	Vec3_normalize(&point, &point);
	
	dist = self->inner_radius + sqrtf(rand_uni()) * (
		self->outer_radius - self->inner_radius);
	Vec3_scalar_muli(&point, dist);

	x = PyFloat_FromDouble(point.x + self->center.x);
	y = PyFloat_FromDouble(point.y + self->center.y);
	z = PyFloat_FromDouble(point.z + self->center.z);
	if (x == NULL || y == NULL || z == NULL) {
		Py_XDECREF(x);
		Py_XDECREF(y);
		Py_XDECREF(z);
		return NULL;
	}
	
	pt = PyTuple_Pack(3, x, y, z);
	Py_DECREF(x);
	Py_DECREF(y);
	Py_DECREF(z);
	return pt;
}

static int
SphereDomain_contains(SphereDomainObject *self, PyObject *pt)
{
	Vec3 point, from_center;
	float dist2;

	pt = PySequence_Tuple(pt);
	if (pt == NULL)
		return -1;
	if (!PyArg_ParseTuple(pt, "fff:__contains__", 
		&point.x, &point.y, &point.z)) {
		Py_DECREF(pt);
		return -1;
	}
	Py_DECREF(pt);

	Vec3_sub(&from_center, &point, &self->center);
	dist2 = Vec3_len_sq(&from_center);
	return ((dist2 <= self->outer_radius*self->outer_radius) 
		& (dist2 >= self->inner_radius*self->inner_radius));
}
	
static PyObject *
SphereDomain_intersect(SphereDomainObject *self, PyObject *args) 
{
	Vec3 start, end, seg, vec, norm;
	float start_dist2, end_dist2, cmag2, r2, a, b, c, bb4ac, t1, t2, t;
	float inner_r2 = self->inner_radius*self->inner_radius;
	float outer_r2 = self->outer_radius*self->outer_radius;

	if (!PyArg_ParseTuple(args, "(fff)(fff):intersect",
		&start.x, &start.y, &start.z,
		&end.x, &end.y, &end.z))
		return NULL;
	
	Vec3_sub(&vec, &start, &self->center);
	start_dist2 = Vec3_len_sq(&vec);
	Vec3_sub(&vec, &end, &self->center);
	end_dist2 = Vec3_len_sq(&vec);

	r2 = ((start_dist2 > outer_r2) 
	      | ((start_dist2 > inner_r2) & (end_dist2 > inner_r2)) 
		  | (!inner_r2)) 
		? outer_r2 : inner_r2;
	if ((start_dist2 > r2) & (end_dist2 > r2)) {
		Vec3_closest_pt_to_line(&end, &self->center, &start, &end);
		// printf("closest = (%f, %f, %f)\n", end.x, end.y, end.z);
		Vec3_sub(&vec, &end, &self->center);
		end_dist2 = Vec3_len_sq(&vec);
		r2 = ((start_dist2 > outer_r2) 
			  | ((start_dist2 > inner_r2) & (end_dist2 > inner_r2)) 
			  | (!inner_r2)) 
			? outer_r2 : inner_r2;
	}

	if (((start_dist2 > outer_r2) & (end_dist2 > outer_r2))
		| ((start_dist2 <= inner_r2) & (end_dist2 <= inner_r2))
		| ((start.x == end.x) & (start.y == end.y) & (start.z == end.z))) {
		Py_INCREF(NO_INTERSECTION);
		return NO_INTERSECTION;
	}

	cmag2 = Vec3_len_sq(&self->center);
	Vec3_sub(&seg, &end, &start);
	a = Vec3_len_sq(&seg);
	b = 2.0f * ((end.x - start.x)*(start.x - self->center.x) 
		+ (end.y - start.y)*(start.y - self->center.y)
		+ (end.z - start.z)*(start.z - self->center.z));
	c = cmag2 + Vec3_len_sq(&start) - 2.0f * (
		self->center.x * start.x + self->center.y * start.y + self->center.z * start.z) - r2;
	bb4ac = b*b - 4.0f * a * c;
	// printf("r2 = %f\n", r2);
	// printf("a = %f, b = %f, c = %f, bb4ac = %f\n", a, b, c, bb4ac);
	if (fabs(bb4ac) <= EPSILON) {
		/* intersects at single point */
		t = -b / (2.0f * a);
	} else if (bb4ac >= -EPSILON) {
		/* 2-point intersection */
		bb4ac = sqrtf(bb4ac);
		t1 = (-b - bb4ac) / (2.0f * a);
		t2 = (-b + bb4ac) / (2.0f * a);
		t = ((t2 < 0.0f) | (t2 > 1.0f)) ? t1 : 
			((t1 < 0.0f) | (t1 > 1.0f)) ? t2 :
			min(t1, t2);
		// printf("t1 = %f, t2 = %f\n", t1, t2);
	} else {
		Py_INCREF(NO_INTERSECTION);
		return NO_INTERSECTION;
	}
	// printf("t = %f\n", t);
	Vec3_scalar_muli(&seg, t);
	Vec3_add(&end, &start, &seg);
	/* decide if normal points inward or outward */
	t = (start_dist2 <= r2) ? 1.0f : -1.0f;
	Vec3_sub(&vec, &self->center, &end);
	Vec3_scalar_muli(&vec, t);
	Vec3_normalize(&norm, &vec);
	return pack_vectors(&end, &norm);
}

static PyObject *
SphereDomain_closest_point_to(SphereDomainObject *self, PyObject *args) 
{
	
	Vec3 point, norm, vec;
	float dist2, inner_r2, outer_r2;
	
	/* point: input point transformed to closest point on the sphere
	   dist2: squared distance between point and line
	   vec: vector between point and center
		     then scaled to become vector between point and closest */

	if (!PyArg_ParseTuple(args, "(fff):closest_point_to",
		&point.x, &point.y, &point.z))
		return NULL;
	
	inner_r2 = self->inner_radius*self->inner_radius;
	outer_r2 = self->outer_radius*self->outer_radius;
	Vec3_sub(&vec, &point, &self->center);
	dist2 = Vec3_len_sq(&vec);

	if (dist2 > outer_r2) {
		/* common case,  point outside sphere */
		Vec3_normalize(&norm, &vec);
		Vec3_copy(&vec, &norm);
		Vec3_scalar_muli(&vec, self->outer_radius);
		Vec3_add(&point, &vec, &self->center);
    } else if ((dist2 < inner_r2) & (dist2 > EPSILON)) {
		/* point inside the inner radius */
		Vec3_normalize(&norm, &vec);
		Vec3_copy(&vec, &norm);
		Vec3_scalar_muli(&vec, self->inner_radius);
		Vec3_add(&point, &vec, &self->center);
		Vec3_neg(&norm, &norm);
	} else {
		/* point inside sphere volume or at dead center */
		norm.x = norm.y = norm.z = 0.0f;
	}
	return pack_vectors(&point, &norm);
}

static PyMethodDef SphereDomain_methods[] = {
	{"generate", (PyCFunction)SphereDomain_generate, METH_NOARGS,
		PyDoc_STR("generate() -> Vector\n"
			"Return a random point inside the sphere or spherical shell")},
	{"intersect", (PyCFunction)SphereDomain_intersect, METH_VARARGS,
		PyDoc_STR("intersect() -> point, normal\n"
			"Intersect the line segment with the sphere and return the first\n"
			"intersection point and normal vector pointing into space from\n"
			"the sphere intersection point. If the sphere has an inner radius,\n"
			"the intersection can occur on the inner or outer shell surface.\n\n"
			"If the line does not intersect, return (None, None)")},
	{"closest_point_to", (PyCFunction)SphereDomain_closest_point_to, METH_VARARGS,
		PyDoc_STR("closest_point_to() -> Vector\n"
			"Returns the closest point on the sphere's surface\n"
			"to the supplied point.")},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
SphereDomain_getattr(SphereDomainObject *self, PyObject *name_str)
{
	if (name_str == center_str) {
		return (PyObject *)Vector_new((PyObject *)self, &self->center, 3);	
	} else if (name_str == outer_radius_str || name_str == radius_str) {
		return (PyObject *)PyFloat_FromDouble(self->outer_radius);
	} else if (name_str == inner_radius_str) {
		return (PyObject *)PyFloat_FromDouble(self->inner_radius);
	} else {
		return Py_FindMethod(SphereDomain_methods, 
			(PyObject *)self, PyString_AS_STRING(name_str));
	}
}

static int
SphereDomain_setattr(SphereDomainObject *self, PyObject *name_str, PyObject *v)
{
	int result;

	if (name_str == center_str) {
		v = PySequence_Tuple(v);
		if (v == NULL)
			return -1;
		result = PyArg_ParseTuple(v, "fff;3 floats expected",
			&self->center.x, &self->center.y, &self->center.z) - 1;
		Py_DECREF(v);
		return result;
	} else if (name_str == outer_radius_str || name_str == radius_str) {
		v = PyNumber_Float(v);
		if (v == NULL)
			return -1;
		self->outer_radius = PyFloat_AS_DOUBLE(v);
		Py_DECREF(v);
		return 0;
	} else if (name_str == inner_radius_str) {
		v = PyNumber_Float(v);
		if (v == NULL)
			return -1;
		self->inner_radius = PyFloat_AS_DOUBLE(v);
		Py_DECREF(v);
		return 0;
	} else {
		return -1;
	}
}

static PySequenceMethods SphereDomain_as_sequence = {
	0,		/* sq_length */
	0,		/* sq_concat */
	0,		/* sq_repeat */
	0,	    /* sq_item */
	0,		/* sq_slice */
	0,		/* sq_ass_item */
	0,	    /* sq_ass_slice */
	(objobjproc)SphereDomain_contains,	/* sq_contains */
};

PyDoc_STRVAR(SphereDomain__doc__, 
	"Sphere or spherical shell domain\n\n"
	"Sphere(center, outer_radius, inner_radius=0)\n\n"
	"center -- Center point of sphere (3-number sequence)\n\n"
	"outer_radius -- Radius of outermost surface of sphere\n\n"
	"inner_radius -- If greater than zero, the radius of the innermost\n"
	"suface of the spherical shell. must be <= outer_radius");

static PyTypeObject SphereDomain_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"domain.Sphere",		/*tp_name*/
	sizeof(SphereDomainObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)Domain_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	&SphereDomain_as_sequence, /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	(getattrofunc)SphereDomain_getattr, /*tp_getattro*/
	(setattrofunc)SphereDomain_setattr, /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	SphereDomain__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	SphereDomain_methods,  /*tp_methods*/
	0,                      /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)SphereDomain_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject DiscDomain_Type;

typedef struct {
	PyObject_HEAD
	Vec3 center;
	Vec3 normal;
	Vec3 up;
	Vec3 right;
	float inner_radius;
	float outer_radius;
	float d;
} DiscDomainObject;

static int
DiscDomain_set_normal(DiscDomainObject *self, PyObject *normal_in, void *closure)
{
	Vec3 axis;

	if (normal_in == NULL) {
		PyErr_SetString(PyExc_TypeError, "Cannot delete normal attribute");
		return -1;
	}
	
	if (!Vec3_FromSequence(&axis, normal_in))
		return -1;
	
	if (!Vec3_create_rot_vectors(&axis, &self->normal, &self->up, &self->right)) {
		PyErr_SetString(PyExc_ValueError, "Disc: invalid normal vector");
		return -1;
	}
	self->d = Vec3_dot(&self->center, &self->normal);
	return 0;
}

static PyObject *
DiscDomain_get_normal(DiscDomainObject *self, void *closure)
{
	return (PyObject *)Vector_new((PyObject *)self, &self->normal, 3);
}

static int
DiscDomain_init(DiscDomainObject *self, PyObject *args)
{
	PyObject *normal;

	self->inner_radius = 0.0f;
	if (!PyArg_ParseTuple(args, "(fff)Of|f:__init__",
		&self->center.x, &self->center.y, &self->center.z,
		&normal, &self->outer_radius, &self->inner_radius))
		return -1;

	if (self->outer_radius < self->inner_radius) {
		PyErr_SetString(PyExc_ValueError, 
			"Disc: Expected outer_radius >= inner_radius");
		return -1;
	}
	
	return DiscDomain_set_normal(self, normal, NULL);
}

/* Generate a random point in the disk specified */
static inline void
generate_point_in_disc(Vec3 *point, Vec3 *center, 
	float inner_radius, float outer_radius, Vec3 *up, Vec3 *right)
{
	float x, y, mag, outer_diam, range;
	
	if (inner_radius == 0.0f) {
		/* solid circle */
		outer_diam = outer_radius * 2.0f;
		do {
			x = rand_uni() * outer_diam - outer_radius;
			y = rand_uni() * outer_diam - outer_radius;
		} while ((x*x) + (y*y) > outer_radius*outer_radius);
	} else {
		/* hollow disc or circular shell */
		do {
			x = rand_norm(0.0f, 1.0f);
			y = rand_norm(0.0f, 1.0f);
			mag = (x*x) + (y*y);
		} while (mag < EPSILON);
		range = (outer_radius - inner_radius) / outer_radius;
		/* Unfortunately InvSqrt() is not precise enough for shells */
		mag = (1.0f / sqrtf(mag)) * (sqrtf(rand_uni()) * range + (1.0f - range)) * outer_radius;
		x *= mag;
		y *= mag;
	}
	/* Rotate x, y using the up and right vectors and offset from center*/
	point->x = x*right->x + y*up->x + center->x;
	point->y = x*right->y + y*up->y + center->y;
	point->z = x*right->z + y*up->z + center->z;
}

static PyObject *
DiscDomain_generate(DiscDomainObject *self) 
{
	PyObject *x, *y, *z, *pt;
	Vec3 point;

	generate_point_in_disc(&point, &self->center, self->inner_radius, self->outer_radius,
		&self->up, &self->right);

	x = PyFloat_FromDouble(point.x);
	y = PyFloat_FromDouble(point.y);
	z = PyFloat_FromDouble(point.z);
	if (x == NULL || y == NULL || z == NULL) {
		Py_XDECREF(x);
		Py_XDECREF(y);
		Py_XDECREF(z);
		return NULL;
	}
	
	pt = PyTuple_Pack(3, x, y, z);
	Py_DECREF(x);
	Py_DECREF(y);
	Py_DECREF(z);
	return pt;
}

static inline int
disc_intersect(Vec3 *sect_pt, Vec3 *sect_norm, 
	Vec3 *disc_center, Vec3 *disc_norm, float disc_cdotn,
	float disc_inner_r2, float disc_outer_r2,
	Vec3 *seg_start, Vec3 *seg_vec)
{
	Vec3 tmp, sect_v;
	float ndotv, t, dist;

	ndotv = Vec3_dot(disc_norm, seg_vec);
	if (ndotv) {
		t = (disc_cdotn - disc_norm->x*seg_start->x - disc_norm->y*seg_start->y 
			- disc_norm->z*seg_start->z) / ndotv;
		if (t >= 0.0f && t <= 1.0f) {
			/* calculate intersection point */
			Vec3_scalar_mul(&sect_v, seg_vec, t);
			Vec3_add(sect_pt, seg_start, &sect_v);
			/* calculate distance from center */
			Vec3_sub(&tmp, sect_pt, disc_center);
			dist = Vec3_len_sq(&tmp);
			if ((dist >= disc_inner_r2) & (dist <= disc_outer_r2)) {
				if (Vec3_dot(disc_norm, &sect_v) > 0.0f) {
					/* start point is on opposite side of normal */
					Vec3_scalar_mul(sect_norm, disc_norm, -1.0f);
				} else {
					Vec3_copy(sect_norm, disc_norm);
				}
				return 1;
			}
		}
	}
	return 0;
}

static PyObject *
DiscDomain_intersect(DiscDomainObject *self, PyObject *args) 
{
	Vec3 start, end, vec, point, normal;

	if (!PyArg_ParseTuple(args, "(fff)(fff):intersect",
		&start.x, &start.y, &start.z,
		&end.x, &end.y, &end.z))
		return NULL;

	Vec3_sub(&vec, &end, &start);
	if (!disc_intersect(&point, &normal, &self->center, &self->normal, self->d,
		self->inner_radius*self->inner_radius, self->outer_radius*self->outer_radius,
		&start, &vec)) {
		Py_INCREF(NO_INTERSECTION);
		return NO_INTERSECTION;
	} else {
		return pack_vectors(&point, &normal);
	}
}

static PyMethodDef DiscDomain_methods[] = {
	{"generate", (PyCFunction)DiscDomain_generate, METH_NOARGS,
		PyDoc_STR("generate() -> Vector\n"
			"Return a random point in the disc")},
	{"intersect", (PyCFunction)DiscDomain_intersect, METH_VARARGS,
		PyDoc_STR("intersect() -> point, normal\n"
			"Intersect the line segment with the disc return the intersection\n"
			"point and normal vector pointing into space on the same side of the\n"
			"disc as the start point.\n\n"
			"If the line does not intersect, or lies completely in the disc\n"
			"return (None, None)")},
	{NULL,		NULL}		/* sentinel */
};

static PyMemberDef DiscDomain_members[] = {
    {"inner_radius", T_FLOAT, offsetof(DiscDomainObject, inner_radius), 0,
        "Inner radius of disc. Set to zero for a solid circle"},
    {"outer_radius", T_FLOAT, offsetof(DiscDomainObject, outer_radius), 0,
        "Outer radius of disc. Must be >= inner_radius"},
	{NULL}
};

static PyGetSetDef DiscDomain_descriptors[] = {
	{"center", (getter)Vector_get, (setter)Vector_set, 
		"Center point of disc", (void *)offsetof(DiscDomainObject, center)},
	{"normal", (getter)DiscDomain_get_normal, (setter)DiscDomain_set_normal,
		"Normal vector that determines disc orientation", NULL},
	{NULL}
};

static int
DiscDomain_contains(DiscDomainObject *self, PyObject *pt)
{
	Vec3 point, from_center;
	float inner_r2, outer_r2, dist2;

	pt = PySequence_Tuple(pt);
	if (pt == NULL)
		return -1;
	if (!PyArg_ParseTuple(pt, "fff:__contains__", &point.x, &point.y, &point.z)) {
		Py_DECREF(pt);
		return -1;
	}
	Py_DECREF(pt);

	Vec3_sub(&from_center, &point, &self->center);
	if (fabs(Vec3_dot(&from_center, &self->normal)) < EPSILON) {
		/* point is coplanar to disc */
		outer_r2 = self->outer_radius*self->outer_radius;
		inner_r2 = self->inner_radius*self->inner_radius;
		dist2 = Vec3_len_sq(&from_center);
		return ((inner_r2 - dist2) < EPSILON) & ((dist2 - outer_r2) < EPSILON);
	}
	return 0;
}

static PySequenceMethods DiscDomain_as_sequence = {
	0,		/* sq_length */
	0,		/* sq_concat */
	0,		/* sq_repeat */
	0,	    /* sq_item */
	0,		/* sq_slice */
	0,		/* sq_ass_item */
	0,	    /* sq_ass_slice */
	(objobjproc)DiscDomain_contains,	/* sq_contains */
};

PyDoc_STRVAR(DiscDomain__doc__, 
	"Circular disc domain with arbitrary orientation\n\n"
	"Disc(center, normal, outer_radius, inner_radius=0)\n\n"
	"center -- The center point of the disc (3-number sequence)\n"
	"normal -- Normal vector perpendicular to the disc. This need not\n"
	"be a unit vector.\n"
	"outer_radius -- The outer radius of the disc.\n"
	"inner_radius -- The inner radius of the disc, must be <= outer_radius.\n"
	"defaults to 0, which creates a solid circle");

static PyTypeObject DiscDomain_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"domain.Disc",		/*tp_name*/
	sizeof(DiscDomainObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)Domain_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	&DiscDomain_as_sequence, /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	0, /*tp_getattro*/
	0, /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	DiscDomain__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	DiscDomain_methods,  /*tp_methods*/
	DiscDomain_members,  /*tp_members*/
	DiscDomain_descriptors, /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)DiscDomain_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject CylinderDomain_Type;

typedef struct {
	PyObject_HEAD
	Vec3 end_point0;
	Vec3 end_point1;
	Vec3 axis;
	Vec3 axis_norm;
	Vec3 up;
	Vec3 right;
	float len;
	float len_sq;
	float inner_radius;
	float outer_radius;
} CylinderDomainObject;

/* Setup rotation vectors after changing cylinder end points */
static int
CylinderDomain_setup_rot(CylinderDomainObject *self)
{
	Vec3_sub(&self->axis, &self->end_point1, &self->end_point0);
	self->len_sq = Vec3_len_sq(&self->axis);
	self->len = sqrtf(self->len_sq);
	if (self->len_sq < EPSILON || 
		!Vec3_create_rot_vectors(&self->axis, &self->axis_norm, &self->up, &self->right)) {
		PyErr_SetString(PyExc_ValueError, "Cylinder: End points too close");
		return -1;
	}
	return 0;
}

static int
CylinderDomain_init(CylinderDomainObject *self, PyObject *args)
{
	self->inner_radius = 0.0f;
	if (!PyArg_ParseTuple(args, "(fff)(fff)f|f:__init__",
		&self->end_point0.x, &self->end_point0.y, &self->end_point0.z,
		&self->end_point1.x, &self->end_point1.y, &self->end_point1.z,
		&self->outer_radius, &self->inner_radius))
		return -1;

	if (self->outer_radius < self->inner_radius) {
		PyErr_SetString(PyExc_ValueError, 
			"Cylinder: Expected outer_radius >= inner_radius");
		return -1;
	}
	
	return CylinderDomain_setup_rot(self);
}

static PyObject *
CylinderDomain_generate(CylinderDomainObject *self) 
{
	PyObject *x, *y, *z, *pt;
	Vec3 center, point;
	float d;

	Vec3_sub(&center, &self->end_point1, &self->end_point0);
	d = rand_uni();
	Vec3_scalar_muli(&center, d);
	Vec3_addi(&center, &self->end_point0);
	generate_point_in_disc(&point, &center, self->inner_radius, self->outer_radius,
		&self->up, &self->right);

	x = PyFloat_FromDouble(point.x);
	y = PyFloat_FromDouble(point.y);
	z = PyFloat_FromDouble(point.z);
	if (x == NULL || y == NULL || z == NULL) {
		Py_XDECREF(x);
		Py_XDECREF(y);
		Py_XDECREF(z);
		return NULL;
	}
	
	pt = PyTuple_Pack(3, x, y, z);
	Py_DECREF(x);
	Py_DECREF(y);
	Py_DECREF(z);
	return pt;
}

static PyObject *
CylinderDomain_intersect(CylinderDomainObject *self, PyObject *args) 
{
	Vec3 start, end, to_start, seg, tmp, xa, xb, norm, tp, tn;
	float inner_r2, outer_r2, r2, d2, dir, a, b, c, bb4ac, t, t1, t2;
	int collided = 0;

	if (!PyArg_ParseTuple(args, "(fff)(fff):intersect",
		&start.x, &start.y, &start.z,
		&end.x, &end.y, &end.z))
		return NULL;
	
	/* The assumed common-case here is no intersection, so we are
	   optimizing for that case. The idea is to cheaply see if
	   the start and end point could possibly intersect and bail
	   early if not.

	   To accomplish that, we find the distance between the start
	   point and the infinite line along the cylinder axis.
	   Then we see if the end point is sufficiently far from the
	   start that it could intersect the domain surface. Only if
	   this possibility exists is the actual intersection test
	   performed.

	   Point to line distance algorithm derived from:
	   http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
	*/
	Vec3_sub(&to_start, &start, &self->end_point0);
	Vec3_cross(&tmp, &self->axis, &to_start);
	t = Vec3_len_sq(&tmp) / self->len_sq;
	a = t * InvSqrt(t); /* distance from start to axis */
	Vec3_sub(&seg, &end, &start);
	b = Vec3_len(&seg);
	inner_r2 = self->inner_radius*self->inner_radius;
	outer_r2 = self->outer_radius*self->outer_radius;

	if ((fabs(a - self->outer_radius) > b) & (fabs(a - self->inner_radius) > b)) {
		/* No chance of intersection */
		Py_INCREF(NO_INTERSECTION);
		return NO_INTERSECTION;
	} else if (a >= self->outer_radius) {
		r2 = outer_r2;
		dir = 1.0f;	
	} else if (!self->inner_radius) {
		/* start point potentially contained in solid cylinder volume */
		r2 = outer_r2;
		dir = -1.0f;
	} else if (a <= self->inner_radius) {
		r2 = inner_r2;
		dir = -1.0f;
	} else {
		/* start point potentially contained in shell cylinder volume,
		   we'll need to compare it to the end point distance to axis
		*/
		Vec3_sub(&tmp, &self->end_point0, &end);
		Vec3_cross(&tmp, &self->axis, &tmp);
		t = Vec3_len_sq(&tmp) / self->len_sq;
		c = t * InvSqrt(t); /* distance from end to axis */
		if (c > a) {
			r2 = outer_r2;
			dir = -1.0f;
		} else {
			r2 = inner_r2;
			dir = 1.0f;
		}
	}
	norm.x = norm.y = norm.z = 0.0f; /* shutup compiler warning */
	d2 = FLT_MAX;

	/* Check for end cap collision at end point 0 */
	if (disc_intersect(&end, &norm, 
		&self->end_point0, &self->axis_norm, Vec3_dot(&self->end_point0, &self->axis_norm),
		inner_r2, outer_r2, &start, &seg)) {
		Vec3_sub(&tmp, &end, &start);
		d2 = Vec3_len_sq(&tmp);
		collided = 1;
	}

	/* Check for end cap collision at end point 1 */
	if (disc_intersect(&tp, &tn, 
		&self->end_point1, &self->axis_norm, Vec3_dot(&self->end_point1, &self->axis_norm),
		inner_r2, outer_r2, &start, &seg)) {
		Vec3_sub(&tmp, &tp, &start);
		t2 = Vec3_len_sq(&tmp);
		if (t2 < d2) {
			Vec3_copy(&end, &tp);
			Vec3_copy(&norm, &tn);
			d2 = t2;
			collided = 1;
		}
	}

	/* Check for intersection against infinite cylinder along axis */
	Vec3_cross(&xa, &to_start, &self->axis);
	Vec3_cross(&xb, &seg, &self->axis);
	a = Vec3_len_sq(&xb);
	b = 2.0f * Vec3_dot(&xb, &xa);
	c = Vec3_len_sq(&xa) - (r2 * self->len_sq);
	bb4ac = b*b - 4.0f * a * c;
	// printf("r2 = %f\n", r2);
	// printf("a = %f, b = %f, c = %f, bb4ac = %f\n", a, b, c, bb4ac);
	if (fabs(bb4ac) <= EPSILON) {
		/* intersects at single point */
		t = -b / (2.0f * a);
	} else if (bb4ac >= -EPSILON) {
		/* 2-point intersection */
		bb4ac = sqrtf(bb4ac);
		t1 = (-b - bb4ac) / (2.0f * a);
		t2 = (-b + bb4ac) / (2.0f * a);
		t = ((t2 < 0.0f) | (t2 > 1.0f)) ? t1 : 
			((t1 < 0.0f) | (t1 > 1.0f)) ? t2 :
			min(t1, t2);
		// printf("t1 = %f, t2 = %f\n", t1, t2);
	} else if (collided) {
		/* collided only against an end cap */
		return pack_vectors(&end, &norm);
	} else {
		Py_INCREF(NO_INTERSECTION);
		return NO_INTERSECTION;
	}
	if ((t < 0.0f) | (t > 1.0f)) {
		/* intersection point not in segment */
		Py_INCREF(NO_INTERSECTION);
		return NO_INTERSECTION;
	}
	// printf("t = %f\n", t);
	Vec3_scalar_muli(&seg, t);
	Vec3_add(&tp, &start, &seg);
	/* Project the intersection point of the infinite cylinder 
	   onto the axis to determine if it truly intersects
	   and allow us to compute the normal */
	Vec3_sub(&tmp, &tp, &self->end_point0);
	t = Vec3_dot(&self->axis_norm, &tmp);

	if ((t >= 0.0f) & (t <= self->len)) {
		/* Intersects middle of cylinder */
		if (collided) {
			Vec3_sub(&tmp, &tp, &start);
			if (d2 <= Vec3_len_sq(&tmp)) {
				/* Other collisions were closer */
				return pack_vectors(&end, &norm);
			}
		}
		Vec3_scalar_mul(&tmp, &self->axis_norm, t);
		Vec3_addi(&tmp, &self->end_point0);
		Vec3_sub(&norm, &tp, &tmp);
		Vec3_scalar_muli(&norm, dir);
		Vec3_normalize(&norm, &norm);
		return pack_vectors(&tp, &norm);
	}
	if (collided) {
		return pack_vectors(&end, &norm);
	}
	Py_INCREF(NO_INTERSECTION);
	return NO_INTERSECTION;
}

static int Cylinder_set_end_point0(CylinderDomainObject *self, PyObject *value, void *closure)
{
	if (value == NULL) {
		PyErr_SetString(PyExc_TypeError, "Cannot delete end_point0 attribute");
		return -1;
	}
	if (!Vec3_FromSequence(&self->end_point0, value)) {
		return -1;
	}
	return CylinderDomain_setup_rot(self);
}

static int Cylinder_set_end_point1(CylinderDomainObject *self, PyObject *value, void *closure)
{
	if (value == NULL) {
		PyErr_SetString(PyExc_TypeError, "Cannot delete end_point1 attribute");
		return -1;
	}
	if (!Vec3_FromSequence(&self->end_point1, value)) {
		return -1;
	}
	return CylinderDomain_setup_rot(self);
}

static PyMethodDef CylinderDomain_methods[] = {
	{"generate", (PyCFunction)CylinderDomain_generate, METH_NOARGS,
		PyDoc_STR("generate() -> Vector\n"
			"Return a random point in the cylinder volume")},
	{"intersect", (PyCFunction)CylinderDomain_intersect, METH_VARARGS,
		PyDoc_STR("intersect() -> point, normal\n"
			"Intersect the line segment with the cylinder return the intersection\n"
			"point and normal vector pointing into space on the same side of the\n"
			"surface as the start point.\n\n"
			"If the line does not intersect, or lies completely in the cylinder\n"
			"return (None, None)")},
	{NULL,		NULL}		/* sentinel */
};

static PyMemberDef CylinderDomain_members[] = {
    {"inner_radius", T_FLOAT, offsetof(CylinderDomainObject, inner_radius), 0,
        "Inner radius of disc. Set to zero for a solid circle"},
    {"outer_radius", T_FLOAT, offsetof(CylinderDomainObject, outer_radius), 0,
        "Outer radius of disc. Must be >= inner_radius"},
	{"length", T_FLOAT, offsetof(CylinderDomainObject, len), RO,
		"Length of cylinder axis"},
	{NULL}
};

static PyGetSetDef CylinderDomain_descriptors[] = {
	{"end_point0", (getter)Vector_get, (setter)Cylinder_set_end_point0, 
		"End point of cylinder axis", (void *)offsetof(CylinderDomainObject, end_point0)},
	{"end_point1", (getter)Vector_get, (setter)Cylinder_set_end_point1, 
		"End point of cylinder axis", (void *)offsetof(CylinderDomainObject, end_point1)},
	{NULL}
};

static int
CylinderDomain_contains(CylinderDomainObject *self, PyObject *pt)
{
	Vec3 point, from_end, tmp;
	float inner_r2, outer_r2, dist2, c;

	pt = PySequence_Tuple(pt);
	if (pt == NULL)
		return -1;
	if (!PyArg_ParseTuple(pt, "fff:__contains__", &point.x, &point.y, &point.z)) {
		Py_DECREF(pt);
		return -1;
	}
	Py_DECREF(pt);

	inner_r2 = self->inner_radius*self->inner_radius;
	outer_r2 = self->outer_radius*self->outer_radius;
	Vec3_sub(&from_end, &point, &self->end_point0);
	Vec3_cross(&tmp, &self->axis, &from_end);
	dist2 = Vec3_len_sq(&tmp) / self->len_sq; /* sq distance from point to axis */
	c = Vec3_dot(&self->axis_norm, &from_end);
	return ((inner_r2 - dist2) < EPSILON) & ((dist2 - outer_r2) < EPSILON) 
		& (c >= 0.0f) & (c <= self->len);
}

static PySequenceMethods CylinderDomain_as_sequence = {
	0,		/* sq_length */
	0,		/* sq_concat */
	0,		/* sq_repeat */
	0,	    /* sq_item */
	0,		/* sq_slice */
	0,		/* sq_ass_item */
	0,	    /* sq_ass_slice */
	(objobjproc)CylinderDomain_contains,	/* sq_contains */
};

PyDoc_STRVAR(CylinderDomain__doc__, 
	"Capped right-cylinder domain with arbitrary orientation\n\n"
	"Cylinder(end_point0, end_point1, outer_radius, inner_radius=0)\n\n"
	"end_point0 -- End point of cylinder axis\n"
	"end_point1 -- End point of cylinder axis\n"
	"outer_radius -- The outer radius of the cylinder volume.\n"
	"inner_radius -- The inner radius of the cylinder, must be <= outer_radius.\n"
	"defaults to 0, which creates a solid volume");

static PyTypeObject CylinderDomain_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"domain.Cylinder",		/*tp_name*/
	sizeof(CylinderDomainObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)Domain_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	&CylinderDomain_as_sequence, /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	0, /*tp_getattro*/
	0, /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	CylinderDomain__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	CylinderDomain_methods,  /*tp_methods*/
	CylinderDomain_members,  /*tp_members*/
	CylinderDomain_descriptors, /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)CylinderDomain_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject ConeDomain_Type;

typedef struct {
	PyObject_HEAD
	Vec3 apex;
	Vec3 base;
	Vec3 axis;
	Vec3 axis_norm;
	Vec3 up;
	Vec3 right;
	float len;
	float len_sq;
	float inner_radius;
	float outer_radius;
	float inner_cosa;
	float outer_cosa;
} ConeDomainObject;

/* Recalculate afer changing cone radii */
static void
ConeDomain_set_radius(ConeDomainObject *self)
{
	Vec3 wall, offset;

	Vec3_scalar_mul(&offset, &self->right, self->outer_radius);
	Vec3_add(&wall, &self->axis, &offset);
	Vec3_normalize(&wall, &wall);
	self->outer_cosa = Vec3_dot(&self->axis_norm, &wall);
	if (self->inner_radius) {
		Vec3_scalar_mul(&offset, &self->right, self->inner_radius);
		Vec3_add(&wall, &self->axis, &offset);
		Vec3_normalize(&wall, &wall);
		self->inner_cosa = Vec3_dot(&self->axis_norm, &wall);
	} else {
		self->inner_cosa = 1.0f;
	}
}

/* Setup rotation vectors after changing cone end points */
static int
ConeDomain_setup_rot(ConeDomainObject *self)
{
	Vec3_sub(&self->axis, &self->base, &self->apex);
	self->len_sq = Vec3_len_sq(&self->axis);
	self->len = sqrtf(self->len_sq);
	if (self->len_sq < EPSILON || 
		!Vec3_create_rot_vectors(&self->axis, &self->axis_norm, &self->up, &self->right)) {
		PyErr_SetString(PyExc_ValueError, "Cone: Apex and end point too close");
		return -1;
	}
	ConeDomain_set_radius(self);
	return 0;
}

static int
ConeDomain_init(ConeDomainObject *self, PyObject *args)
{
	self->inner_radius = 0.0f;
	if (!PyArg_ParseTuple(args, "(fff)(fff)f|f:__init__",
		&self->apex.x, &self->apex.y, &self->apex.z,
		&self->base.x, &self->base.y, &self->base.z,
		&self->outer_radius, &self->inner_radius))
		return -1;

	if (self->outer_radius < self->inner_radius) {
		PyErr_SetString(PyExc_ValueError, 
			"Cone: Expected outer_radius >= inner_radius");
		return -1;
	}
	
	return ConeDomain_setup_rot(self);
}

static PyObject *
ConeDomain_generate(ConeDomainObject *self) 
{
	PyObject *x, *y, *z, *pt;
	Vec3 center, point;
	float d;

	Vec3_copy(&center, &self->axis);
	d = sqrtf(rand_uni());
	Vec3_scalar_muli(&center, d);
	Vec3_addi(&center, &self->apex);
	generate_point_in_disc(&point, &center, self->inner_radius*d, self->outer_radius*d,
		&self->up, &self->right);

	x = PyFloat_FromDouble(point.x);
	y = PyFloat_FromDouble(point.y);
	z = PyFloat_FromDouble(point.z);
	if (x == NULL || y == NULL || z == NULL) {
		Py_XDECREF(x);
		Py_XDECREF(y);
		Py_XDECREF(z);
		return NULL;
	}
	
	pt = PyTuple_Pack(3, x, y, z);
	Py_DECREF(x);
	Py_DECREF(y);
	Py_DECREF(z);
	return pt;
}

/* Set point to the point on the segment at t
   Return true if the point is in the segment and on the "right" side of the cone */
static inline int
cone_sect_point(Vec3 *point, Vec3 *seg_start, Vec3 *seg_norm, float seg_len, float t, 
	Vec3 *cone_apex, Vec3 *cone_axis, float cone_len)
{
	Vec3 to_pt;
	float h;

	Vec3_scalar_mul(point, seg_norm, t);
	Vec3_addi(point, seg_start);
	Vec3_sub(&to_pt, point, cone_apex);
	h = Vec3_dot(&to_pt, cone_axis);
	// printf("t=%f, sl=%f, h=%f, cl=%f, valid=%d\n", t, seg_len, h, cone_len, (t > 0.0f) & (t <= seg_len) & (h > -EPSILON) & (h <= cone_len));
	return (t > EPSILON) & (t <= seg_len) & (h > -EPSILON) & (h <= cone_len);
}

static int
cone_intersect(Vec3 *sect_pt, Vec3 *sect_norm, 
	Vec3 *cone_apex, Vec3 *cone_axis, float cone_cosa, float cone_len,
	Vec3 *seg_start, Vec3 *seg_norm, float seg_len)
{
	/* Code derived from: http://www.geometrictools.com/Documentation/IntersectionLineCone.pdf */
	Vec3 to_start, to_sect, pt1, pt2;
	float cosa2, d1, d2, a, b, c, bbac;
	float t1, t2 = FLT_MAX;
	int pt1_valid, pt2_valid, at_apex;

	d1 = Vec3_dot(cone_axis, seg_norm);
	cosa2 = cone_cosa * cone_cosa;
	Vec3_sub(&to_start, seg_start, cone_apex);
	d2 = Vec3_dot(cone_axis, &to_start);
	a = d1*d1 - cosa2;
	b = d1*d2 - cosa2 * Vec3_dot(seg_norm, &to_start);
	c = d2*d2 - cosa2 * Vec3_len_sq(&to_start);
	pt2.x = pt2.y = pt2.z = 0.0f; /* shutup compiler warning */
	// printf("a=%f, b=%f, c=%f\n", a, b, c);
	
	if ((a > EPSILON) | (a < -EPSILON)) {
		bbac = b*b - a*c;
		// printf("bbac=%f\n", bbac);
		if (bbac < -EPSILON) {
			return 0; /* no intersection */
		} else if (bbac < EPSILON) { /* desr == 0 */
			t1 = -b / a; /* intersects at single point */
			pt1_valid = cone_sect_point(&pt1, seg_start, seg_norm, seg_len, 
				t1, cone_apex, cone_axis, cone_len);
			pt2_valid = 0;
		} else {
			/* two intersection points */
			bbac = sqrtf(bbac);
			t1 = (-b + bbac) / a;
			pt1_valid = cone_sect_point(&pt1, seg_start, seg_norm, seg_len,
				t1, cone_apex, cone_axis, cone_len);
			t2 = (-b - bbac) / a;
			pt2_valid = cone_sect_point(&pt2, seg_start, seg_norm, seg_len,
				t2, cone_apex, cone_axis, cone_len);
		}
	} else if ((b > EPSILON) | (b < -EPSILON)) {
		/* a = 0, b != 0 */
		t1 = -0.5f * c / b;
		// printf("t1=%f\n", t1);
		pt1_valid = cone_sect_point(&pt1, seg_start, seg_norm, seg_len,
			t1, cone_apex, cone_axis, cone_len);
		pt2_valid = 0;
	} else if ((c > EPSILON) | (c < -EPSILON)) {
		/* a = b = 0, c != 0 */
		return 0;
	} else {
		/* a = b = c = 0, intersects apex */
		Vec3_copy(sect_pt, cone_apex);
		Vec3_neg(sect_norm, cone_axis);
		return 1;
	}
	if (pt1_valid & (!pt2_valid | (t1 <= t2))) {
		Vec3_copy(sect_pt, &pt1);
	} else if (pt2_valid & (!pt1_valid | (t2 <= t1))) {
		Vec3_copy(sect_pt, &pt2);
	} else {
		return 0;
	}
	/* calculate the normal by projecting the point onto the axis 
	   perpendicular to the cone surface */
	Vec3_sub(&to_sect, sect_pt, cone_apex);
	t1 = Vec3_dot(&to_sect, cone_axis) / cosa2;
	Vec3_scalar_mul(&pt1, cone_axis, t1);
	// printf("cosa2=%f, t1=%f, d=%f\n", cosa2, t1, Vec3_dot(&to_sect, cone_axis));
	Vec3_addi(&pt1, cone_apex);
	Vec3_sub(&pt1, sect_pt, &pt1);
	at_apex = !Vec3_normalize(sect_norm, &pt1);
	if (at_apex) {
		Vec3_neg(sect_norm, cone_axis);
	}
	return 1;
}
		
static PyObject *
ConeDomain_intersect(ConeDomainObject *self, PyObject *args) 
{
	Vec3 start, end, to_start, seg, seg_norm, tmp, norm, tp, tn;
	float d2, a, b, t2, seg_len;
	float dir = 1.0f;
	int collided = 0;

	if (!PyArg_ParseTuple(args, "(fff)(fff):intersect",
		&start.x, &start.y, &start.z,
		&end.x, &end.y, &end.z))
		return NULL;
	
	/* figure out where the start point is in relation to the 
	   cone volume. It's either outside the outer cone, inside the
	   inner cone or inside the cone volume
	*/
	Vec3_sub(&seg, &end, &start);
	seg_len = Vec3_len(&seg);
	if (seg_len) {
		Vec3_scalar_div(&seg_norm, &seg, seg_len);
		Vec3_sub(&to_start, &start, &self->apex);
		Vec3_normalize(&to_start, &to_start);
		a = Vec3_dot(&to_start, &self->axis_norm);
		norm.x = norm.y = norm.z = 0.0f; /* shutup compiler warning */
		d2 = FLT_MAX;
		// printf("a=%f, cosa=%f\n", a, self->outer_cosa);
		if (a <= self->inner_cosa) {
			// printf("start outside\n");
			/* start outside inner cone */
			if (cone_intersect(&end, &norm,	
				&self->apex, &self->axis_norm, self->outer_cosa, self->len,
				&start, &seg_norm, seg_len)) {
				Vec3_sub(&tmp, &end, &start);
				d2 = Vec3_len_sq(&tmp);
				collided = 1;
				dir = (a <= self->outer_cosa) * 2.0f - 1.0f;
			}
		}
		Vec3_sub(&to_start, &start, &self->base);
		b = Vec3_dot(&to_start, &self->axis);
		if ((b > 0.0f) | (a > self->inner_cosa)) {
			/* start is "below" base plane or inside inner cone */
			// printf("start below\n");
			if (self->inner_cosa < 1.0f && cone_intersect(&tp, &tn, 
				&self->apex, &self->axis_norm, self->inner_cosa, self->len,
				&start, &seg_norm, seg_len)) {
				Vec3_sub(&tmp, &tp, &start);
				t2 = Vec3_len_sq(&tmp);
				collided = 1;
				if (t2 < d2) {
					Vec3_copy(&end, &tp);
					Vec3_copy(&norm, &tn);
					d2 = t2;
					dir = (a <= self->inner_cosa) * 2.0f - 1.0f;
				}
			}
			if (disc_intersect(&tp, &tn,
				&self->base, &self->axis_norm, Vec3_dot(&self->base, &self->axis_norm),
				self->inner_radius*self->inner_radius, self->outer_radius*self->outer_radius,
				&start, &seg)) {
				Vec3_sub(&tmp, &tp, &start);
				collided = 1;
				if (Vec3_len_sq(&tmp) < d2) {
					Vec3_copy(&end, &tp);
					Vec3_copy(&norm, &tn);
					dir = 1.0f;
				}
			}
		}
	} else {
		Py_INCREF(NO_INTERSECTION);
		return NO_INTERSECTION;
	}
	if (collided) {
		// printf("dir=%f\n", dir);
		Vec3_scalar_muli(&norm, dir);
		return pack_vectors(&end, &norm);
	} else {
		Py_INCREF(NO_INTERSECTION);
		return NO_INTERSECTION;
	}
}

static int Cone_set_apex(ConeDomainObject *self, PyObject *value, void *closure)
{
	if (value == NULL) {
		PyErr_SetString(PyExc_TypeError, "Cannot delete apex attribute");
		return -1;
	}
	if (!Vec3_FromSequence(&self->apex, value)) {
		return -1;
	}
	return ConeDomain_setup_rot(self);
}

static int Cone_set_base(ConeDomainObject *self, PyObject *value, void *closure)
{
	if (value == NULL) {
		PyErr_SetString(PyExc_TypeError, "Cannot delete base attribute");
		return -1;
	}
	if (!Vec3_FromSequence(&self->base, value)) {
		return -1;
	}
	return ConeDomain_setup_rot(self);
}

static int Cone_set_inner_radius(ConeDomainObject *self, PyObject *value, void *closure)
{
	float radius;

	if (value == NULL) {
		PyErr_SetString(PyExc_TypeError, "Cannot delete inner_radius attribute");
		return -1;
	}
	value = PyNumber_Float(value);
	if (value != NULL) {
		radius = PyFloat_AS_DOUBLE(value);
		if (radius > self->outer_radius) {
			PyErr_SetString(PyExc_ValueError, 
				"Cone: Expected outer_radius >= inner_radius");
			return -1;
		}
		self->inner_radius = radius;
		ConeDomain_set_radius(self);
		Py_DECREF(value);
		return 0;
	}
	return -1;
}

static PyObject *
Cone_get_inner_radius(ConeDomainObject *self, void *closure)
{
	return PyFloat_FromDouble(self->inner_radius);
}

static int Cone_set_outer_radius(ConeDomainObject *self, PyObject *value, void *closure)
{
	float radius;

	if (value == NULL) {
		PyErr_SetString(PyExc_TypeError, "Cannot delete outer_radius attribute");
		return -1;
	}
	value = PyNumber_Float(value);
	if (value != NULL) {
		radius = PyFloat_AS_DOUBLE(value);
		if (radius < self->inner_radius) {
			PyErr_SetString(PyExc_ValueError, 
				"Cone: Expected outer_radius >= inner_radius");
			return -1;
		}
		self->outer_radius = radius;
		ConeDomain_set_radius(self);
		Py_DECREF(value);
		return 0;
	}
	return -1;
}

static PyObject *
Cone_get_outer_radius(ConeDomainObject *self, void *closure)
{
	return PyFloat_FromDouble(self->outer_radius);
}

static PyMethodDef ConeDomain_methods[] = {
	{"generate", (PyCFunction)ConeDomain_generate, METH_NOARGS,
		PyDoc_STR("generate() -> Vector\n"
			"Return a random point in the cylinder volume")},
	{"intersect", (PyCFunction)ConeDomain_intersect, METH_VARARGS,
		PyDoc_STR("intersect() -> point, normal\n"
			"Intersect the line segment with the cylinder return the intersection\n"
			"point and normal vector pointing into space on the same side of the\n"
			"surface as the start point.\n\n"
			"If the line does not intersect, or lies completely in the cylinder\n"
			"return (None, None)")},
	{NULL,		NULL}		/* sentinel */
};

static PyMemberDef ConeDomain_members[] = {
	{"length", T_FLOAT, offsetof(ConeDomainObject, len), RO,
		"Length of cylinder axis"},
	{NULL}
};

static PyGetSetDef ConeDomain_descriptors[] = {
	{"apex", (getter)Vector_get, (setter)Cone_set_apex, 
		"End point of cylinder axis", (void *)offsetof(ConeDomainObject, apex)},
	{"base", (getter)Vector_get, (setter)Cone_set_base, 
		"End point of cylinder axis", (void *)offsetof(ConeDomainObject, base)},
	{"inner_radius", (getter)Cone_get_inner_radius, (setter)Cone_set_inner_radius, 
		"Inner radius of cone base. Set to zero for a solid volume", NULL},
	{"outer_radius", (getter)Cone_get_outer_radius, (setter)Cone_set_outer_radius, 
		"Outer radius of cone base. Must be >= inner_radius", NULL},
	{NULL}
};

static int
ConeDomain_contains(ConeDomainObject *self, PyObject *pt)
{
	Vec3 point, from_apex, from_base;
	float axis_cos, base_cos;
	int at_apex;

	pt = PySequence_Tuple(pt);
	if (pt == NULL)
		return -1;
	if (!PyArg_ParseTuple(pt, "fff:__contains__", &point.x, &point.y, &point.z)) {
		Py_DECREF(pt);
		return -1;
	}
	Py_DECREF(pt);

	Vec3_sub(&from_apex, &point, &self->apex);
	at_apex = !Vec3_normalize(&from_apex, &from_apex);
	axis_cos = Vec3_dot(&from_apex, &self->axis_norm);
	Vec3_sub(&from_base, &point, &self->base);
	base_cos = Vec3_dot(&from_base, &self->axis_norm);
	return at_apex | ((axis_cos - self->inner_cosa < EPSILON) 
		& (self->outer_cosa - axis_cos < EPSILON)
		& (base_cos <= 0.0f));
}

static PySequenceMethods ConeDomain_as_sequence = {
	0,		/* sq_length */
	0,		/* sq_concat */
	0,		/* sq_repeat */
	0,	    /* sq_item */
	0,		/* sq_slice */
	0,		/* sq_ass_item */
	0,	    /* sq_ass_slice */
	(objobjproc)ConeDomain_contains,	/* sq_contains */
};

PyDoc_STRVAR(ConeDomain__doc__, 
	"Right-cone domain with arbitrary orientation\n\n"
	"Cone(apex, base, outer_radius, inner_radius=0)\n\n"
	"apex -- End point of cone axis at the apex where it tapers to zero radius.\n"
	"base -- End point of cone axis at the base of the cone.\n"
	"outer_radius -- The outer radius of the cone volume.\n"
	"inner_radius -- The inner radius of the cone, must be <= outer_radius.\n"
	"Describes the radius of a smaller cone that is subtracted from the larger\n"
	"cone described by the outer_radius. defaults to 0, which creates a solid volume");

static PyTypeObject ConeDomain_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"domain.Cone",		/*tp_name*/
	sizeof(ConeDomainObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)Domain_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	&ConeDomain_as_sequence, /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	0, /*tp_getattro*/
	0, /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	ConeDomain__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	ConeDomain_methods,  /*tp_methods*/
	ConeDomain_members,  /*tp_members*/
	ConeDomain_descriptors, /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)ConeDomain_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

PyMODINIT_FUNC
init_domain(void)
{
	PyObject *m;

	/* Bind tp_new and tp_alloc here to appease certain compilers */
	LineDomain_Type.tp_alloc = PyType_GenericAlloc;
	LineDomain_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&LineDomain_Type) < 0)
		return;

	PlaneDomain_Type.tp_alloc = PyType_GenericAlloc;
	PlaneDomain_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&PlaneDomain_Type) < 0)
		return;

	AABoxDomain_Type.tp_alloc = PyType_GenericAlloc;
	AABoxDomain_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&AABoxDomain_Type) < 0)
		return;

	SphereDomain_Type.tp_alloc = PyType_GenericAlloc;
	SphereDomain_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&SphereDomain_Type) < 0)
		return;

	DiscDomain_Type.tp_alloc = PyType_GenericAlloc;
	DiscDomain_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&DiscDomain_Type) < 0)
		return;

	CylinderDomain_Type.tp_alloc = PyType_GenericAlloc;
	CylinderDomain_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&CylinderDomain_Type) < 0)
		return;

	ConeDomain_Type.tp_alloc = PyType_GenericAlloc;
	ConeDomain_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&ConeDomain_Type) < 0)
		return;

	/* Create the module and add the types */
	m = Py_InitModule3("_domain", NULL, "Spacial domains");
	if (m == NULL)
		return;

	/* initialize singleton marker for no intersection */
	NO_INTERSECTION = PyTuple_Pack(2, Py_None, Py_None);
	if (NO_INTERSECTION == NULL)
		return;
	
	/* Intern attribute name strings for fast access */
	point_str = PyString_InternFromString("point");
	if (point_str == NULL)
		return;
	normal_str = PyString_InternFromString("normal");
	if (normal_str == NULL)
		return;
	start_point_str = PyString_InternFromString("start_point");
	if (start_point_str == NULL)
		return;
	end_point_str = PyString_InternFromString("end_point");
	if (end_point_str == NULL)
		return;
	min_point_str = PyString_InternFromString("min_point");
	if (min_point_str == NULL)
		return;
	max_point_str = PyString_InternFromString("max_point");
	if (max_point_str == NULL)
		return;
	inner_radius_str = PyString_InternFromString("inner_radius");
	if (inner_radius_str == NULL)
		return;
	outer_radius_str = PyString_InternFromString("outer_radius");
	if (outer_radius_str == NULL)
		return;
	radius_str = PyString_InternFromString("radius");
	if (radius_str == NULL)
		return;
	center_str = PyString_InternFromString("center");
	if (center_str == NULL)
		return;

	Py_INCREF(&LineDomain_Type);
	PyModule_AddObject(m, "Line", (PyObject *)&LineDomain_Type);
	Py_INCREF(&PlaneDomain_Type);
	PyModule_AddObject(m, "Plane", (PyObject *)&PlaneDomain_Type);
	Py_INCREF(&AABoxDomain_Type);
	PyModule_AddObject(m, "AABox", (PyObject *)&AABoxDomain_Type);
	Py_INCREF(&SphereDomain_Type);
	PyModule_AddObject(m, "Sphere", (PyObject *)&SphereDomain_Type);
	Py_INCREF(&DiscDomain_Type);
	PyModule_AddObject(m, "Disc", (PyObject *)&DiscDomain_Type);
	Py_INCREF(&CylinderDomain_Type);
	PyModule_AddObject(m, "Cylinder", (PyObject *)&CylinderDomain_Type);
	Py_INCREF(&ConeDomain_Type);
	PyModule_AddObject(m, "Cone", (PyObject *)&ConeDomain_Type);

	rand_seed(time(NULL));
}
