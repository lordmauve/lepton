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
/* Low-level particle group functions
 *
 * $Id$
 */

#include <Python.h>
#include <float.h>
#include "group.h"

/* Return an index for a new particle in the group, allocating space for it if
 * necessary.
 */
long
Group_new_p(GroupObject *group) {
	long pindex;
	long expansion;
	ParticleList *realloc_plist;

	pindex = group->plist->pactive + group->plist->pkilled + group->plist->pnew;
	if (pindex >= group->plist->palloc) {
		expansion = group->plist->palloc / 5;
		if (expansion < GROUP_MIN_ALLOC)
			expansion = GROUP_MIN_ALLOC;
		realloc_plist = (ParticleList *)PyMem_Realloc(group->plist,
			sizeof(ParticleList) + 
			sizeof(Particle) * (group->plist->palloc + expansion));
		if (realloc_plist == NULL) {
			return -1;
		}
		group->plist = realloc_plist;
		group->plist->palloc += expansion;
	}
	group->plist->pnew++;
	return pindex;
}

/* Kill the particle specified.
 */
void inline
Group_kill_p(GroupObject *group, Particle *p) {
	Particle *lastactive;
	lastactive = &group->plist->p[GroupObject_ActiveCount(group)];	
	if (Particle_IsAlive(*p) && p < lastactive) {
		group->plist->pactive--;
		group->plist->pkilled++;
	}
	p->age = -FLT_MAX;
	p->position.z = FLT_MAX;
}

static PyTypeObject *GroupObject_Type = NULL;

/* Return true if o is a bon-a-fide GroupObject */
int
GroupObject_Check(GroupObject *o)
{
	PyObject *m;
	if (GroupObject_Type == NULL) {
		m = PyImport_ImportModule("lepton.group");
		if (m == NULL)
			return 0;
		GroupObject_Type = (PyTypeObject *)PyObject_GetAttrString(m, "ParticleGroup");
		Py_DECREF(m);
	}
	if (o->ob_type != GroupObject_Type) {
		PyErr_SetString(PyExc_TypeError, "Expected ParticleGroup object");
		return 0;
	}
	return 1;
}

/* Get a vector from an attrbute of the template and store it in vec */
int
get_Vec3(Vec3 *vec, PyObject *template, const char *attrname)
{
	int result = 0;
	PyObject *attr = NULL;
	PyObject *tuple = NULL;
	attr = PyObject_GetAttrString(template, attrname);
	if (attr != NULL) {
		tuple = PySequence_Tuple(attr);
		if (tuple != NULL)
			result = PyArg_ParseTuple(tuple, "fff; 3 floats expected", 
				&vec->x, &vec->y, &vec->z);
	} else {
		PyErr_Clear();
		vec->x = 0; vec->y = 0; vec->z = 0;
		result = 1;
	}
	Py_XDECREF(attr);
	Py_XDECREF(tuple);
	return result;
}

/* Get a color from an attrbute of the template and store it in vec */
int
get_Color(Color *color, PyObject *template, const char *attrname)
{
	int result = 0;
	PyObject *attr = NULL;
	PyObject *tuple = NULL;
	attr = PyObject_GetAttrString(template, attrname);
	if (attr != NULL) {
		tuple = PySequence_Tuple(attr);
		if (tuple != NULL) {
			color->a = 1; /* default */
			result = PyArg_ParseTuple(tuple, "fff|f; 3 or 4 floats expected", 
				&color->r, &color->g, &color->b, &color->a);
		}
	} else {
		PyErr_Clear();
		color->r = 0; color->g = 0; color->b = 0; color->a = 0;
		result = 1;
	}
	Py_XDECREF(attr);
	Py_XDECREF(tuple);
	return result;
}

/* Get a number from an attrbute of the template and store it in f */
int
get_Float(float *f, PyObject *template, const char *attrname)
{
	int result = 0;
	PyObject *attr = NULL;
	PyObject *pyfloat = NULL;
	attr = PyObject_GetAttrString(template, attrname);
	if (attr != NULL) {
		pyfloat = PyNumber_Float(attr);
		if (pyfloat != NULL) {
			*f = PyFloat_AS_DOUBLE(pyfloat);
			result = 1;
		}
	} else {
		PyErr_Clear();
		f = 0;
		result = 1;
	}
	Py_XDECREF(attr);
	Py_XDECREF(pyfloat);
	return result;
}

