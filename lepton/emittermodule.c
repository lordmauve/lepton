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

#include "cccompat.h"
#include "compat.h"
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
Emitter_dealloc(StaticEmitterObject *self) {
	int i;
	for (i = 0; i < DISCRETE_COUNT; i++) {
		Py_XDECREF(self->domain[i]);
		Py_XDECREF(self->discrete[i]);
	}
	PyObject_Del(self);
}

#define NO_TTL -1.0f

static int
Emitter_parse_kwargs(StaticEmitterObject *self,
	PyObject **ptemplate, PyObject **pdeviation, PyObject *kwargs)
{
	PyObject *key, *value;
	Py_ssize_t pos;
	char *key_str;
	int i;

	if (self->rate == -FLT_MAX) {
		value = PyDict_GetItemString(kwargs, "rate");
		if (value != NULL) {
			value = PyNumber_Float(value);
			if (value == NULL) {
				PyErr_SetString(PyExc_ValueError,
					"StaticEmitter: expected float value for rate");
				return 0;
			}
			self->rate = (float)PyFloat_AS_DOUBLE(value);
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
				return 0;
			}
			self->time_to_live = (float)PyFloat_AS_DOUBLE(value);
			Py_DECREF(value);
			PyDict_DelItemString(kwargs, "time_to_live");
		}
	}
	if (*ptemplate == NULL) {
		*ptemplate = PyDict_GetItemString(kwargs, "template");
		if (*ptemplate != NULL) {
			Py_INCREF(*ptemplate);
			PyDict_DelItemString(kwargs, "template");
		}
	} else {
		Py_INCREF(*ptemplate);
	}
	if (*pdeviation == NULL) {
		*pdeviation = PyDict_GetItemString(kwargs, "deviation");
		if (*pdeviation != NULL) {
			Py_INCREF(*pdeviation);
			PyDict_DelItemString(kwargs, "deviation");
		}
	} else {
		Py_INCREF(*pdeviation);
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
	return 1;

error:
	Py_XDECREF(*ptemplate);
	Py_XDECREF(*pdeviation);
	return 0;
}

static int
Emitter_fill_particle_from(Particle *p, PyObject *ptemplate)
{
	return (
		get_Vec3(&p->position, NULL, ptemplate, "position") &&
		get_Vec3(&p->velocity, NULL, ptemplate, "velocity") &&
		get_Vec3(&p->size, NULL, ptemplate, "size") &&
		get_Vec3(&p->up, NULL, ptemplate, "up") &&
		get_Vec3(&p->rotation, NULL, ptemplate, "rotation") &&
		get_Color(&p->color, NULL, ptemplate, "color") &&
		get_Float(&p->age, NULL, ptemplate, "age") &&
		get_Float(&p->mass, NULL, ptemplate, "mass"));
}

static int
StaticEmitter_init(StaticEmitterObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *ptemplate = NULL, *pdeviation = NULL;
	int i, success;

	for (i = 0; i < DISCRETE_COUNT; i++) {
		self->domain[i] = NULL;
		self->discrete[i] = NULL;
	}
	self->rate = -FLT_MAX;
	self->time_to_live = NO_TTL;
	if (!PyArg_ParseTuple(args, "|fOOf:__init__",
		&self->rate, &ptemplate, &pdeviation, &self->time_to_live))
		return -1;

	if (kwargs != NULL) {
		if (!Emitter_parse_kwargs(self, &ptemplate, &pdeviation, kwargs))
			return -1;
	}

	if (self->rate == -FLT_MAX) {
		self->rate = 0;
	} else if (self->rate < 0) {
		PyErr_SetString(PyExc_ValueError, "StaticEmitter: Expected rate >= 0");
		return -1;
	}

	if (ptemplate != NULL) {
		success = Emitter_fill_particle_from(&self->ptemplate, ptemplate);
		Py_DECREF(ptemplate);
		if (!success)
			goto error;
	}

	if (pdeviation != NULL) {
		success = Emitter_fill_particle_from(&self->pdeviation, pdeviation);
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
		if (!Vec3_FromSequence(vec, v)) {
			Py_DECREF(v);
			return 0;
		}
		Py_DECREF(v);
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
		if (!Color_FromSequence(color, v)) {
			Py_DECREF(v);
			return 0;
		}
		Py_DECREF(v);
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
			*f = (float)PyFloat_AS_DOUBLE(pyfloat);
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

EXTERN_INLINE void
Vec3_deviate(Vec3 *dest, Vec3 *deviation)
{
	dest->x = deviation->x ? rand_norm(dest->x, deviation->x) : dest->x;
	dest->y = deviation->y ? rand_norm(dest->y, deviation->y) : dest->y;
	dest->z = deviation->z ? rand_norm(dest->z, deviation->z) : dest->z;
}

EXTERN_INLINE void
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
static int
Emitter_make_particle(StaticEmitterObject *self, Particle *p)
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
		if (!Emitter_make_particle(self, &pgroup->plist->p[pindex])) {
			Py_DECREF(result);
			return NULL;
		}
		count--;
	}
	self->partial = count;

	return result;
}

