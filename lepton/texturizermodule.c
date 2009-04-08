/****************************************************************************
*
* Copyright (c) 2009 by Casey Duncan and contributors
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
#include "renderer.h"

static PyTypeObject SpriteTex_Type;

typedef struct {
	PyObject_HEAD
	PyObject *dict;
	GLuint texture;
	GLint tex_filter;
	GLint tex_wrap;
	int coord_count;
	float *tex_coords;
	unsigned long *weights;
	FloatArrayObject *tex_array;
} SpriteTexObject;

static PyObject *
SpriteTex_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    SpriteTexObject *self;

    self = (SpriteTexObject *)type->tp_alloc(type, 0);
    if (self == NULL)
		return NULL;
	self->dict = PyDict_New();
    if (self->dict == NULL) {
		Py_DECREF(self);
		return NULL;
	}
	self->tex_coords = NULL;
	self->weights = NULL;
	self->tex_array = NULL;
	return (PyObject *)self;
}

static int
SpriteTex_clear(SpriteTexObject *self) 
{
	Py_CLEAR(self->dict);
	Py_CLEAR(self->tex_array);
	return 0;
}

static void
SpriteTex_dealloc(SpriteTexObject *self) 
{
	PyMem_Free(self->tex_coords);
	self->tex_coords = NULL;
	PyMem_Free(self->weights);
	self->weights = NULL;
	SpriteTex_clear(self);
	self->ob_type->tp_free((PyObject *)self);
}

static int
SpriteTex_traverse(SpriteTexObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->dict);
    Py_VISIT(self->tex_array);
    return 0;
}

#define WEIGHT_MAX INT_MAX

static int
SpriteTex_init(SpriteTexObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *tex_coords_seq = NULL, *weights_seq = NULL;
	PyObject *s = NULL, *t = NULL, **item;
	float *tex, ignore;
	int tlen, i;
	double total_weight, weight_scale;

	static char *kwlist[] = {"texture", "coords", "weights", "filter", "wrap", NULL};

	PyMem_Free(self->tex_coords);
	self->tex_coords = NULL;
	PyMem_Free(self->weights);
	self->weights = NULL;
	self->tex_filter = GL_LINEAR;
	self->tex_wrap = GL_CLAMP;
	self->coord_count = 0;
	Py_CLEAR(self->tex_array);
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|OOii:__init__", kwlist,
		&self->texture, &tex_coords_seq, &weights_seq, &self->tex_filter, &self->tex_wrap))
		return -1;
	
	if (tex_coords_seq != NULL) {
		s = PySequence_Fast(tex_coords_seq, "SpriteTexturizer: coords not iterable");
		if (s == NULL)
			goto error;
		self->coord_count = PySequence_Fast_GET_SIZE(s);
		if (self->coord_count == 0) {
			PyErr_SetString(PyExc_ValueError, "SpriteTexturizer: coords is empty");
			goto error;
		}
		tex = self->tex_coords = (float *)PyMem_Malloc(sizeof(float) * self->coord_count * 8);
		if (self->tex_coords == NULL) {
			PyErr_NoMemory();
			goto error;
		}
		item = PySequence_Fast_ITEMS(s);
		for (i = 0; i < self->coord_count; i++) {
			t = PySequence_Tuple(item[i]);
			if (t == NULL)
				goto error;
			tlen = PyTuple_GET_SIZE(t);
			if (tlen == 4) {
				if (!PyArg_ParseTuple(t, 
					"(ff)(ff)(ff)(ff);Expected coords element with sequence of 4 float pairs", 
					tex, tex+1, tex+2, tex+3, tex+4, tex+5, tex+6, tex+7))
					goto error;
			} else if (tlen == 8) {
				if (!PyArg_ParseTuple(t, 
					"ffffffff;Expected coords element with sequence of 8 floats",
					tex, tex+1, tex+2, tex+3, tex+4, tex+5, tex+6, tex+7))
					goto error;
			} else if (tlen == 12) {
				if (!PyArg_ParseTuple(t, 
					"ffffffffffff;Expected coords element with sequence of 12 floats",
					tex, tex+1, &ignore, tex+2, tex+3, &ignore, 
					tex+4, tex+5, &ignore, tex+6, tex+7, &ignore))
					goto error;
			} else {
				PyErr_SetString(PyExc_ValueError, 
					"SpriteTexturizer: coords elements must be sequence of "
					"4 float pairs, 8 floats or 12 floats");
					goto error;
			}
			tex += 8;
			Py_CLEAR(t);
		}
		Py_CLEAR(s);
		if (weights_seq != NULL && weights_seq != Py_None) {
			s = PySequence_Fast(weights_seq, "SpriteTexturizer: weights not iterable");
			if (s == NULL)
				goto error;
			if (PySequence_Fast_GET_SIZE(s) != self->coord_count) {
				PyErr_SetString(PyExc_ValueError,
					"SpriteTexturizer: Length of coords and weights do not match");
				goto error;
			}
			self->weights = (unsigned long *)PyMem_Malloc(
				sizeof(unsigned long) * self->coord_count);
			if (self->weights == NULL) {
				PyErr_NoMemory();
				goto error;
			}
			item = PySequence_Fast_ITEMS(s);
			total_weight = 0;
			for (i = 0; i < self->coord_count; i++) {
				total_weight += PyFloat_AsDouble(item[i]);
				if (PyErr_Occurred())
					goto error;
			}
			/* scale weights to full integer range */
			if (total_weight > (double)(WEIGHT_MAX)) {
				PyErr_SetString(PyExc_ValueError, "SpriteTexturizer: Weights out of range");
				goto error;
			}
			weight_scale = (double)(WEIGHT_MAX) / total_weight;
			total_weight = 0;
			for (i = 0; i < self->coord_count; i++) {
				total_weight += PyFloat_AsDouble(item[i]);
				if (PyErr_Occurred())
					goto error;
				 self->weights[i] = (unsigned long)(total_weight * weight_scale);
			}
			Py_CLEAR(s);
		}
	} else if (weights_seq != NULL) {
		PyErr_SetString(PyExc_TypeError,
			"SpriteTexturizer: weights specified without coords");
		goto error;
	}
	return 0;

