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
/* Particle emitters
 *
 * $Id$ */

#include <Python.h>
#include <structmember.h>
#include <float.h>
#include <time.h>
#include "fastrng.h"
#include "group.h"
#include "vector.h"

static PyTypeObject StaticEmitter_Type;

#define DISCRETE_COUNT 8

static const char *discrete_names[] = {
	"position", "velocity", "size", "up", 
	"rotation", "color", "mass", "age", 
	NULL
};

#define POSITION_I 0
#define VELOCITY_I 1
#define SIZE_I 2
#define UP_I 3
#define ROTATION_I 4
#define COLOR_I 5
#define MASS_I 6
#define AGE_I 7

typedef struct {
	PyObject_HEAD
	Particle ptemplate;
	Particle pdeviation;
	int has_deviation;
	float rate;
	float partial;
	float time_to_live;
	PyObject *domain[DISCRETE_COUNT];
	PyObject *discrete[DISCRETE_COUNT];
} StaticEmitterObject;

static void
StaticEmitter_dealloc(StaticEmitterObject *self) {
	int i;
	for (i = 0; i < DISCRETE_COUNT; i++) {
		Py_XDECREF(self->domain[i]);
		Py_XDECREF(self->discrete[i]);
	}
	PyObject_Del(self);
}

#define NO_TTL -1.0f

static int
StaticEmitter_init(StaticEmitterObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *ptemplate = NULL, *pdeviation = NULL;
	PyObject *key, *value;
	char *key_str;
	int i, success;
	Py_ssize_t pos;

	for (i = 0; i < DISCRETE_COUNT; i++) {
		self->domain[i] = NULL;
		self->discrete[i] = NULL;
	}
	self->rate = -FLT_MAX;
	self->time_to_live = NO_TTL;
	if (!PyArg_ParseTuple(args, "|fOOf:__init__",
		&self->rate, &ptemplate, &pdeviation, &self->time_to_live))
		return -1;
	
	/* Grab kwargs, since we accept arbitrary kwargs we need to do this by
	 * hand */
	if (kwargs != NULL) {
		if (self->rate == -FLT_MAX) {
			value = PyDict_GetItemString(kwargs, "rate");
			if (value != NULL) {
				value = PyNumber_Float(value);
				if (value == NULL) {
					PyErr_SetString(PyExc_ValueError, 
						"StaticEmitter: expected float value for rate");
					return -1;
				}
				self->rate = PyFloat_AS_DOUBLE(value);
				Py_DECREF(value);
				PyDict_DelItemString(kwargs, "rate");
			}
		}
		if (self->time_to_live == NO_TTL) {
			value = PyDict_GetItemString(kwargs, "time_to_live");
			if (value != NULL) {
				value = PyNumber_Float(value);
				if (value == NULL) {
					PyErr_SetString(PyExc_ValueError, 
						"StaticEmitter: expected float value for time_to_live");
					return -1;
				}
				self->time_to_live = PyFloat_AS_DOUBLE(value);
				Py_DECREF(value);
				PyDict_DelItemString(kwargs, "time_to_live");
			}
		}
		if (ptemplate == NULL) {
			ptemplate = PyDict_GetItemString(kwargs, "template");
			if (ptemplate != NULL) {
				Py_INCREF(ptemplate);
				PyDict_DelItemString(kwargs, "template");
			}
		} else {
			Py_INCREF(ptemplate);
		}
		if (pdeviation == NULL) {
			pdeviation = PyDict_GetItemString(kwargs, "deviation");
			if (pdeviation != NULL) {
				Py_INCREF(pdeviation);
				PyDict_DelItemString(kwargs, "deviation");
			}
		} else {
			Py_INCREF(pdeviation);
		}

		pos = 0;
		while (PyDict_Next(kwargs, &pos, &key, &value)) {
			key_str = PyString_AsString(key);
			if (key_str == NULL)
				goto error;
			for (i = 0; i < DISCRETE_COUNT; i++) {
				if (!strcmp(key_str, discrete_names[i])) {
					if (PyObject_HasAttrString(value, "generate")) {
						Py_INCREF(value);
						self->domain[i] = value;
					} else if (PySequence_Check(value)) {
						value = PySequence_Fast(value, 
							"StaticEmitter: Invalid discrete value sequence");
						if (value == NULL)
							goto error;
						if (PySequence_Fast_GET_SIZE(value) == 0) {
							Py_DECREF(value);
							PyErr_Format(PyExc_TypeError, 
								"StaticEmitter: empty discrete value sequence "
								"for %s", key_str);
							goto error;
						}
						self->discrete[i] = value;
					} else {
						Py_DECREF(value);
						PyErr_Format(PyExc_TypeError, 
							"StaticEmitter: discrete argument %s not "
							"a sequence or domain", key_str);
						goto error;
					}
					value = NULL;
				}
			}
			if (value != NULL) {
				PyErr_Format(PyExc_TypeError, 
					"StaticEmitter: unexpected keyword argument: %s",
					key_str);
				goto error;
			}
		}
	}

	if (self->rate == -FLT_MAX)
		self->rate = 0;

	if (ptemplate != NULL) {
		success = (
			get_Vec3(&self->ptemplate.position, ptemplate, "position") &&
			get_Vec3(&self->ptemplate.velocity, ptemplate, "velocity") &&
			get_Vec3(&self->ptemplate.size, ptemplate, "size") &&
			get_Vec3(&self->ptemplate.up, ptemplate, "up") &&
			get_Vec3(&self->ptemplate.rotation, ptemplate, "rotation") &&
			get_Color(&self->ptemplate.color, ptemplate, "color") &&
			get_Float(&self->ptemplate.age, ptemplate, "age") &&
			get_Float(&self->ptemplate.mass, ptemplate, "mass"));
		Py_DECREF(ptemplate);
		if (!success)
			goto error;
	}
	
	if (pdeviation != NULL) {
		success = (
			get_Vec3(&self->pdeviation.position, pdeviation, "position") &&
			get_Vec3(&self->pdeviation.velocity, pdeviation, "velocity") &&
			get_Vec3(&self->pdeviation.size, pdeviation, "size") &&
			get_Vec3(&self->pdeviation.up, pdeviation, "up") &&
			get_Vec3(&self->pdeviation.rotation, pdeviation, "rotation") &&
			get_Color(&self->pdeviation.color, pdeviation, "color") &&
			get_Float(&self->pdeviation.age, pdeviation, "age") &&
			get_Float(&self->pdeviation.mass, pdeviation, "mass"));
		Py_DECREF(pdeviation);
		if (!success)
			goto error;
		self->has_deviation = 1;
	} else {
		self->has_deviation = 0;
	}
	
	return 0;

error:
	Py_XDECREF(ptemplate);
	Py_XDECREF(pdeviation);
	return -1;
}

