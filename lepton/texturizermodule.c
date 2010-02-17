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
/* Renderer texturizers
 *
 * $Id$ */

#include <Python.h>
#include <structmember.h>
#include <math.h>

#include <GL/glew.h>
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

static void
adjust_particle_widths(GroupObject *pgroup, FloatArrayObject *tex_array)
{
	Particle *p;
	float *tex, min_s, min_t, max_s, max_t, t_width, t_height;
	int i, j, t;

	p = pgroup->plist->p;
	tex = tex_array->data;
	for (i = 0, t = 0; i < GroupObject_ActiveCount(pgroup); i++, t += 8) {
		min_s = max_s = tex[t];
		min_t = max_t = tex[t+1];
		for (j = 2; j < 8; j += 2) {
			min_s = min_s <= tex[t+j] ? min_s : tex[t+j];
			max_s = max_s >= tex[t+j] ? max_s : tex[t+j];
			min_t = min_t <= tex[t+j+1] ? min_t : tex[t+j+1];
			max_t = max_t >= tex[t+j+1] ? max_t : tex[t+j+1];
		}
		t_width = max_s - min_s;
		t_height = max_t - min_t + EPSILON;
		p[i].size.x = p[i].size.y * t_width / t_height;
	}
}
		
static void
adjust_particle_heights(GroupObject *pgroup, FloatArrayObject *tex_array)
{
	Particle *p;
	float *tex, min_s, min_t, max_s, max_t, t_width, t_height;
	int i, j, t;

	p = pgroup->plist->p;
	tex = tex_array->data;
	for (i = 0, t = 0; i < GroupObject_ActiveCount(pgroup); i++, t += 8) {
		min_s = max_s = tex[t];
		min_t = max_t = tex[t+1];
		for (j = 2; j < 8; j += 2) {
			min_s = min_s <= tex[t+j] ? min_s : tex[t+j];
			max_s = max_s >= tex[t+j] ? max_s : tex[t+j];
			min_t = min_t <= tex[t+j+1] ? min_t : tex[t+j+1];
			max_t = max_t >= tex[t+j+1] ? max_t : tex[t+j+1];
		}
		t_width = max_s - min_s + EPSILON;
		t_height = max_t - min_t;
		p[i].size.y = p[i].size.x * t_height / t_width;
	}
}

/* --------------------------------------------------------------------- */

static PyTypeObject SpriteTex_Type;

typedef struct {
	PyObject_HEAD
	PyObject *dict;
	GLuint texture;
	GLint tex_filter;
	GLint tex_wrap;
	int adjust_width;
	int adjust_height;
	Py_ssize_t coord_count;
	float *tex_coords;
	FloatArrayObject *tex_array;
	unsigned long *weights;
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

static float *
get_tex_coords_2d(PyObject *tex_coords_seq, Py_ssize_t *count_out)
{
	PyObject *s = NULL, *t = NULL, **item;
	float ignore, *tex, *tex_coords = NULL;
	Py_ssize_t i, count, tlen;

	s = PySequence_Fast(tex_coords_seq, "coords not iterable");
	if (s == NULL)
		goto error;
	count = PySequence_Fast_GET_SIZE(s);
	if (count == 0) {
		PyErr_SetString(PyExc_ValueError, "coords is empty");
		goto error;
	}
	tex = tex_coords = (float *)PyMem_Malloc(sizeof(float) * count * 8);
	if (tex_coords == NULL) {
		PyErr_NoMemory();
		goto error;
	}
	item = PySequence_Fast_ITEMS(s);
	for (i = 0; i < count; i++) {
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
				"coords elements must be sequence of 4 float pairs, 8 floats or 12 floats");
				goto error;
		}
		tex += 8;
	}
	Py_DECREF(s);
	Py_DECREF(t);
	*count_out = count;
	return tex_coords;

error:
	PyMem_Free(tex_coords);
	Py_XDECREF(s);
	Py_XDECREF(t);
	return NULL;
}