error:
	Py_XDECREF(s);
	Py_XDECREF(t);
	PyMem_Free(self->tex_coords);
	self->tex_coords = NULL;
	PyMem_Free(self->weights);
	self->weights = NULL;
	return -1;
}

static PyObject *
SpriteTex_get_tex_coords(SpriteTexObject *self, void *closure)
{
	PyObject *coord_sets = NULL, *set = NULL;
	float *tex;
	int i, c;

	tex = self->tex_coords;
	if (tex != NULL) {
		coord_sets = PyTuple_New(self->coord_count);
		if (coord_sets == NULL)
			goto error;
		for (i = 0, c = 0; i < self->coord_count; i++, c+=8) {
			set = Py_BuildValue("(ffffffff)",
				tex[c], tex[c+1], tex[c+2], tex[c+3], tex[c+4], tex[c+5], tex[c+6], tex[c+7]);
			if (set == NULL)
				goto error;
			PyTuple_SET_ITEM(coord_sets, i, set);
		}
		return coord_sets;
	} else {
		Py_INCREF(Py_None);
		return Py_None;
	}
error:
	Py_XDECREF(coord_sets);
	Py_XDECREF(set);
	return NULL;
}

static PyObject *
SpriteTex_get_weights(SpriteTexObject *self, void *closure)
{
	PyObject *weights = NULL, *w = NULL;
	double total = 0.0;
	int i;
	
	if (self->weights != NULL) {
		weights = PyTuple_New(self->coord_count);
		if (weights == NULL)
			goto error;
		for (i = 0; i < self->coord_count; i++) {
			w = PyFloat_FromDouble(((double)self->weights[i] - total) / (double)WEIGHT_MAX);
			if (w == NULL)
				goto error;
			PyTuple_SET_ITEM(weights, i, w);
			total = (double)self->weights[i];
		}
		return weights;
	} else {
		Py_INCREF(Py_None);
		return Py_None;
	}
error:
	Py_XDECREF(weights);
	Py_XDECREF(w);
	return NULL;
}