/* Fill in a vector value either from a domain, discrete sequence or template
 * vector value. Return true on success
 */
static inline int
Vec3_fill(Vec3 * __restrict__ vec, PyObject *domain, PyObject *discrete_seq, 
	Vec3 * __restrict__ tmpl)
{
	PyObject *v = NULL;

	if (domain != NULL) {
		v = PyObject_CallMethod(domain, "generate", NULL);
		if (v == NULL)
			return 0;
		if (!Vec3_FromSequence(vec, v))
			return 0;
	} else if (discrete_seq != NULL) {
		v = PySequence_Fast_GET_ITEM(discrete_seq,
				(Py_ssize_t)(PySequence_Fast_GET_SIZE(discrete_seq) * rand_uni()));
		if (!Vec3_FromSequence(vec, v))
			return 0;
	} else {
		Vec3_copy(vec, tmpl);
	}
	return 1;
}

/* Fill in a color value either from a domain, discrete sequence or template
 * vector value. Return true on success
 */
static inline int
Color_fill(Color * __restrict__ color, PyObject *domain, PyObject *discrete_seq, 
	Color * __restrict__ tmpl)
{
	PyObject *v = NULL;

	if (domain != NULL) {
		v = PyObject_CallMethod(domain, "generate", NULL);
		if (v == NULL)
			return 0;
		if (!Color_FromSequence(color, v))
			return 0;
	} else if (discrete_seq != NULL) {
		v = PySequence_Fast_GET_ITEM(discrete_seq,
				(Py_ssize_t)(PySequence_Fast_GET_SIZE(discrete_seq) * rand_uni()));
		if (!Color_FromSequence(color, v))
			return 0;
	} else {
		Color_copy(color, tmpl);
	}
	return 1;
}

/* Fill in a float value either from a domain, discrete sequence or template
 * vector value. Return true on success
 */
static inline int
Float_fill(float * f, PyObject *domain, PyObject *discrete_seq, float tmpl)
{
	int result = 0;
	PyObject *v = NULL, *pyfloat = NULL;

	if (domain != NULL) {
		v = PyObject_CallMethod(domain, "generate", NULL);
		if (v == NULL)
			return 0;
	} else if (discrete_seq != NULL) {
		v = PySequence_Fast_GET_ITEM(discrete_seq,
				(Py_ssize_t)(PySequence_Fast_GET_SIZE(discrete_seq) * rand_uni()));
		Py_INCREF(v);
	}

	if (v != NULL) {
		pyfloat = PyNumber_Float(v);
		if (pyfloat != NULL) {
			*f = PyFloat_AS_DOUBLE(pyfloat);
			result = 1;
		}
	} else {
		*f = tmpl;
		return 1;
	}
	Py_XDECREF(v);
	Py_XDECREF(pyfloat);
	return result;
}

