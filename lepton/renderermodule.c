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
/* Native-code renderers
 *
 * $Id $ */

#include <Python.h>
#include <structmember.h>
#include <math.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "vector.h"
#include "group.h"

typedef struct {
	PyObject_HEAD
	GroupObject	*pgroup;
} RendererObject;

/* Base renderer methods */

static PyObject *
Renderer_set_group(RendererObject *self, GroupObject *new_group)
{
	PyObject *r;

	if (self->pgroup != NULL && (PyObject *)self->pgroup != Py_None) {
		/* call self.group.set_renderer(None) */
		r = PyObject_CallMethod((PyObject *)self->pgroup, "set_renderer", "O", Py_None);
		Py_XDECREF(r);
		if (r == NULL || PyErr_Occurred())
			return NULL;
	}

	Py_INCREF(new_group);
	Py_XDECREF(self->pgroup);
	self->pgroup = new_group;

	Py_INCREF(Py_None);
	return Py_None;
}

static void
Renderer_dealloc(RendererObject *self) {
	Py_CLEAR(self->pgroup);
	PyObject_Del(self);
}
	
/* --------------------------------------------------------------------- */

static PyTypeObject PointRenderer_Type;

typedef struct {
	PyObject_HEAD
	GroupObject	*pgroup;
	float		 point_size;
} PointRendererObject;

static int
PointRenderer_init(PointRendererObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"point_size", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "f:__init__",kwlist, 
		&self->point_size))
		return -1;
	self->pgroup = NULL;
	return 0;
}

static PyObject *
PointRenderer_draw(PointRendererObject *self)
{
	Particle *p;
	int GL_error;
	unsigned long count_particles;

	if (self->pgroup == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "cannot draw, no group set");
		return NULL;
	}

	count_particles = GroupObject_ActiveCount(self->pgroup);
	if (count_particles > 0){
		p = self->pgroup->plist->p;
		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glPointSize(self->point_size);
		glVertexPointer(3, GL_FLOAT, sizeof(Particle), &p[0].position);
		glColorPointer(4, GL_FLOAT, sizeof(Particle), &p[0].color);
		glDrawArrays(GL_POINTS, 0, GroupObject_ActiveCount(self->pgroup));
		glPopClientAttrib();

		GL_error = glGetError();
		if (GL_error != GL_NO_ERROR) {
			PyErr_Format(PyExc_RuntimeError, "GL error %d", GL_error);
			return NULL;
		}
	}	
	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMemberDef PointRenderer_members[] = {
    {"point_size", T_FLOAT, offsetof(PointRendererObject, point_size), RESTRICTED,
        "Size of GL_POINTS drawn"},
    {"group", T_OBJECT, offsetof(PointRendererObject, pgroup), RO,
        "Particle group this renderer is bound to"},
	{NULL}
};

static PyMethodDef PointRenderer_methods[] = {
	{"set_group", (PyCFunction)Renderer_set_group, METH_O,
		PyDoc_STR("set_group(group) -> None\n"
			"Sets the particle group for this renderer")},
	{"draw", (PyCFunction)PointRenderer_draw, METH_NOARGS,
		PyDoc_STR("Draw the particles using points")},
	{NULL,		NULL}		/* sentinel */
};

PyDoc_STRVAR(PointRenderer__doc__, 
	"Simple particle renderer using GL_POINTS. All particles in the\n"
	"group are rendered with the same point size\n\n"
	"PointRenderer(point_size)\n\n"
	"point_size -- Size of GL points (float)");

static PyTypeObject PointRenderer_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"renderer.PointRenderer",		/*tp_name*/
	sizeof(PointRendererObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)Renderer_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	0,                      /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	PointRenderer__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	PointRenderer_methods,  /*tp_methods*/
	PointRenderer_members,  /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)PointRenderer_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject BillboardRenderer_Type;

static int
BillboardRenderer_init(RendererObject *self, PyObject *args)
{
	self->pgroup = NULL;
	return 0;
}

