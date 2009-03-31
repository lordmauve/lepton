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

#ifdef _WIN32
#include <windows.h>
#endif

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
	PyObject *texturizer;
} RendererObject;

/* Base renderer methods */

static void
Renderer_dealloc(RendererObject *self) {
	PyObject_Del(self);
}

/* --------------------------------------------------------------------- */

/* Vertex data functions and structs */

typedef struct {
	float x, y, z;
} VertItem;

typedef struct {
	unsigned char r, g, b, a;
} ColorSwizzle;

typedef struct {
	union {
		ColorSwizzle rgba;
		unsigned long colorl;
	};
} ColorItem;

typedef struct {
	int is_vbo;
	Py_ssize_t size; /* Number of verts */
	VertItem *verts;
	ColorItem *colors;
	float *tex_coords;
} VertArray;

/* Allocate space for vertex data for the given particle group
   and texture dimension. Store the results in data.

   Return 1 on success, 0 on failure
*/
static int
VertArray_alloc(GroupObject *pgroup, int tex_dimension, VertArray *data)
{
	data->size = GroupObject_ActiveCount(pgroup) * 4;
	data->is_vbo = 0;
	data->verts = (VertItem *)PyMem_Malloc(
		data->size*sizeof(VertItem) + /* vert data */
		data->size*sizeof(ColorItem) + /* color data */
		data->size*sizeof(float)*tex_dimension /* tex coord data */
		);
	if (data->verts != NULL) {
		data->colors = (ColorItem *)(data->verts + data->size);
		data->tex_coords = (float *)(data->colors + data->size);
		return 1;
	} else {
		PyErr_NoMemory();
		return 0;
	}
}

static void
VertArray_free(VertArray *data)
{
	if (!data->is_vbo)
		PyMem_Free((void *)data->verts);
}
	
/* --------------------------------------------------------------------- */

/* A float array provides a simple interface from python to
   an arbitrary fixed-sized array of C floats. This underlying
   array data itself is not owned by this object, it is managed
   outside by other parts of the system, or the graphics driver
   itself. This prevents us from using an existing array type
*/
static PyTypeObject FloatArray_Type;

typedef struct {
	PyObject_HEAD
	Py_ssize_t size;
	float *data;
} FloatArrayObject;

static FloatArrayObject *
FloatArray_new(Py_ssize_t size, float *data)
{
	FloatArrayObject *floatarray;
	floatarray = PyObject_New(FloatArrayObject, &FloatArray_Type);
	if (floatarray == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	floatarray->size = size;
	floatarray->data = data;
	return floatarray;
}

static PyObject *
FloatArray_getitem(FloatArrayObject *self, Py_ssize_t index)
{
	//if (index < 0)
	//	index += self->size;
	if (index >= 0 && index < self->size)
		return PyFloat_FromDouble((double)self->data[index]);
	PyErr_Format(PyExc_IndexError, "%d", (int)index);
	return NULL;
}

static int
FloatArray_assitem(FloatArrayObject *self, Py_ssize_t index, PyObject *v)
{
	float f;
	if (index >= 0 && index < self->size) {
		 f = (float)PyFloat_AsDouble(v);
		 if (PyErr_Occurred() != NULL)
		 	return -1;
		 self->data[index] = f;
		 return 0;
	}
	PyErr_Format(PyExc_IndexError, "%d", (int)index);
	return -1;
}

static Py_ssize_t
FloatArray_length(FloatArrayObject *self)
{
	return (Py_ssize_t)self->size;
}

/* Clear the float array length and data when the underlying data is
 * deallocated */
static void
FloatArray_clear(FloatArrayObject *self)
{
	if (self != NULL) {
		self->size = 0;
		self->data = NULL;
	}
}

static PySequenceMethods FloatArray_as_sequence = {
	(lenfunc)FloatArray_length,	/* sq_length */
	0,		/*sq_concat*/
	0,		/*sq_repeat*/
	(ssizeargfunc)FloatArray_getitem,		/*sq_item*/
	0,		/* sq_slice */
	(ssizeobjargproc)FloatArray_assitem,	/* sq_ass_item */
};

PyDoc_STRVAR(FloatArray__doc__, "Fixed length float array");

static PyTypeObject FloatArray_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			                /*ob_size*/
	"renderer.FloatArray",		/*tp_name*/
	sizeof(FloatArrayObject),	/*tp_basicsize*/
	0,			                /*tp_itemsize*/
	/* methods */
	(destructor)Renderer_dealloc, /*tp_dealloc*/
	0,			            /*tp_print*/
	0,                      /*tp_getattr*/
	0,                      /*tp_setattr*/
	0,			            /*tp_compare*/
	0,                      /*tp_repr*/
	0,			            /*tp_as_number*/
	&FloatArray_as_sequence,    /*tp_as_sequence*/
	0,			            /*tp_as_mapping*/
	0,			            /*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	0,                      /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	FloatArray__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	0,                      /*tp_methods*/
	0,                      /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	0, /*(initproc)FloatArray_init,*/ /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject PointRenderer_Type;

typedef struct {
	PyObject_HEAD
	float		 point_size;
} PointRendererObject;

static int
PointRenderer_init(PointRendererObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"point_size", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "f:__init__",kwlist, 
		&self->point_size))
		return -1;
	return 0;
}