static PyObject *
SpriteTex_get_tex_dimension(SpriteTexObject *self, void *closure)
{
	return PyInt_FromLong(2);
}

static PyObject *
SpriteTex_set_state(SpriteTexObject *self)
{
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, self->tex_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, self->tex_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, self->tex_wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, self->tex_wrap);
	glBindTexture(GL_TEXTURE_2D, self->texture);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
SpriteTex_restore_state(SpriteTexObject *self)
{
	glPopAttrib();

	Py_INCREF(Py_None);
	return Py_None;
}

static unsigned long jz, jsr=123456789;
#define SHR3SEED(s) (jz=0,jsr=(s))
#define SHR3RAND() (jz=jsr, jsr^=(jsr<<13), jsr^=(jsr>>17), jsr^=(jsr<<5),jz+jsr)

static FloatArrayObject *
SpriteTex_generate_tex_coords(SpriteTexObject *self, GroupObject *pgroup)
{
	register unsigned long pcount, i;
	unsigned long w, *weights;
	int tcount, coord_count;
	float *ptex, *ttex, *tex_coords;

	if (!GroupObject_Check(pgroup)) {
		PyErr_SetString(PyExc_TypeError, "Expected ParticleGroup first argument");
		return NULL;
	}

	if (self->tex_coords == NULL) {
		/* Special case, default texture coordinates. These can be cached
		   and shared between texturizers, so we don't generate them here */
		return generate_default_2D_tex_coords(pgroup);
	}

	if (self->tex_array != NULL) {
		if (self->tex_array->size >= GroupObject_ActiveCount(pgroup) * 8) {
			Py_INCREF(self->tex_array);
			return self->tex_array;
		} else {
			Py_CLEAR(self->tex_array);
		}
	}
	
	pcount = pgroup->plist->palloc;
	self->tex_array = FloatArray_new(pgroup->plist->palloc * 8);
	ptex = self->tex_array->data;
	ttex = tex_coords = self->tex_coords;
	tcount = coord_count = self->coord_count;
	weights = self->weights;
	if (self->coord_count == 1) {
		/* Special case, all particles have same coords */
		while (pcount--) {
			*ptex++ = ttex[0];
			*ptex++ = ttex[1];
			*ptex++ = ttex[2];
			*ptex++ = ttex[3];
			*ptex++ = ttex[4];
			*ptex++ = ttex[5];
			*ptex++ = ttex[6];
			*ptex++ = ttex[7];
		}
	} else if (self->weights == NULL) {
		/* Assign coords in round-robin fashion */
		while (pcount--) {
			tcount -= 1;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
			if (tcount <= 0) {
				ttex = tex_coords;
				tcount = coord_count;
			}
		}
	} else {
		/* Assign coords randomly according to weight */

		/* use our pointer as the random seed so the generated
		   random sequence is repeatable */
		SHR3SEED((unsigned long)self);
		while (pcount--) {
			w = SHR3RAND() & WEIGHT_MAX;
			for (i = 0; i < coord_count && w > weights[i]; i++);
			ttex = tex_coords + i * 8;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
			*ptex++ = *ttex++;
		}
	}

	Py_INCREF(self->tex_array);
	return self->tex_array;
}

static PyMemberDef SpriteTex_members[] = {
	{"__dict__", T_OBJECT, offsetof(SpriteTexObject, dict), READONLY},
	{NULL}
};