typedef struct {
	float tex[2];
	union {
		unsigned char color[4];
		unsigned long colorl;
	};
	struct {
		float x; float y; float z;
	} vert;
} BBData;

#define BILLBOARD_BATCH_SIZE 5000

static unsigned short billboard_indices[BILLBOARD_BATCH_SIZE*6];

static void
init_billboard_indices(void) {
	int i;
	unsigned short v = 0;

	for (i = 0; i < BILLBOARD_BATCH_SIZE*6; i += 6) {
		billboard_indices[i] = v;
		billboard_indices[i+1] = v+1;
		billboard_indices[i+2] = v+3;
		billboard_indices[i+3] = v+1;
		billboard_indices[i+4] = v+2;
		billboard_indices[i+5] = v+3;
		v += 4;
	}
}

static PyObject *
BillboardRenderer_draw(RendererObject *self)
{
	Particle *p;
	int GL_error;
	unsigned long remaining, batch_size;
	BBData data[BILLBOARD_BATCH_SIZE*4];
	register int i;
	float mvmatrix[16], rotcos, rotsin;
	Vec3 vright, vright_unit, vup, vup_unit, vrot;

	if (self->pgroup == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "cannot draw, no group set");
		return NULL;
	}

	p = self->pgroup->plist->p;
	remaining = GroupObject_ActiveCount(self->pgroup);

	/* Get the alignment vectors from the view matrix */
	glGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix);
	vright_unit.x = mvmatrix[0];
	vright_unit.y = mvmatrix[4];
	vright_unit.z = mvmatrix[8];
	Vec3_normalize(&vright_unit, &vright_unit);
	vup_unit.x = mvmatrix[1];
	vup_unit.y = mvmatrix[5];
	vup_unit.z = mvmatrix[9];
	Vec3_normalize(&vup_unit, &vup_unit);

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	while (remaining) {
		if (remaining > BILLBOARD_BATCH_SIZE) {
			batch_size = BILLBOARD_BATCH_SIZE;
			remaining -= BILLBOARD_BATCH_SIZE;
		} else {
			batch_size = remaining;
			remaining = 0;
		}

		for (i = 0; i < batch_size*4; i += 4) {

#define POINT0 i
#define POINT1 i+1
#define POINT2 i+2
#define POINT3 i+3

			/*

			POINT3                POINT2
				   +-------------+
				   |\            |
				   |  \          |
				   |    \        |
				   |      + ---- | --- Particle position
				   |        \    |
				   |          \  |
				   |            \|
				   +-------------+
			POINT0                POINT1

			*/
			

			/* vertex coords */

			if (p->up.z) {
				/* billboard supports only z-axis rotation
				   where the z-axiz is always that of the
				   model-view matrix
				*/
				rotsin = sin(p->up.z);
				rotcos = cos(p->up.z);
				Vec3_scalar_mul(&vright, &vright_unit, rotcos);
				Vec3_scalar_mul(&vrot, &vup_unit, rotsin);
				Vec3_addi(&vright, &vrot);
				Vec3_scalar_mul(&vup, &vup_unit, rotcos);
				Vec3_scalar_mul(&vrot, &vright_unit, rotsin);
				Vec3_subi(&vup, &vrot);
				Vec3_scalar_muli(&vright, p->size.x * 0.5);
				Vec3_scalar_muli(&vup, p->size.y * 0.5);
			} else {
				Vec3_scalar_mul(&vright, &vright_unit, p->size.x * 0.5);
				Vec3_scalar_mul(&vup, &vup_unit, p->size.y * 0.5);
			}

			Vec3_sub(&data[POINT0].vert, &p->position, &vright);
			Vec3_subi(&data[POINT0].vert, &vup);
			Vec3_add(&data[POINT1].vert, &p->position, &vright);
			Vec3_subi(&data[POINT1].vert, &vup);
			Vec3_add(&data[POINT2].vert, &p->position, &vright);
			Vec3_addi(&data[POINT2].vert, &vup);
			Vec3_sub(&data[POINT3].vert, &p->position, &vright);
			Vec3_addi(&data[POINT3].vert, &vup);

			/* colors */
			data[POINT0].color[0] = p->color.r * 255;
			data[POINT0].color[1] = p->color.b * 255;
			data[POINT0].color[2] = p->color.g * 255;
			data[POINT0].color[3] = p->color.a * 255;
			data[POINT1].colorl = data[POINT0].colorl;
			data[POINT2].colorl = data[POINT0].colorl;
			data[POINT3].colorl = data[POINT0].colorl;

			/* texture coords */
			data[POINT0].tex[0] = 0.0f;
			data[POINT0].tex[1] = 0.0f;
			data[POINT1].tex[0] = 1.0f;
			data[POINT1].tex[1] = 0.0f;
			data[POINT2].tex[0] = 1.0f;
			data[POINT2].tex[1] = 1.0f;
			data[POINT3].tex[0] = 0.0f;
			data[POINT3].tex[1] = 1.0f;

			p++;
		}

		glInterleavedArrays(GL_T2F_C4UB_V3F, 0, data);
		glDrawElements(GL_TRIANGLES, batch_size*6, GL_UNSIGNED_SHORT, billboard_indices);
	}
	glPopClientAttrib();

	GL_error = glGetError();
	if (GL_error != GL_NO_ERROR) {
		PyErr_Format(PyExc_RuntimeError, "GL error %d", GL_error);
		return NULL;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMemberDef BillboardRenderer_members[] = {
    {"group", T_OBJECT, offsetof(RendererObject, pgroup), RO,
        "Particle group this renderer is bound to"},
	{NULL}
};

static PyMethodDef BillboardRenderer_methods[] = {
	{"set_group", (PyCFunction)Renderer_set_group, METH_O,
		PyDoc_STR("set_group(group) -> None\n"
			"Sets the particle group for this renderer")},
	{"draw", (PyCFunction)BillboardRenderer_draw, METH_NOARGS,
		PyDoc_STR("Draw the particles using textured billboard quads")},
	{NULL,		NULL}		/* sentinel */
};

PyDoc_STRVAR(BillboardRenderer__doc__, 
	"Particle renderer using textured billboard-aligned quads\n"
	"quads are aligned orthogonal to the model-view matrix\n\n"
	"BillboardRenderer()\n");

static PyTypeObject BillboardRenderer_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"renderer.BillboardRenderer",		/*tp_name*/
	sizeof(RendererObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)Renderer_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	0,                      /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	BillboardRenderer__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	BillboardRenderer_methods,  /*tp_methods*/
	BillboardRenderer_members,  /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)BillboardRenderer_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

PyMODINIT_FUNC
initrenderer(void)
{
	PyObject *m;

	/* Bind external consts here to appease certain compilers */
	PointRenderer_Type.tp_alloc = PyType_GenericAlloc;
	PointRenderer_Type.tp_new = PyType_GenericNew;
	PointRenderer_Type.tp_getattro = PyObject_GenericGetAttr;
	if (PyType_Ready(&PointRenderer_Type) < 0)
		return;

	BillboardRenderer_Type.tp_alloc = PyType_GenericAlloc;
	BillboardRenderer_Type.tp_new = PyType_GenericNew;
	BillboardRenderer_Type.tp_getattro = PyObject_GenericGetAttr;
	if (PyType_Ready(&BillboardRenderer_Type) < 0)
		return;

	/* Create the module and add the types */
	m = Py_InitModule3("renderer", NULL, "Particle Renderers");
	if (m == NULL)
		return;

	Py_INCREF(&PointRenderer_Type);
	PyModule_AddObject(m, "PointRenderer", (PyObject *)&PointRenderer_Type);
	Py_INCREF(&BillboardRenderer_Type);
	PyModule_AddObject(m, "BillboardRenderer", (PyObject *)&BillboardRenderer_Type);
	init_billboard_indices();
}