static float *
get_tex_coords_3d(PyObject *tex_coords_seq, Py_ssize_t *count_out)
{
	PyObject *s = NULL, *t = NULL, **item;
	float *tex, *tex_coords = NULL;
	Py_ssize_t i, count, tlen;

	s = PySequence_Fast(tex_coords_seq, "coords not iterable");
	if (s == NULL)
		goto error;
	count = PySequence_Fast_GET_SIZE(s);
	if (count == 0) {
		PyErr_SetString(PyExc_ValueError, "coords is empty");
		goto error;
	}
	tex = tex_coords = (float *)PyMem_Malloc(sizeof(float) * count * 12);
	if (tex_coords == NULL) {
		PyErr_NoMemory();
		goto error;
	}
	item = PySequence_Fast_ITEMS(s);
	for (i = 0; i < count; i++) {
		t = PySequence_Tuple(item[i]);
		if (t == NULL)
			goto error;
		tlen = PyTuple_GET_SIZE(t);
		tex[2] = tex[5] = tex[8] = tex[11] = 0.0f;
		if (tlen == 4) {
			if (!PyArg_ParseTuple(t, 
				"(ff)(ff)(ff)(ff);Expected coords element with sequence of 4 float pairs", 
				tex, tex+1, tex+3, tex+4, tex+6, tex+7, tex+9, tex+10))
				goto error;
		} else if (tlen == 8) {
			if (!PyArg_ParseTuple(t, 
				"ffffffff;Expected coords element with sequence of 8 floats",
				tex, tex+1, tex+3, tex+4, tex+6, tex+7, tex+9, tex+10))
				goto error;
		} else if (tlen == 12) {
			if (!PyArg_ParseTuple(t, 
				"ffffffffffff;Expected coords element with sequence of 12 floats",
				tex, tex+1, tex+2, tex+3, tex+4, tex+5, tex+6, tex+7, tex+8, 
				tex+9, tex+10, tex+11))
				goto error;
		} else {
			PyErr_SetString(PyExc_ValueError, 
				"coords elements must be sequence of 4 float pairs, 8 floats or 12 floats");
				goto error;
		}
		tex += 12;
	}
	Py_DECREF(s);
	Py_DECREF(t);
	*count_out = count;
	return tex_coords;

error:
	PyMem_Free(tex_coords);
	Py_XDECREF(s);
	Py_XDECREF(t);
	return NULL;
}