static PyObject *
Emitter_emit(StaticEmitterObject *self, PyObject *args)
{
	long count;
	GroupObject *pgroup;
	long pindex;

	if (!PyArg_ParseTuple(args, "lO:emit", &count, &pgroup))
		return NULL;

	if (!GroupObject_Check(pgroup))
		return NULL;

	/* Clamp to zero to gracefully handle random input values that
	 * occasionally go negative */
	if (count < 0)
        count = 0;

	for (; count > 0; count--) {
		pindex = Group_new_p(pgroup);
		if (pindex < 0) {
			PyErr_NoMemory();
			return NULL;
		}
		if (!Emitter_make_particle(self, &pgroup->plist->p[pindex])) {
			return NULL;
		}
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMemberDef StaticEmitter_members[] = {
    {"rate", T_FLOAT, offsetof(StaticEmitterObject, rate), 0,
        "Rate of particle emission per unit time"},
    {"time_to_live", T_FLOAT, offsetof(StaticEmitterObject, time_to_live), 0,
        "Time remaining before emitter is removed from the group (-1 to disable)"},
	{NULL}
};

static PyMethodDef StaticEmitter_methods[] = {
	{"emit", (PyCFunction)Emitter_emit, METH_VARARGS,
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
		return PY_FIND_METHOD(StaticEmitter_methods, self, o);
	}
}

PyDoc_STRVAR(StaticEmitter__doc__,
	"Creates particles in a group at a fixed rate deriving particle\n"
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
	PyVarObject_HEAD_INIT(NULL, 0)
	"emitter.StaticEmitter",		/*tp_name*/
	sizeof(StaticEmitterObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)Emitter_dealloc, /*tp_dealloc*/
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
	StaticEmitter_methods,  /*tp_methods*/
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

static PyTypeObject PerParticleEmitter_Type;

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
	GroupObject *source_group;
} PerParticleEmitterObject;

static int
PerParticleEmitter_init(PerParticleEmitterObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *ptemplate = NULL, *pdeviation = NULL;
	int i, success;

	for (i = 0; i < DISCRETE_COUNT; i++) {
		self->domain[i] = NULL;
		self->discrete[i] = NULL;
	}
	self->rate = -FLT_MAX;
	self->time_to_live = NO_TTL;
	if (!PyArg_ParseTuple(args, "O|fOOf:__init__",
		&self->source_group, &self->rate, &ptemplate, &pdeviation, &self->time_to_live))
		return -1;

	if (!GroupObject_Check(self->source_group))
		return -1;

	if (kwargs != NULL) {
		if (!Emitter_parse_kwargs((StaticEmitterObject *)self, &ptemplate, &pdeviation, kwargs))
			return -1;
	}

	if (self->rate == -FLT_MAX) {
		self->rate = 0;
	} else if (self->rate < 0) {
		PyErr_SetString(PyExc_ValueError, "PerParticleEmitter: Expected rate >= 0");
		return -1;
	}

	if (ptemplate != NULL) {
		success = Emitter_fill_particle_from(&self->ptemplate, ptemplate);
		Py_DECREF(ptemplate);
		if (!success)
			goto error;
	}

	if (pdeviation != NULL) {
		success = Emitter_fill_particle_from(&self->pdeviation, pdeviation);
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
	return 1;
}

static PyObject *
PerParticleEmitter_call(PerParticleEmitterObject *self, PyObject *args)
{
	float td;
	GroupObject *pgroup;
	float count, remaining;
	long pindex, total = 0;
	Particle *p;
	unsigned long pcount;
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
	remaining = count;

	if (count >= 1.0f) {
		p = self->source_group->plist->p;
		pcount = GroupObject_ActiveCount(self->source_group);

		while (pcount--) {
			if (Particle_IsAlive(*p)) {
				remaining = count;
				Vec3_copy(&self->ptemplate.position, &p->position);

				while (remaining >= 1.0f) {
					pindex = Group_new_p(pgroup);
					if (pindex < 0) {
						PyErr_NoMemory();
						return NULL;
					}
					if (!Emitter_make_particle(
						(StaticEmitterObject *)self, &pgroup->plist->p[pindex]))
						return NULL;
					remaining--;
				}
				total += (long)count;
			}
			p++;
		}
		self->partial = remaining;
	} else {
		self->partial = count;
	}

	return PyInt_FromLong(total);
}

static PyObject *
PerParticleEmitter_emit(PerParticleEmitterObject *self, PyObject *args)
{
	long count, remaining;
	unsigned long pcount;
	GroupObject *pgroup;
	Particle *p;
	long pindex;

	if (!PyArg_ParseTuple(args, "lO:emit", &count, &pgroup))
		return NULL;

	if (!GroupObject_Check(pgroup))
		return NULL;

	/* Clamp to zero to gracefully handle random input values that
	 * occasionally go negative */
	if (count <= 0)
        count = 0;

	p = self->source_group->plist->p;
	pcount = GroupObject_ActiveCount(self->source_group);

	for (; pcount > 0; pcount--, p++) {
		if (Particle_IsAlive(*p)) {
			Vec3_copy(&self->ptemplate.position, &p->position);
			for (remaining = count; remaining > 0; remaining--) {
				pindex = Group_new_p(pgroup);
				if (pindex < 0) {
					PyErr_NoMemory();
					return NULL;
				}
				if (!Emitter_make_particle(
					(StaticEmitterObject *)self, &pgroup->plist->p[pindex])) {
					return NULL;
				}
			}
		}
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMemberDef PerParticleEmitter_members[] = {
    {"rate", T_FLOAT, offsetof(PerParticleEmitterObject, rate), 0,
        "Number of particles per particle in the source group emitted per unit time"},
    {"time_to_live", T_FLOAT, offsetof(PerParticleEmitterObject, time_to_live), 0,
        "Time remaining before emitter is removed from the group (-1 to disable)"},
    {"source_group", T_OBJECT, offsetof(PerParticleEmitterObject, source_group), 0,
        "Source particle group containing template particles"},
	{NULL}
};

static PyMethodDef PerParticleEmitter_methods[] = {
	{"emit", (PyCFunction)PerParticleEmitter_emit, METH_VARARGS,
		PyDoc_STR("emit(count, group) -> None\n"
			"Emit count new particles per source particle into the\n"
			"group specified. This call is not affected by the emitter\n"
			"rate or time to live values.")},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
PerParticleEmitter_getattr(PerParticleEmitterObject *self, PyObject *o)
{
	char *name = PyString_AS_STRING(o);
	if (!strcmp(name, "template")) {
		return (PyObject *)ParticleRefObject_New(NULL, &self->ptemplate);
	} else if (!strcmp(name, "deviation")) {
		return (PyObject *)ParticleRefObject_New(NULL, &self->ptemplate);
	} else if (!strcmp(name, "rate")) {
		return PyMember_GetOne((char *)self, &PerParticleEmitter_members[0]);
	} else if (!strcmp(name, "time_to_live")) {
		return PyMember_GetOne((char *)self, &PerParticleEmitter_members[1]);
	} else if (!strcmp(name, "source_group")) {
		return PyMember_GetOne((char *)self, &PerParticleEmitter_members[2]);
	} else {
		return PY_FIND_METHOD(PerParticleEmitter_methods, self, o);
	}
}

PyDoc_STRVAR(PerParticleEmitter__doc__,
	"Creates particles in a group for each particle in a source group at\n"
	"a fixed rate. Particle attributes are derived from a configurable mix\n"
	"of source particle, domain, discrete value lists and fixed template \n"
	"particles plus random deviation\n\n"
	"PerParticleEmitter(source_group, rate, template=None, deviation=None,\n"
	"    time_to_live=None, **discrete)\n\n"
	"source_group -- Source particles used as templates for new particles\n"
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

static PyTypeObject PerParticleEmitter_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyVarObject_HEAD_INIT(NULL, 0)
	"emitter.PerParticleEmitter",		/*tp_name*/
	sizeof(PerParticleEmitterObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)Emitter_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,   /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	(ternaryfunc)PerParticleEmitter_call, /*tp_call*/
	0,                      /*tp_str*/
	(getattrofunc)PerParticleEmitter_getattr, /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	PerParticleEmitter__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	PerParticleEmitter_methods,  /*tp_methods*/
	PerParticleEmitter_members,  /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)PerParticleEmitter_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

MOD_INIT(emitter)
{
	PyObject *m;

	/* Bind tp_new and tp_alloc here to appease certain compilers */
	StaticEmitter_Type.tp_alloc = PyType_GenericAlloc;
	StaticEmitter_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&StaticEmitter_Type) < 0)
		return MOD_ERROR_VAL;

	PerParticleEmitter_Type.tp_alloc = PyType_GenericAlloc;
	PerParticleEmitter_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&PerParticleEmitter_Type) < 0)
		return MOD_ERROR_VAL;

	/* Create the module and add the types */
	MOD_DEF(m, "emitter", "Particle Emitters", NULL);
	if (m == NULL)
		return MOD_ERROR_VAL;

	Py_INCREF(&StaticEmitter_Type);
	PyModule_AddObject(m, "StaticEmitter", (PyObject *)&StaticEmitter_Type);
	Py_INCREF(&PerParticleEmitter_Type);
	PyModule_AddObject(m, "PerParticleEmitter", (PyObject *)&PerParticleEmitter_Type);

	rand_seed((unsigned long)time(NULL));

    return MOD_SUCCESS_VAL(m);
}