static PyObject *
PointRenderer_draw(PointRendererObject *self, GroupObject *pgroup)
{
	Particle *p;
	int GL_error;
	unsigned long count_particles;

	if (!GroupObject_Check(pgroup)) {
		PyErr_SetString(PyExc_TypeError, "Expected ParticleGroup first argument");
		return NULL;
	}

	count_particles = GroupObject_ActiveCount(pgroup);
	if (count_particles > 0){
		p = pgroup->plist->p;
		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glPointSize(self->point_size);
		glVertexPointer(3, GL_FLOAT, sizeof(Particle), &p[0].position);
		glColorPointer(4, GL_FLOAT, sizeof(Particle), &p[0].color);
		glDrawArrays(GL_POINTS, 0, GroupObject_ActiveCount(pgroup));
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
    {"point_size", T_FLOAT, offsetof(PointRendererObject, point_size), 0,
        "Size of GL_POINTS drawn"},
	{NULL}
};

static PyMethodDef PointRenderer_methods[] = {
	{"draw", (PyCFunction)PointRenderer_draw, METH_O,
		PyDoc_STR("Draw the particles in the specified group using GL_POINTS")},
	{NULL,		NULL}		/* sentinel */
};

PyDoc_STRVAR(PointRenderer__doc__, 
	"Simple particle renderer using GL_POINTS. All particles in the\n"
	"group are rendered with the same point size\n\n"
	"PointRenderer(point_size)\n\n"
	"point_size -- Size of GL_POINTS points (float)");

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
BillboardRenderer_init(RendererObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"point_size", NULL};

	self->texturizer = NULL;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:__init__", kwlist, 
		&self->texturizer))
		return -1;
	if (self->texturizer == Py_None)
		self->texturizer = NULL; /* Avoid having to test for NULL and None */
	if (self->texturizer != NULL)
		Py_INCREF(self->texturizer);
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

#define MIN_SHORT_INDEX_COUNT 4096
#define MAX_SHORT_INDEX_COUNT 65536