static int
SpriteTex_init(SpriteTexObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *tex_coords_seq = NULL, *weights_seq = NULL;
	PyObject *s = NULL, *t = NULL, **item;
	int i;
	double total_weight, weight_scale, w;

	static char *kwlist[] = {"texture", "coords", "weights", "filter", "wrap", 
		"aspect_adjust_width", "aspect_adjust_height", NULL};

	PyMem_Free(self->tex_coords);
	self->tex_coords = NULL;
	PyMem_Free(self->weights);
	self->weights = NULL;
	self->tex_filter = GL_LINEAR;
	self->tex_wrap = GL_CLAMP;
	self->coord_count = 0;
	self->adjust_width = 0;
	self->adjust_height = 0;
	Py_CLEAR(self->tex_array);
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|OOiiii:__init__", kwlist,
		&self->texture, &tex_coords_seq, &weights_seq, &self->tex_filter, &self->tex_wrap,
		&self->adjust_width, &self->adjust_height))
		return -1;
	
	if (self->adjust_height && self->adjust_width) {
		PyErr_SetString(PyExc_TypeError,
			"SpriteTexturizer: Only one of aspect_adjust_width and aspect_adjust_height "
			"can be enabled at once");
		goto error;
	}
	if (tex_coords_seq != NULL) {
		self->tex_coords = get_tex_coords_2d(tex_coords_seq, &self->coord_count);
		if (self->tex_coords == NULL)
			goto error;
		if (weights_seq != NULL && weights_seq != Py_None) {
			s = PySequence_Fast(weights_seq, "SpriteTexturizer: weights not iterable");
			if (s == NULL)
				goto error;
			if (PySequence_Fast_GET_SIZE(s) != self->coord_count) {
				PyErr_SetString(PyExc_ValueError,
					"SpriteTexturizer: length of coords and weights do not match");
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
				w = PyFloat_AsDouble(item[i]);
				if (PyErr_Occurred())
					goto error;
				if (w <= 0) {
					PyErr_SetString(PyExc_ValueError,
						"SpriteTexturizer: weight values must be >= 0");
					goto error;
				}
				total_weight += w;
			}
			/* scale weights to full integer range */
			if (total_weight > (double)(WEIGHT_MAX)) {
				PyErr_SetString(PyExc_ValueError, "SpriteTexturizer: weights out of range");
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
	glBindTexture(GL_TEXTURE_2D, self->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, self->tex_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, self->tex_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, self->tex_wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, self->tex_wrap);

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
	Py_ssize_t tcount, coord_count;
	float *ptex, *ttex, *tex_coords;
	FloatArrayObject *tex_array;

	if (!GroupObject_Check(pgroup)) {
		PyErr_SetString(PyExc_TypeError, "Expected ParticleGroup first argument");
		return NULL;
	}

	if (self->tex_coords == NULL) {
		/* Special case, default texture coordinates. These can be cached
		   and shared between texturizers, so we don't generate them here */
		tex_array = generate_default_2D_tex_coords(pgroup);
	} else if (self->tex_array == NULL ||
		       self->tex_array->size < GroupObject_ActiveCount(pgroup) * 8) {
		/* calculate texture coordinates and cache them */
		pcount = pgroup->plist->palloc;
		Py_XDECREF(self->tex_array);
		tex_array = self->tex_array = FloatArray_new(pgroup->plist->palloc * 8);
		if (tex_array == NULL)
			return NULL;
		Py_INCREF(self->tex_array); /* for persistence */
		Py_INCREF(tex_array); /* for the caller */
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
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				if (--tcount <= 0) {
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
	} else {
		/* use cached texture coordinates */
		tex_array = self->tex_array;
		Py_INCREF(tex_array);
	}
	if (self->adjust_width) {
		adjust_particle_widths(pgroup, tex_array);
	} else if (self->adjust_height) {
		adjust_particle_heights(pgroup, tex_array);
	}

	return tex_array;
}

static PyMemberDef SpriteTex_members[] = {
	{"__dict__", T_OBJECT, offsetof(SpriteTexObject, dict), READONLY},
	{"aspect_adjust_width", T_INT, offsetof(SpriteTexObject, adjust_width), 0,
		"If true, the particle widths will be adjusted so the aspect "
		"ratio of their size matches that of their assigned texture coordinates."},
	{"aspect_adjust_height", T_INT, offsetof(SpriteTexObject, adjust_height), 0,
		"If true, the particle heights will be adjusted so the aspect "
		"ratio of their size matches that of their assigned texture coordinates."},
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
		PyDoc_STR("generate_tex_coords(group) -> FloatArray\n"
			"Generate texture coordinates for the given particle\n"
			"group and return them as a FloatArray ovject\n"
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
	"SpriteTexturizer(texture, coords=(), weights=(), filter=GL_LINEAR, wrap=GL_CLAMP, "
	"aspect_adjust_width=False, aspect_adjust_height=False)\n\n"
	"texture -- OpenGL texture name, acquired via glGenTextures. It is up\n"
	"to the application to load the texture's data before using the texturizer\n\n"
	"coords -- A sequence of texture coordinate sets. Each set consists of coordinates\n"
	"for the four corners of the quad drawn for a particle (8 floats). Sets may\n"
	"consist of 4 coordinate pairs (tuples) or simply 8 floats corresponding to the\n"
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
	"rendering. One of: GL_CLAMP, GL_REPEAT, GL_CLAMP_TO_EDGE, etc.\n\n"
	"aspect_adjust_width, aspect_adjust_height -- These two flags\n"
	"are used to match the aspect ratio of the particle's width and height\n"
	"to the dimensions of its texture coordinates. This is useful to\n"
	"match particles to textures of various dimensions without distortion.\n"
	"If one flag is set, the texturizer adjusts the width or height of the\n"
	"particle size respectively as appropriate. Only one of these flags\n"
	"may be set at one time.");

static PyTypeObject SpriteTex_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"_texturizer.SpriteTexturizer",		/*tp_name*/
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

/* --------------------------------------------------------------------- */

static PyTypeObject FlipBookTex_Type;

typedef struct {
	PyObject_HEAD
	PyObject *dict;
	GLuint texture;
	GLint tex_filter;
	GLint tex_wrap;
	int adjust_width;
	int adjust_height;
	Py_ssize_t coord_count;
	float *tex_coords;
	FloatArrayObject *tex_array;
	int dimension;
	int loop;
	float duration;
	float *frame_times;
} FlipBookTexObject;

static PyObject *
FlipBookTex_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    FlipBookTexObject *self;

    self = (FlipBookTexObject *)type->tp_alloc(type, 0);
    if (self == NULL)
		return NULL;
	self->dict = PyDict_New();
    if (self->dict == NULL) {
		Py_DECREF(self);
		return NULL;
	}
	self->tex_coords = NULL;
	self->frame_times = NULL;
	self->tex_array = NULL;
	return (PyObject *)self;
}

static int
FlipBookTex_clear(FlipBookTexObject *self) 
{
	Py_CLEAR(self->dict);
	Py_CLEAR(self->tex_array);
	return 0;
}

static void
FlipBookTex_dealloc(FlipBookTexObject *self) 
{
	PyMem_Free(self->tex_coords);
	self->tex_coords = NULL;
	PyMem_Free(self->frame_times);
	self->frame_times = NULL;
	FlipBookTex_clear(self);
	self->ob_type->tp_free((PyObject *)self);
}

static int
FlipBookTex_traverse(FlipBookTexObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->dict);
    Py_VISIT(self->tex_array);
    return 0;
}

static int
FlipBookTex_init(FlipBookTexObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *tex_coords_seq = NULL, *duration = NULL;
	PyObject *s = NULL, **item;
	int i;
	double total_time, t;

	static char *kwlist[] = {"texture", "coords", "duration", "loop", "dimension",
		"filter", "wrap", "aspect_adjust_width", "aspect_adjust_height", NULL};

	PyMem_Free(self->tex_coords);
	self->tex_coords = NULL;
	PyMem_Free(self->frame_times);
	self->frame_times = NULL;
	self->tex_filter = GL_LINEAR;
	self->tex_wrap = GL_CLAMP;
	self->coord_count = 0;
	self->adjust_width = 0;
	self->adjust_height = 0;
	self->loop = 1;
	self->dimension = 2;
	Py_CLEAR(self->tex_array);
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iOO|iiiiii:__init__", kwlist,
		&self->texture, &tex_coords_seq, &duration, &self->loop, &self->dimension,
		&self->tex_filter, &self->tex_wrap,
		&self->adjust_width, &self->adjust_height))
		return -1;
	
	if (self->adjust_height && self->adjust_width) {
		PyErr_SetString(PyExc_TypeError,
			"FlipBookTexturizer: Only one of aspect_adjust_width and aspect_adjust_height "
			"can be enabled at once");
		goto error;
	}
	if (self->dimension == 2) {
		self->tex_coords = get_tex_coords_2d(tex_coords_seq, &self->coord_count);
		if (self->tex_coords == NULL)
			goto error;
	} else if (self->dimension == 3) {
		self->tex_coords = get_tex_coords_3d(tex_coords_seq, &self->coord_count);
		if (self->tex_coords == NULL)
			goto error;
	} else {
		PyErr_SetString(PyExc_ValueError,
			"FlipBookTexturizer: expected dimension value of 2 or 3");
		goto error;
	}
	
	if (PySequence_Check(duration)) {
		s = PySequence_Fast(duration, "FlipBookTexturizer: duration not iterable");
		if (s == NULL)
			goto error;
		if (PySequence_Fast_GET_SIZE(s) != self->coord_count) {
			PyErr_SetString(PyExc_ValueError,
				"FlipBookTexturizer: length of coords and duration do not match");
			goto error;
		}
		self->frame_times = (float *)PyMem_Malloc(sizeof(float) * self->coord_count);
		if (self->frame_times == NULL) {
			PyErr_NoMemory();
			goto error;
		}
		item = PySequence_Fast_ITEMS(s);
		total_time = 0.0;
		for (i = 0; i < self->coord_count; i++) {
			t = PyFloat_AsDouble(item[i]);
			if (PyErr_Occurred())
				goto error;
			if (t < 0) {
				PyErr_SetString(PyExc_ValueError, "FlipBookTexturizer: negative frame time");
				goto error;
			}
			total_time += t;
			self->frame_times[i] = (float)total_time;
		}
		self->duration = (float)(total_time / self->coord_count);
		Py_CLEAR(s);
	} else if (PyNumber_Check(duration)) {
		s = PyNumber_Float(duration);
		if (s == NULL)
			goto error;
		self->duration = (float)PyFloat_AsDouble(s);
		if (PyErr_Occurred())
			goto error;
		if (self->duration <= 0) {
			PyErr_SetString(PyExc_ValueError, "FlipBookTexturizer: expected duration >= 0");
			goto error;
		}
		Py_CLEAR(s);
	} else {
		PyErr_SetString(PyExc_TypeError, 
			"FlipBookTexturizer: duration must be number or number sequence");
		goto error;
	}
	return 0;

error:
	Py_XDECREF(s);
	PyMem_Free(self->tex_coords);
	self->tex_coords = NULL;
	PyMem_Free(self->frame_times);
	self->frame_times = NULL;
	return -1;
}