static PyMethodDef SpriteTex_methods[] = {
	{"set_state", (PyCFunction)SpriteTex_set_state, METH_NOARGS,
		PyDoc_STR("set_state() -> None\n"
			"Setup the OpenGL texture state for rendering.\n"
			"Called by the renderer before particles are drawn.")},
	{"restore_state", (PyCFunction)SpriteTex_restore_state, METH_NOARGS,
		PyDoc_STR("restore_state() -> None\n"
			"Restore the OpenGL texture state after rendering.\n"
			"Called by the renderer after particles are drawn.")},
	{"generate_tex_coords", (PyCFunction)SpriteTex_generate_tex_coords, METH_O,
		PyDoc_STR("generate_tex_coords(group, coord_array) -> None\n"
			"Generate texture coordinates for the given particle\n"
			"group and store them in the provided coord array\n"
			"Called by the renderer when particles are drawn.")},
	{NULL,		NULL}		/* sentinel */
};

static PyGetSetDef SpriteTex_descriptors[] = {
	{"tex_dimension", (getter)SpriteTex_get_tex_dimension, NULL, 
		"The number of dimensions per texture coordinate", NULL},
	{"tex_coords", (getter)SpriteTex_get_tex_coords, NULL, 
		"The sequence of texture coordinate sets", NULL},
	{"weights", (getter)SpriteTex_get_weights, NULL, 
		"The sequence of texture coordinate set weights or None", NULL},
	{NULL}
};

PyDoc_STRVAR(SpriteTex__doc__, 
	"Applies a set of static texture coordinates from a single resident\n"
	"texture to a particle group\n\n"
	"SpriteTexturizer(texture, coords=(), weights=(), filter=GL_LINEAR, wrap=GL_CLAMP)\n\n"
	"texture -- OpenGL texture name, acquired via glGenTextures. It is up\n"
	"to the application to load the texture's data before using the texturizer\n\n"
	"coords -- A sequence of texture coordinate sets. Each set consists of coordinates\n"
	"for the four corners of the quad drawn for a particle (8 floats). Sets may\n"
	"consist of 4 coodinate pairs (tuples) or simply 8 floats corresponding to the\n"
	"bottom left, bottom right, top right and top left texture coordinates respectively.\n"
	"if omitted, coords defaults to a single set of coordinates: (0,0, 1,0, 1,1, 0,1)\n\n"
	"weights -- An optional list of weight values applied to the coordinate sets\n"
	"specified in coords. Must have the same length as the coords sequence.\n"
	"If specified, the coordinate set is randomly chosen for each particle in\n"
	"proportion to its weight. If not specified, the the coordinates are\n"
	"assigned evenly to the particles\n\n"
	"filter -- The OpenGL filter used to scale the texture when rendering.\n"
	"One of: GL_NEAREST, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, etc.\n\n"
	"wrap -- The OpenGL wrapping parameter for texture application when\n"
	"rendering. One of: GL_CLAMP, GL_REPEAT, GL_CLAMP_TO_EDGE, etc.");

static PyTypeObject SpriteTex_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"texturizer.SpriteTexturizer",		/*tp_name*/
	sizeof(SpriteTexObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)SpriteTex_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,          /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	0, /*tp_getattro*/
	0, /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
	SpriteTex__doc__,       /*tp_doc*/
	(traverseproc)SpriteTex_traverse,  /*tp_traverse*/
	(inquiry)SpriteTex_clear,          /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	SpriteTex_methods,      /*tp_methods*/
	SpriteTex_members,      /*tp_members*/
	SpriteTex_descriptors,  /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	offsetof(SpriteTexObject, dict), /*tp_dictoffset*/
	(initproc)SpriteTex_init, /*tp_init*/
	0,                      /*tp_alloc*/
	SpriteTex_new,          /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

PyMODINIT_FUNC
init_texturizer(void)
{
	PyObject *m;

	/* Bind tp_new and tp_alloc here to appease certain compilers */
	if (PyType_Ready(&SpriteTex_Type) < 0)
		return;

	/* Create the module and add the types */
	m = Py_InitModule3("_texturizer", NULL, "Particle renderer texturizers");
	if (m == NULL)
		return;

	Py_INCREF(&SpriteTex_Type);
	PyModule_AddObject(m, "SpriteTexturizer", (PyObject *)&SpriteTex_Type);
}