static int
draw_billboards(unsigned long count) {
	int i;
	unsigned short sv = 0;
	static unsigned short *short_indices = NULL;
 	static size_t short_alloc = 0;
	unsigned long index_count;

	index_count = count * 6;
	if (index_count <= MAX_SHORT_INDEX_COUNT) {
		if (index_count > short_alloc) {
			if (short_alloc < MIN_SHORT_INDEX_COUNT)
				short_alloc = MIN_SHORT_INDEX_COUNT;
			while (index_count > short_alloc)
				short_alloc *= 2;
			if (short_indices != NULL)
				PyMem_Free(short_indices);
			short_indices = PyMem_Malloc(short_alloc * sizeof(unsigned short));
			if (short_indices == NULL) {
				PyErr_NoMemory();
				return 0;
			}

			for (i = 0; i <= (short_alloc - 6); i += 6) {
				short_indices[i] = sv;
				short_indices[i+1] = sv+1;
				short_indices[i+2] = sv+3;
				short_indices[i+3] = sv+1;
				short_indices[i+4] = sv+2;
				short_indices[i+5] = sv+3;
				sv += 4;
			}
		}
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, short_indices);
	} else {
		/* Too many verts to use an index array of short ints. Instead just
		   draw using GL_QUADS directly and let the driver do the dirty work.
		   Alternately we could use full int indices, but I'd prefer to save
		   the code complexity, bandwidth and memory in this case */
		glDrawArrays(GL_QUADS, 0, count * 4);
	}
	return 1;
}

static PyObject *
BillboardRenderer_draw(RendererObject *self, GroupObject *pgroup)
{
	Particle *p;
	int GL_error;
	unsigned int pcount;
	register unsigned int i;
	float mvmatrix[16], rotcos, rotsin;
	Vec3 vright, vright_unit, vup, vup_unit, vrot;
	long tex_dimension;
	PyObject *r;
	FloatArrayObject *texarray = NULL;
	VertArray data;
	float *tex;

	if (!GroupObject_Check(pgroup)) {
		PyErr_SetString(PyExc_TypeError, "Expected ParticleGroup first argument");
		return NULL;
	}

	p = pgroup->plist->p;
	pcount = GroupObject_ActiveCount(pgroup);
	if (pcount == 0) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	if (self->texturizer != NULL) {
		r = PyObject_GetAttrString(self->texturizer, "tex_dimension");
		if (r == NULL)
			return NULL;
		tex_dimension = PyInt_AsLong(r);
		Py_DECREF(r);
		if (PyErr_Occurred() != NULL)
			return NULL;
		if (tex_dimension != 2 && tex_dimension != 3) {
			PyErr_Format(PyExc_ValueError, 
				"Expected texturizer.tex_dimension value of 2 or 3, got %ld", tex_dimension);
			return NULL;
		}
		if (!VertArray_alloc(pgroup, tex_dimension, &data))
			return NULL;
		texarray = FloatArray_new(data.size * tex_dimension, data.tex_coords);
		if (texarray == NULL)
			goto error;
		r = PyObject_CallMethod(self->texturizer, "set_state", NULL);
		if (r == NULL)
			goto error;
		Py_DECREF(r);
	} else {
		tex_dimension = 2;
		if (!VertArray_alloc(pgroup, tex_dimension, &data))
			return NULL;
	}

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
	for (i = 0; i < pcount*4; i += 4) {

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
			rotsin = (float)sin(p->up.z);
			rotcos = (float)cos(p->up.z);
			Vec3_scalar_mul(&vright, &vright_unit, rotcos);
			Vec3_scalar_mul(&vrot, &vup_unit, rotsin);
			Vec3_addi(&vright, &vrot);
			Vec3_scalar_mul(&vup, &vup_unit, rotcos);
			Vec3_scalar_mul(&vrot, &vright_unit, rotsin);
			Vec3_subi(&vup, &vrot);
			Vec3_scalar_muli(&vright, p->size.x * 0.5f);
			Vec3_scalar_muli(&vup, p->size.y * 0.5f);
		} else {
			Vec3_scalar_mul(&vright, &vright_unit, p->size.x * 0.5f);
			Vec3_scalar_mul(&vup, &vup_unit, p->size.y * 0.5f);
		}

		Vec3_sub(&data.verts[POINT0], &p->position, &vright);
		Vec3_subi(&data.verts[POINT0], &vup);
		Vec3_add(&data.verts[POINT1], &p->position, &vright);
		Vec3_subi(&data.verts[POINT1], &vup);
		Vec3_add(&data.verts[POINT2], &p->position, &vright);
		Vec3_addi(&data.verts[POINT2], &vup);
		Vec3_sub(&data.verts[POINT3], &p->position, &vright);
		Vec3_addi(&data.verts[POINT3], &vup);

		/* colors */
		data.colors[POINT0].rgba.r = (unsigned char)(p->color.r * 255);
		data.colors[POINT0].rgba.g = (unsigned char)(p->color.g * 255);
		data.colors[POINT0].rgba.b = (unsigned char)(p->color.b * 255);
		data.colors[POINT0].rgba.a = (unsigned char)(p->color.a * 255);
		data.colors[POINT1].colorl = data.colors[POINT0].colorl;
		data.colors[POINT2].colorl = data.colors[POINT0].colorl;
		data.colors[POINT3].colorl = data.colors[POINT0].colorl;

		p++;
	}

	if (self->texturizer != NULL) {
		r = PyObject_CallMethod(
			self->texturizer, "generate_tex_coords", "OO", pgroup, texarray);
		if (r == NULL) {
			r = PyObject_CallMethod(self->texturizer, "restore_state", NULL);
			Py_XDECREF(r);
			goto error;
		}
	} else {
		/* No texturizer, map full texture to particles */
		tex = data.tex_coords;
		for (i = 0; i < pcount*4; i += 4) {
			*tex++ = 0.0f;
			*tex++ = 0.0f;
			*tex++ = 1.0f;
			*tex++ = 0.0f;
			*tex++ = 1.0f;
			*tex++ = 1.0f;
			*tex++ = 0.0f;
			*tex++ = 1.0f;
		}
	}

	glVertexPointer(3, GL_FLOAT, 0, data.verts);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, data.colors);
	glTexCoordPointer(tex_dimension, GL_FLOAT, 0, data.tex_coords);
	if (!draw_billboards(pcount))
		goto error;
	glPopClientAttrib();

	GL_error = glGetError();
	if (GL_error != GL_NO_ERROR) {
		PyErr_Format(PyExc_RuntimeError, "GL error %d", GL_error);
		goto error;
	}

	if (self->texturizer != NULL) {
		r = PyObject_CallMethod(self->texturizer, "restore_state", NULL);
		if (r == NULL)
			goto error;
		Py_DECREF(r);
	}
	
	FloatArray_clear(texarray);
	Py_XDECREF(texarray);
	VertArray_free(&data);

	Py_INCREF(Py_None);
	return Py_None;