static PyObject *
FlipBookTex_get_tex_coords(FlipBookTexObject *self, void *closure)
{
	PyObject *coord_sets = NULL, *set = NULL;
	float *tex;
	int i, c;

	tex = self->tex_coords;
	if (tex != NULL) {
		coord_sets = PyTuple_New(self->coord_count);
		if (coord_sets == NULL)
			goto error;
		if (self->dimension == 2) {
			for (i = 0, c = 0; i < self->coord_count; i++, c+=8) {
				set = Py_BuildValue("(ffffffff)",
					tex[c], tex[c+1], tex[c+2], tex[c+3], tex[c+4], tex[c+5], tex[c+6], tex[c+7]);
				if (set == NULL)
					goto error;
				PyTuple_SET_ITEM(coord_sets, i, set);
			}
		} else {
			for (i = 0, c = 0; i < self->coord_count; i++, c+=8) {
				set = Py_BuildValue("(ffffffffffff)",
					tex[c], tex[c+1], tex[c+2], tex[c+3], 
					tex[c+4], tex[c+5], tex[c+6], tex[c+7],
					tex[c+8], tex[c+9], tex[c+10], tex[c+11]);
				if (set == NULL)
					goto error;
				PyTuple_SET_ITEM(coord_sets, i, set);
			}
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
FlipBookTex_get_duration(FlipBookTexObject *self, void *closure)
{
	PyObject *times = NULL, *t = NULL;
	double total = 0.0;
	int i;
	
	if (self->frame_times != NULL) {
		times = PyTuple_New(self->coord_count);
		if (times == NULL)
			goto error;
		for (i = 0; i < self->coord_count; i++) {
			t = PyFloat_FromDouble((double)self->frame_times[i] - total);
			if (t == NULL)
				goto error;
			PyTuple_SET_ITEM(times, i, t);
			total = (double)self->frame_times[i];
		}
		return times;
	} else {
		return PyFloat_FromDouble((double)self->duration);
	}
error:
	Py_XDECREF(times);
	Py_XDECREF(t);
	return NULL;
}
static PyObject *
FlipBookTex_set_state(FlipBookTexObject *self)
{
	GLenum tex_target;
	if (self->dimension == 2) {
		tex_target = GL_TEXTURE_2D;
	} else if (self->dimension == 3) {
		tex_target = GL_TEXTURE_3D;
	} else {
		PyErr_SetString(PyExc_ValueError,
			"FlipBookTexturizer: invalid dimension value");
		return NULL;
	}
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(tex_target);
	glBindTexture(tex_target, self->texture);
	glTexParameteri(tex_target, GL_TEXTURE_MIN_FILTER, self->tex_filter);
	glTexParameteri(tex_target, GL_TEXTURE_MAG_FILTER, self->tex_filter);
	glTexParameteri(tex_target, GL_TEXTURE_WRAP_S, self->tex_wrap);
	glTexParameteri(tex_target, GL_TEXTURE_WRAP_T, self->tex_wrap);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
FlipBookTex_restore_state(FlipBookTexObject *self)
{
	glPopAttrib();

	Py_INCREF(Py_None);
	return Py_None;
}

static FloatArrayObject *
FlipBookTex_generate_tex_coords(FlipBookTexObject *self, GroupObject *pgroup)
{
	register unsigned long pcount;
	register Particle *p;
	int coord_count, loop, last_coord, frame = 0;
	register float *ptex, *ttex;
	float *tex_coords, total_time, duration, age, *times;

	if (!GroupObject_Check(pgroup)) {
		PyErr_SetString(PyExc_TypeError, "Expected ParticleGroup first argument");
		return NULL;
	}

	pcount = GroupObject_ActiveCount(pgroup);
	p = pgroup->plist->p;

	if (self->tex_array == NULL || self->tex_array->size < pcount * self->dimension * 4) {
		Py_XDECREF(self->tex_array);
		self->tex_array = FloatArray_new(pgroup->plist->palloc * self->dimension * 4);
		if (self->tex_array == NULL)
			return NULL;
	}

	ptex = self->tex_array->data;
	tex_coords = self->tex_coords;
	coord_count = self->coord_count;
	last_coord = self->coord_count - 1;
	times = self->frame_times;
	loop = self->loop;
	
	if (self->dimension == 2) {
		if (times == NULL) {
			total_time = self->duration * last_coord;
			duration = self->duration;
			while (pcount--) {
				if (p->age >= 0.0f) {
					if (loop) {
						frame = (int)(p->age / duration) % coord_count; 
					} else {
						frame = (int)(fminf(p->age, total_time) / duration);
					}
				} /* we don't care what the frame is for dead particles */ 
				ttex = tex_coords + frame * 8;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				p++;
			}
		} else {
			total_time = times[last_coord];
			while (pcount--) {
				if (p->age >= 0.0f) {
					if (loop) {
						age = fmodf(p->age, total_time);
					} else {
						age = p->age;
					}
					for (; frame < last_coord && age > times[frame]; frame++);
					for (; frame > 0 && age <= times[frame - 1]; frame--);
				} /* we don't care what the frame is for dead particles */ 
				ttex = tex_coords + frame * 8;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				p++;
			}
		}
		if (self->adjust_width) {
			adjust_particle_widths(pgroup, self->tex_array);
		} else if (self->adjust_height) {
			adjust_particle_heights(pgroup, self->tex_array);
		}
	} else { /* 3D texture coords */
		if (times == NULL) {
			total_time = self->duration * last_coord;
			duration = self->duration;
			while (pcount--) {
				if (p->age >= 0.0f) {
					if (loop) {
						frame = (int)(p->age / duration) % coord_count; 
					} else {
						frame = (int)(fminf(p->age, total_time) / duration);
					}
				} /* we don't care what the frame is for dead particles */ 
				ttex = tex_coords + frame * 12;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				p++;
			}
		} else {
			total_time = times[last_coord];
			while (pcount--) {
				if (p->age >= 0.0f) {
					if (loop) {
						age = fmodf(p->age, total_time);
					} else {
						age = p->age;
					}
					for (; frame < last_coord && age > times[frame]; frame++);
					for (; frame > 0 && age <= times[frame - 1]; frame--);
				} /* we don't care what the frame is for dead particles */ 
				ttex = tex_coords + frame * 12;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				*ptex++ = *ttex++;
				p++;
			}
		}
	}
	Py_INCREF(self->tex_array);
	return self->tex_array;
}

static PyMemberDef FlipBookTex_members[] = {
	{"__dict__", T_OBJECT, offsetof(FlipBookTexObject, dict), READONLY},
	{"loop", T_INT, offsetof(FlipBookTexObject, loop), 0,
		"If true the animation will loop continuously, if false it "
		"stop at the last texture frame."},
	{"tex_dimension", T_INT, offsetof(FlipBookTexObject, dimension), READONLY,
		"The number of dimensions per texture coordinate"},
	{"aspect_adjust_width", T_INT, offsetof(FlipBookTexObject, adjust_width), 0,
		"If true, the particle widths will be adjusted so the aspect "
		"ratio of their size matches that of their assigned texture coordinates."},
	{"aspect_adjust_height", T_INT, offsetof(FlipBookTexObject, adjust_height), 0,
		"If true, the particle heights will be adjusted so the aspect "
		"ratio of their size matches that of their assigned texture coordinates."},
	{NULL}
};

static PyMethodDef FlipBookTex_methods[] = {
	{"set_state", (PyCFunction)FlipBookTex_set_state, METH_NOARGS,
		PyDoc_STR("set_state() -> None\n"
			"Setup the OpenGL texture state for rendering.\n"
			"Called by the renderer before particles are drawn.")},
	{"restore_state", (PyCFunction)FlipBookTex_restore_state, METH_NOARGS,
		PyDoc_STR("restore_state() -> None\n"
			"Restore the OpenGL texture state after rendering.\n"
			"Called by the renderer after particles are drawn.")},
	{"generate_tex_coords", (PyCFunction)FlipBookTex_generate_tex_coords, METH_O,
		PyDoc_STR("generate_tex_coords(group) -> FloatArray\n"
			"Generate texture coordinates for the given particle\n"
			"group and return them as a FloatArray ovject\n"
			"Called by the renderer when particles are drawn.")},
	{NULL,		NULL}		/* sentinel */
};

static PyGetSetDef FlipBookTex_descriptors[] = {
	{"tex_coords", (getter)FlipBookTex_get_tex_coords, NULL, 
		"The sequence of texture coordinate sets", NULL},
	{"duration", (getter)FlipBookTex_get_duration, NULL, 
		"Return the time duration for all frames or \n"
		"a sequence of durations for each frame", NULL},
	{NULL}
};

PyDoc_STRVAR(FlipBookTex__doc__, 
	"Animates sets of texture coordinates from a single resident\n"
	"\"flipbook\" texture to a particle group according to each particle's age.\n\n"
	"FlipBookTexturizer(texture, coords, duration, loop=True, dimension=2, "
	"filter=GL_LINEAR, wrap=GL_CLAMP, "
	"aspect_adjust_width=False, aspect_adjust_height=False)\n\n"
	"texture -- OpenGL texture name, acquired via glGenTextures. It is up\n"
	"to the application to load the texture's data before using the texturizer\n\n"
	"coords -- A sequence of texture coordinate sets. Each set is used as one\n"
	"frame of the animation. Each set consists of coordinates for the four\n"
	"corners of the quad drawn for a particle (8 or 12 floats). When using 8\n"
	"floats, the r value of the texture coordinates is set to 0.\n\n"
	"duration -- The time duration of each frame. This may be specified as a\n"
	"single value, if the duration of all frames are the same, or as a sequence\n"
	"of individual duration values for each frame.\n\n"
	"loop -- If true (the default), the animation will continuously loop through\n"
	"the frames from last back to first. If false, the animation will stop on the\n"
	"last frame.\n\n"
	"dimension -- The number of texture coordinate dimensions. Use 2 for 2D\n"
	"textures (the default) or 3 for 3D textures.\n\n"
	"filter -- The OpenGL filter used to scale the texture when rendering.\n"
	"One of: GL_NEAREST, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, etc.\n\n"
	"wrap -- The OpenGL wrapping parameter for texture application when\n"
	"rendering. One of: GL_CLAMP, GL_REPEAT, GL_CLAMP_TO_EDGE, etc.\n\n"
	"aspect_adjust_width, aspect_adjust_height -- These two flags\n"
	"are used to match the aspect ratio of the particle's width and height\n"
	"to the dimensions of its texture coordinates. This is useful to\n"
	"match particles to textures of various dimensions without distortion.\n"
	"If one flag is set, the texturizer adjusts the width or height of the\n"
	"particle size respectively as appropriate. Only one of these flags\n"
	"may be set at one time.");

static PyTypeObject FlipBookTex_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"_texturizer.FlipBookTexturizer",		/*tp_name*/
	sizeof(FlipBookTexObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)FlipBookTex_dealloc, /*tp_dealloc*/
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
	FlipBookTex__doc__,       /*tp_doc*/
	(traverseproc)FlipBookTex_traverse,  /*tp_traverse*/
	(inquiry)FlipBookTex_clear,          /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	FlipBookTex_methods,      /*tp_methods*/
	FlipBookTex_members,      /*tp_members*/
	FlipBookTex_descriptors,  /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	offsetof(FlipBookTexObject, dict), /*tp_dictoffset*/
	(initproc)FlipBookTex_init, /*tp_init*/
	0,                      /*tp_alloc*/
	FlipBookTex_new,          /*tp_new*/
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
	if (PyType_Ready(&FlipBookTex_Type) < 0)
		return;

	/* Create the module and add the types */
	m = Py_InitModule3("_texturizer", NULL, "Particle renderer texturizers");
	if (m == NULL)
		return;

	Py_INCREF(&SpriteTex_Type);
	PyModule_AddObject(m, "SpriteTexturizer", (PyObject *)&SpriteTex_Type);
	Py_INCREF(&FlipBookTex_Type);
	PyModule_AddObject(m, "FlipBookTexturizer", (PyObject *)&FlipBookTex_Type);
}