inline void
Vec3_deviate(Vec3 *dest, Vec3 *deviation)
{
	dest->x = deviation->x ? rand_norm(dest->x, deviation->x) : dest->x;
	dest->y = deviation->y ? rand_norm(dest->y, deviation->y) : dest->y;
	dest->z = deviation->z ? rand_norm(dest->z, deviation->z) : dest->z;
}

inline void
Color_deviate(Color *dest, Color *deviation)
{
	dest->r = deviation->r ? rand_norm(dest->r, deviation->r) : dest->r;
	dest->g = deviation->g ? rand_norm(dest->g, deviation->g) : dest->g;
	dest->b = deviation->b ? rand_norm(dest->b, deviation->b) : dest->b;
	dest->a = deviation->a ? rand_norm(dest->a, deviation->a) : dest->a;
}

/* Populate the values for a particle based on the emitter's domain,
 * discrete and template particle values
 */
static inline int
StaticEmitter_make_particle(StaticEmitterObject *self, Particle *p)
{
	int success = (
		Vec3_fill(&p->position, self->domain[POSITION_I], 
			self->discrete[POSITION_I], &self->ptemplate.position) &&
		Vec3_fill(&p->velocity, self->domain[VELOCITY_I], 
			self->discrete[VELOCITY_I], &self->ptemplate.velocity) &&
		Vec3_fill(&p->size, self->domain[SIZE_I], 
			self->discrete[SIZE_I], &self->ptemplate.size) &&
		Vec3_fill(&p->up, self->domain[UP_I], 
			self->discrete[UP_I], &self->ptemplate.up) &&
		Vec3_fill(&p->rotation, self->domain[ROTATION_I], 
			self->discrete[ROTATION_I], &self->ptemplate.rotation) &&
		Color_fill(&p->color, self->domain[COLOR_I], 
			self->discrete[COLOR_I], &self->ptemplate.color) &&
		Float_fill(&p->age, self->domain[AGE_I], 
			self->discrete[AGE_I], self->ptemplate.age) &&
		Float_fill(&p->mass, self->domain[MASS_I], 
			self->discrete[MASS_I], self->ptemplate.mass));
	if (!success)
		return 0;
	if (self->has_deviation) {
		Vec3_deviate(&p->position, &self->pdeviation.position);
		Vec3_deviate(&p->velocity, &self->pdeviation.velocity);
		Vec3_deviate(&p->size, &self->pdeviation.size);
		Vec3_deviate(&p->up, &self->pdeviation.up);
		Vec3_deviate(&p->rotation, &self->pdeviation.rotation);
		Color_deviate(&p->color, &self->pdeviation.color);
		p->age = self->pdeviation.age ? rand_norm(p->age, self->pdeviation.age) : p->age;
		p->mass = self->pdeviation.mass ? rand_norm(p->mass, self->pdeviation.mass) : p->mass;
	}
	if (p->age < 0)
		p->age = 0;
	return 1;
}

static PyObject *
StaticEmitter_call(StaticEmitterObject *self, PyObject *args)
{
	float td;
	GroupObject *pgroup;
	float count;
	long pindex;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "fO:__init__", &td, &pgroup))
		return NULL;
	
	if (!GroupObject_Check(pgroup))
		return NULL;

	if (self->time_to_live != NO_TTL) {
		if (self->time_to_live > td) {
			self->time_to_live -= td;
		} else {
			/* time's up, remove ourselves from the group */
			td = self->time_to_live;
			self->time_to_live = 0;
			result = PyObject_CallMethod((PyObject *)pgroup, "unbind_controller", 
				"O", (PyObject *)self);
			if (result == NULL)
				return NULL;
			Py_DECREF(result);
		}
	}
	count = td * self->rate + self->partial;
	result = PyInt_FromLong((long)count);

	while (count >= 1.0f) {
		pindex = Group_new_p(pgroup);
		if (pindex < 0) {
			PyErr_NoMemory();
			Py_DECREF(result);
			return NULL;
		}
		if (!StaticEmitter_make_particle(self, &pgroup->plist->p[pindex])) {
			Py_DECREF(result);
			return NULL;
		}
		count--;
	}
	self->partial = count;

	return result;
}

