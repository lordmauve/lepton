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
 * $Id $ */

#include <Python.h>
#include <math.h>
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
	PyObject *x, *y, *z;
	Vec3 direction;

	Vec3_sub(&direction, &self->end_point, &self->start_point);
	d = rand_uni();
	x = PyFloat_FromDouble(self->start_point.x + direction.x * d);
	if (x == NULL)
		return NULL;
	y = PyFloat_FromDouble(self->start_point.y + direction.y * d);
	if (y == NULL)
		return NULL;
	z = PyFloat_FromDouble(self->start_point.z + direction.z * d);
	if (z == NULL)
		return NULL;
	
	return PyTuple_Pack(3, x, y, z);
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
LineDomain_getattr(LineDomainObject *self, PyObject *o)
{
	char *name = PyString_AS_STRING(o);
	if (!strcmp(name, "start_point")) {
		return (PyObject *)Vector_new(NULL, &self->start_point, 3);	
	} else if (!strcmp(name, "end_point")) {
		return (PyObject *)Vector_new(NULL, &self->end_point, 3);	
	} else {
		return Py_FindMethod(LineDomain_methods, (PyObject *)self, name);
	}
}

static int
LineDomain_setattr(LineDomainObject *self, PyObject *nameob, PyObject *v)
{
	Vec3 *point;
	int result;
	char *name = PyString_AS_STRING(nameob);

	if (!strcmp(name, "start_point")) {
		point = &self->start_point;
	} else if (!strcmp(name, "end_point")) {
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
		/* Normalize more precisely then via Vec3_normalize,
		   checking for zero */
		if (len) {
			len = sqrt(len);
			self->normal.x /= len;
			self->normal.y /= len;
			self->normal.z /= len;
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
	PyObject *x, *y, *z;

	x = PyFloat_FromDouble(self->point.x);
	if (x == NULL)
		return NULL;
	y = PyFloat_FromDouble(self->point.y);
	if (y == NULL)
		return NULL;
	z = PyFloat_FromDouble(self->point.z);
	if (z == NULL)
		return NULL;
	
	return PyTuple_Pack(3, x, y, z);
}

static PyObject *
PlaneDomain_intersect(PlaneDomainObject *self, PyObject *args) 
{
	Vec3 norm, start, end, vec;
	float ndotv, t, dist;

	if (!PyArg_ParseTuple(args, "(fff)(fff):__init__",
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
PlaneDomain_getattr(PlaneDomainObject *self, PyObject *o)
{
	char *name = PyString_AS_STRING(o);
	if (!strcmp(name, "point")) {
		return (PyObject *)Vector_new(NULL, &self->point, 3);	
	} else if (!strcmp(name, "normal")) {
		return (PyObject *)Vector_new(NULL, &self->normal, 3);	
	} else {
		return Py_FindMethod(PlaneDomain_methods, (PyObject *)self, name);
	}
}

static int
PlaneDomain_setattr(PlaneDomainObject *self, PyObject *nameob, PyObject *v)
{
	Vec3 *point;
	int result;
	char *name = PyString_AS_STRING(nameob);

	if (!strcmp(name, "point")) {
		point = &self->point;
	} else if (!strcmp(name, "normal")) {
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

static PySequenceMethods PlaneDomain_as_sequence = {
	0,		/* sq_length */
	0,		/* sq_concat */
	0,		/* sq_repeat */
	0,	    /* sq_item */
	0,		/* sq_slice */
	0,		/* sq_ass_item */
	0,	    /* sq_ass_slice */
	(objobjproc)Domain_never_contains,	/* sq_contains */
};

PyDoc_STRVAR(PlaneDomain__doc__, 
	"Infinite 2D plane domain\n\n"
	"Plane(point, normal)\n\n"
	"point -- Any point in the plane (3-number sequence)\n"
	"normal -- Normal vector perpendicular to the plane. This need not\n"
	"be a unit vector.");

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
	PyObject *x, *y, *z;
	Vec3 size;

	Vec3_sub(&size, &self->max, &self->min);

	x = PyFloat_FromDouble(self->min.x + size.x * rand_uni());
	if (x == NULL)
		return NULL;
	y = PyFloat_FromDouble(self->min.y + size.y * rand_uni());
	if (y == NULL)
		return NULL;
	z = PyFloat_FromDouble(self->min.z + size.z * rand_uni());
	if (z == NULL)
		return NULL;
	
	return PyTuple_Pack(3, x, y, z);
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

	if (!PyArg_ParseTuple(args, "(fff)(fff):__init__",
		&start.x, &start.y, &start.z,
		&end.x, &end.y, &end.z))
		return NULL;

	int start_in = pt_in_box(self, start.x, start.y, start.z);
	int end_in = pt_in_box(self, end.x, end.y, end.z);
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
	char buf[256];
	snprintf(buf, 256, "AABox.intersect BUG: Intersect face not identified "
		"min=(%f, %f, %f) max=(%f, %f, %f) start=(%f, %f, %f) end=(%f, %f, %f)",
		self->min.x, self->min.y, self->min.z, self->max.x, self->max.y, self->max.z,
		start.x, start.y, start.z, end.x, end.y, end.z);
	PyErr_SetString(PyExc_RuntimeError, buf);
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
		return (PyObject *)Vector_new(NULL, &self->min, 3);	
	} else if (name_str == max_point_str) {
		return (PyObject *)Vector_new(NULL, &self->max, 3);	
	} else {
		return Py_FindMethod(AABoxDomain_methods, 
			(PyObject *)self, PyString_AS_STRING(name_str));
	}
}

static int
AABoxDomain_setattr(AABoxDomainObject *self, PyObject *nameob, PyObject *v)
{
	Vec3 *point;
	int result;
	char *name = PyString_AS_STRING(nameob);

	if (!strcmp(name, "min_point")) {
		point = &self->min;
	} else if (!strcmp(name, "max_point")) {
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

	/* Create the module and add the types */
	m = Py_InitModule3("_domain", NULL, "Spacial domains");
	if (m == NULL)
		return;

	AABoxDomain_Type.tp_alloc = PyType_GenericAlloc;
	AABoxDomain_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&AABoxDomain_Type) < 0)
		return;

	/* initialize singleton marker for no intersection */
	Py_INCREF(Py_None);
	Py_INCREF(Py_None);
	NO_INTERSECTION = PyTuple_Pack(2, Py_None, Py_None);
	if (NO_INTERSECTION == NULL)
		return;
	
	min_point_str = PyString_InternFromString("min_point");
	if (min_point_str == NULL)
		return;
	max_point_str = PyString_InternFromString("max_point");
	if (max_point_str == NULL)
		return;

	Py_INCREF(&LineDomain_Type);
	PyModule_AddObject(m, "Line", (PyObject *)&LineDomain_Type);
	Py_INCREF(&PlaneDomain_Type);
	PyModule_AddObject(m, "Plane", (PyObject *)&PlaneDomain_Type);
	Py_INCREF(&AABoxDomain_Type);
	PyModule_AddObject(m, "AABox", (PyObject *)&AABoxDomain_Type);

	rand_seed(time(NULL));
}