error:
	FloatArray_clear(texarray);
	Py_XDECREF(texarray);
	VertArray_free(&data);
	return NULL;
}

static PyMethodDef BillboardRenderer_methods[] = {
	{"draw", (PyCFunction)BillboardRenderer_draw, METH_O,
		PyDoc_STR("Draw the particles using textured billboard quads")},
	{NULL,		NULL}		/* sentinel */
};

static struct PyMemberDef BillboardRenderer_members[] = {
    {"texturizer", T_OBJECT, offsetof(RendererObject, texturizer), 0,
        "A texturizer object that generates texture coordinates\n"
		"for the particles and sets up texture state for the renderer."},
	{NULL}
};

PyDoc_STRVAR(BillboardRenderer__doc__, 
	"Particle renderer using textured billboard-aligned quads\n"
	"quads are aligned orthogonal to the model-view matrix\n\n"
	"BillboardRenderer(texturizer=None)\n\n"
	"texturizer -- A texturizer object that generates texture\n"
	"coordinates for the particles and sets up texture state.\n"
	"If not specified, texture coordinates are fixed at (0,0)\n"
	"for the lower-left corner of each particle quad and (1,1)\n"
	"for the upper-right. Without a texturizer the application\n"
	"is responsible for setting up the desired texture state\n"
	"before invoking the renderer.");

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
}