static PyObject *
StaticEmitter_emit(StaticEmitterObject *self, PyObject *args)
{
	unsigned long count;
	GroupObject *pgroup;
	long pindex;

	if (!PyArg_ParseTuple(args, "kO:__init__", &count, &pgroup))
		return NULL;
	
	if (!GroupObject_Check(pgroup))
		return NULL;
	
	while (count--) {
		pindex = Group_new_p(pgroup);
		if (pindex < 0) {
			PyErr_NoMemory();
			return NULL;
		}
		if (!StaticEmitter_make_particle(self, &pgroup->plist->p[pindex])) {
			return NULL;
		}
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMemberDef StaticEmitter_members[] = {
    {"rate", T_FLOAT, offsetof(StaticEmitterObject, rate), RESTRICTED,
        "Rate of particle emission per unit time"},
    {"time_to_live", T_FLOAT, offsetof(StaticEmitterObject, time_to_live), RESTRICTED,
        "Time remaining before emitter is removed from the group (-1 to disable)"},
	{NULL}
};

static PyMethodDef ParticleGroup_methods[] = {
	{"emit", (PyCFunction)StaticEmitter_emit, METH_VARARGS,
		PyDoc_STR("emit(count, group) -> None\n"
			"Emit count new particles into the group specified.\n"
			"This call is not affected by the emitter rate or\n"
			"time to live values.")},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
StaticEmitter_getattr(StaticEmitterObject *self, PyObject *o)
{
	char *name = PyString_AS_STRING(o);
	if (!strcmp(name, "template")) {
		return (PyObject *)ParticleRefObject_New(NULL, &self->ptemplate);	
	} else if (!strcmp(name, "deviation")) {
		return (PyObject *)ParticleRefObject_New(NULL, &self->ptemplate);	
	} else if (!strcmp(name, "rate")) {
		return PyMember_GetOne((char *)self, &StaticEmitter_members[0]);
	} else if (!strcmp(name, "time_to_live")) {
		return PyMember_GetOne((char *)self, &StaticEmitter_members[1]);
	} else {
		return Py_FindMethod(ParticleGroup_methods, (PyObject *)self, name);
	}
}

PyDoc_STRVAR(StaticEmitter__doc__, 
	"Creates particles in a group at a fixed rate deriving partice\n"
	"attributes from a configurable mix of domain, discrete value lists\n"
	"and fixed template particles plus random deviation\n\n"
	"StaticEmitter(rate, template=None, deviation=None, time_to_live=None, **discrete)\n\n"
	"rate -- Emission rate in particles per unit time.\n\n"
	"template -- A Particle instance used as the basis (mathematical\n"
	"average) for the emitted particles' attributes for attributes\n"
	"not specified using a keyword argument\n\n"
	"deviation -- A Particle instance used as the standard deviation for\n"
	"randomizing the particle attributes. If deviation is not specified\n"
	"then the emitted particles attribute values are deterministic\n\n"
	"time_to_live -- If specified, the emitter will unbind itself from\n"
	"its calling group after the specified time has elapsed. This\n"
	"allows you to schedule the emitter to stop participating in\n"
	"a group after a certain elapsed time.\n\n"
	"discrete -- Additional keyword arguments can be supplied to specify\n"
	"discrete values for particular particle attributes. These values\n"
	"override the cooresponding value in the template. The values may be\n"
	"supplied as a sequence of specific values, or as a domain instance (or\n"
	"any object with a generate() method returning values).  This allows you\n"
	"to specify a discrete range of values that will be used uniformly for\n"
	"a particular attribute. Note the discrete values are still randomized\n"
	"by the deviation template the same way as the template values.");

static PyTypeObject StaticEmitter_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"emitter.StaticEmitter",		/*tp_name*/
	sizeof(StaticEmitterObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)StaticEmitter_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,   /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	(ternaryfunc)StaticEmitter_call, /*tp_call*/
	0,                      /*tp_str*/
	(getattrofunc)StaticEmitter_getattr, /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	StaticEmitter__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	ParticleGroup_methods,  /*tp_methods*/
	StaticEmitter_members,  /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)StaticEmitter_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

PyMODINIT_FUNC
initemitter(void)
{
	PyObject *m;

	/* Bind tp_new and tp_alloc here to appease certain compilers */
	StaticEmitter_Type.tp_alloc = PyType_GenericAlloc;
	StaticEmitter_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&StaticEmitter_Type) < 0)
		return;

	/* Create the module and add the types */
	m = Py_InitModule3("emitter", NULL, "Particle Emitters");
	if (m == NULL)
		return;

	Py_INCREF(&StaticEmitter_Type);
	PyModule_AddObject(m, "StaticEmitter", (PyObject *)&StaticEmitter_Type);

	rand_seed(time(NULL));
}
