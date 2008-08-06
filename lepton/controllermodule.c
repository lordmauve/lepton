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
/* Native-code controllers
 *
 * $Id $ */

#include <Python.h>
#include <structmember.h>
#include <float.h>

#include "group.h"
#include "vector.h"

#define clamp(n, min, max) \
	((n) < (min) ? (min) : ((n) > (max) ? (max) : (n)))

static PyTypeObject GravityController_Type;

typedef struct {
	PyObject_HEAD
	Vec3 gravity;
} GravityControllerObject;

static void
GravityController_dealloc(GravityControllerObject *self) {
	PyObject_Del(self);
}

static int
GravityController_init(GravityControllerObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, "(fff):__init__",
		&self->gravity.x, &self->gravity.y, &self->gravity.z))
		return -1;
	return 0;
}

static PyObject *
GravityController_call(GravityControllerObject *self, PyObject *args)
{
	float td;
	GroupObject *pgroup;
	register Particle *p;
	Vec3 g;
	register unsigned long count;

	if (!PyArg_ParseTuple(args, "fO:__init__", &td, &pgroup))
		return NULL;
	
	if (!GroupObject_Check(pgroup))
		return NULL;

	p = pgroup->plist->p;
	g.x = self->gravity.x * td;
	g.y = self->gravity.y * td;
	g.z = self->gravity.z * td;
	count = GroupObject_ActiveCount(pgroup);
	while (count--) {
		Vec3_add(&p->velocity, &p->velocity, &g);
		p++;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(GravityController__doc__, 
	"Imparts a fixed accelleration to all particles\n\n"
	"Gravity((gx, gy, gz))\n\n"
	"(gx, gy, gz) -- Gravity vector");

static PyTypeObject GravityController_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"controller.Gravity",		/*tp_name*/
	sizeof(GravityControllerObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)GravityController_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	(ternaryfunc)GravityController_call, /*tp_call*/
	0,                      /*tp_str*/
	PyObject_GenericGetAttr, /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	GravityController__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	0,  /*tp_methods*/
	0,  /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)GravityController_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject MovementController_Type;

typedef struct {
	PyObject_HEAD
	Vec3 damping;
	float min_velocity;
	float max_velocity;
} MovementControllerObject;

static void
MovementController_dealloc(MovementControllerObject *self) {
	PyObject_Del(self);
}

static int
MovementController_init(MovementControllerObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *damping_arg = NULL;

	static char *kwlist[] = {"damping", "min_velocity", "max_velocity", NULL};

	self->min_velocity = 0;
	self->max_velocity = FLT_MAX;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Off:__init__", kwlist,
		&damping_arg, &self->min_velocity, &self->max_velocity))
		return -1;
	
	if (self->min_velocity < 0) {
		PyErr_SetString(PyExc_ValueError, "Movement: expected min_velocity >= 0");
		return -1;
	}

	if (self->max_velocity < 0) {
		PyErr_SetString(PyExc_ValueError, "Movement: expected max_velocity >= 0");
		return -1;
	}

	if (self->max_velocity < self->min_velocity) {
		PyErr_SetString(PyExc_ValueError, 
			"Movement: expected max_velocity >= min_velocity");
		return -1;
	}
	
	if (damping_arg != NULL) {
		if (PySequence_Check(damping_arg)) {
			damping_arg = PySequence_Tuple(damping_arg);
			if (damping_arg == NULL)
				return -1;
			if (!PyArg_ParseTuple(damping_arg, "fff", 
				&self->damping.x, &self->damping.y, &self->damping.z)) {
				Py_DECREF(damping_arg);
				return -1;
			}
			Py_DECREF(damping_arg);
		} else {
			/* scalar damping value */
			damping_arg = PyNumber_Float(damping_arg);
			if (damping_arg == NULL)
				return -1;
			self->damping.x = PyFloat_AS_DOUBLE(damping_arg);
			self->damping.y = PyFloat_AS_DOUBLE(damping_arg);
			self->damping.z = PyFloat_AS_DOUBLE(damping_arg);
			Py_DECREF(damping_arg);
		}
	} else {
		self->damping.x = 1.0f;
		self->damping.y = 1.0f;
		self->damping.z = 1.0f;
	}
	return 0;
}

static PyObject *
MovementController_call(MovementControllerObject *self, PyObject *args)
{
	float td;
	GroupObject *pgroup;
	register Particle *p;
	Vec3 v;
	float min_v, min_v_sq, max_v, max_v_sq, v_sq, v_adj;
	register unsigned long count;

	if (!PyArg_ParseTuple(args, "fO:__init__", &td, &pgroup))
		return NULL;
	
	if (!GroupObject_Check(pgroup))
		return NULL;

	p = pgroup->plist->p;
	min_v = self->min_velocity;
	min_v_sq = min_v * min_v;
	max_v = self->max_velocity;
	if (max_v != FLT_MAX)
		max_v_sq = max_v * max_v;
	else
		max_v_sq = FLT_MAX;
	count = GroupObject_ActiveCount(pgroup);

	if (self->damping.x == 1.0f && 
		self->damping.y == 1.0f && 
		self->damping.z == 1.0f && 
		max_v == FLT_MAX && min_v == 0) {
		/* simple case, no damping or velocity bounds */
		while (count--) {
			Vec3_scalar_mul(&v, &p->velocity, td);
			Vec3_add(&p->position, &p->position, &v);
			p++;
		}
	} else {
		while (count--) {
			Vec3_mul(&p->velocity, &p->velocity, &self->damping);
			v_sq = Vec3_len_sq(&p->velocity);
			if (v_sq > max_v_sq) {
				v_adj = max_v * InvSqrt(v_sq);
				Vec3_scalar_mul(&p->velocity, &p->velocity, v_adj);
			} else if (v_sq < min_v_sq && v_sq > 0) {
				v_adj = min_v * InvSqrt(v_sq);
				Vec3_scalar_mul(&p->velocity, &p->velocity, v_adj);
			}
			Vec3_scalar_mul(&v, &p->velocity, td);
			Vec3_add(&p->position, &p->position, &v);
			p++;
		}
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(MovementController__doc__, 
	"Updates particle position and velocity\n\n"
	"Movement(damping=None, min_velocity=None, max_velocity=None)\n\n"
	"damping -- A three-tuple velocity multiplier per unit time\n"
	"in each respective dimension. A scalar may also be specified\n"
	"if the damping is equal in all dimensions (thus .25 is equivilent\n"
	"to (.25, .25. .25))\n\n"
	"min_velocity -- Minimum velocity scalar. Particle velocities\n"
	"less than this are scaled up, except for particles with no\n"
	"velocity since their velocity vectors have no direction\n\n"
	"max_velocity -- Maximum velocity scalar. Particle velocity\n"
	"magnitudes are clamped to this value");

static PyTypeObject MovementController_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"controller.Movement",		/*tp_name*/
	sizeof(MovementControllerObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)MovementController_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	(ternaryfunc)MovementController_call, /*tp_call*/
	0,                      /*tp_str*/
	PyObject_GenericGetAttr, /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	MovementController__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	0,  /*tp_methods*/
	0,  /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)MovementController_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject LifetimeController_Type;

typedef struct {
	PyObject_HEAD
	float max_age;
} LifetimeControllerObject;

static void
LifetimeController_dealloc(LifetimeControllerObject *self) {
	PyObject_Del(self);
}

static int
LifetimeController_init(LifetimeControllerObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"max_age", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "f:__init__", kwlist,
		&self->max_age))
		return -1;
	return 0;
}

static PyObject *
LifetimeController_call(LifetimeControllerObject *self, PyObject *args)
{
	float td, max_age;
	GroupObject *pgroup;
	register Particle *p;
	register unsigned long i, count;

	if (!PyArg_ParseTuple(args, "fO:__init__", &td, &pgroup))
		return NULL;
	
	if (!GroupObject_Check(pgroup))
		return NULL;

	p = pgroup->plist->p;
	max_age = self->max_age;
	count = GroupObject_ActiveCount(pgroup);
	for (i = 0; i < count; i++) {
		if (p[i].age > max_age)
			Group_kill_p(pgroup, i);
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(LifetimeController__doc__, 
	"Kills particles beyond an age threshold\n\n"
	"Lifetime(max_age)\n\n"
	"max_age -- Age threshold, particles older than this are killed.");

static PyTypeObject LifetimeController_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"controller.Lifetime",		/*tp_name*/
	sizeof(LifetimeControllerObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)LifetimeController_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	(ternaryfunc)LifetimeController_call, /*tp_call*/
	0,                      /*tp_str*/
	PyObject_GenericGetAttr, /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	LifetimeController__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	0,  /*tp_methods*/
	0,  /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)LifetimeController_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject ColorBlenderController_Type;

typedef struct {
	PyObject_HEAD
	float min_age;
	float max_age;
	unsigned long resolution;
	unsigned long length;
	Color *gradient;
} ColorBlenderControllerObject;

static void
ColorBlenderController_dealloc(ColorBlenderControllerObject *self) {
	if (self->gradient != NULL) {
		PyMem_Free(self->gradient);
		self->gradient = NULL;
	}
	PyObject_Del(self);
}

static int
ColorBlenderController_init(ColorBlenderControllerObject *self, PyObject *args)
{
	PyObject *color_times = NULL, *item = NULL, *color_tup = NULL;
	float last_time, next_time, iter_time, t;
	Color last_color, next_color, grad, *color;
	Py_ssize_t color_times_len;
	unsigned long cti, i, count;

	self->resolution = 30;
	if (!PyArg_ParseTuple(args, "O|k:__init__", &color_times, &self->resolution))
		return -1;

	if (!PySequence_Check(color_times)) {
		PyErr_SetString(PyExc_ValueError, 
			"ColorBlender: expected sequence for color_times");
		return -1;
	}
	
	color_times_len = PySequence_Length(color_times);
	if (color_times_len < 2) {
		PyErr_SetString(PyExc_ValueError, 
			"ColorBlender: color_times sequence must have at least 2 elements");
		return -1;
	}

	color_times = PySequence_List(color_times);
	if (color_times == NULL)
		return -1;
	if (PyList_Sort(color_times) == -1)
		goto error;

	item = PyList_GET_ITEM(color_times, 0);
	color_tup = PySequence_Tuple(item);
	if (color_tup == NULL)
		return -1;
	last_color.a = 1.0f;
	if (!PyArg_ParseTuple(color_tup, "f(ffff)",
		&self->min_age, &last_color.r, &last_color.g, &last_color.b, &last_color.a))
		goto error;
	Py_DECREF(color_tup);

	item = PyList_GET_ITEM(color_times, color_times_len - 1);
	color_tup = PySequence_Tuple(item);
	if (color_tup == NULL)
		return -1;
	if (!PyArg_ParseTuple(color_tup, "f(ffff)",
		&self->max_age, &next_color.r, &next_color.g, &next_color.b, &next_color.a))
		goto error;
	Py_DECREF(color_tup);

	if (self->min_age == self->max_age) {
		PyErr_SetString(PyExc_ValueError, 
			"ColorBlender: color_times sequence contains duplicate times");
		return -1;
	}

	self->length = (self->max_age - self->min_age) * self->resolution;
	if (self->length == 0) {
		PyErr_SetString(PyExc_ValueError, 
			"ColorBlender: color_times interval too short for resolution");
		return -1;
	}
	self->gradient = PyMem_New(Color, self->length);
	if (self->gradient == NULL) {
		PyErr_NoMemory();
		return -1;
	}

	/* Create color gradient array from color_times */
	last_time = self->min_age;
	color = self->gradient;
	count = self->length;
	i = 0;

	for (cti = 1; cti < color_times_len; cti++) {
		item = PyList_GET_ITEM(color_times, cti);
		color_tup = PySequence_Tuple(item);
		if (color_tup == NULL)
			goto error;
		next_color.a = 1.0f;
		if (!PyArg_ParseTuple(color_tup, "f(ffff)",
			&next_time, &next_color.r, &next_color.g, &next_color.b, &next_color.a))
			goto error;
		Py_CLEAR(color_tup);

		if (last_time == next_time) {
			PyErr_SetString(PyExc_ValueError, 
				"ColorBlender: color_times sequence contains duplicate times");
			goto error;
		}
		
		grad.r = next_color.r - last_color.r;
		grad.g = next_color.g - last_color.g;
		grad.b = next_color.b - last_color.b;
		grad.a = next_color.a - last_color.a;
		iter_time = 1.0f / ((next_time - last_time) * (float)self->resolution);
		for (i = 0; i < (next_time - last_time) * self->resolution - 1; i++) {
			/* interpolate the colors over time */
			if (--count <= 0) {
				PyErr_Format(PyExc_RuntimeError,
					"ColorBlender (BUG): Overrun creating color gradient: "
					"length=%d res=%d cti=%d iter=%d count=%d", 
					(int)self->length,  (int)self->resolution, (int)cti, (int)i, 
					(int)((next_time - last_time) * self->resolution));
				goto error;
			}
				
			t = (float)i * iter_time;
			color->r = last_color.r + grad.r * t;
			color->g = last_color.g + grad.g * t;
			color->b = last_color.b + grad.b * t;
			color->a = last_color.a + grad.a * t;
			color++;
		}

		last_color.r = next_color.r;
		last_color.g = next_color.g;
		last_color.b = next_color.b;
		last_color.a = next_color.a;
		last_time = next_time;
	}
	return 0;

error:
	Py_XDECREF(color_times);
	Py_XDECREF(color_tup);
	if (self->gradient != NULL) {
		PyMem_Free(self->gradient);
		self->gradient = NULL;
	}
	return -1;
}

static PyObject *
ColorBlenderController_call(ColorBlenderControllerObject *self, PyObject *args)
{
	float td, min_age, max_age;
	unsigned long resolution;
	GroupObject *pgroup;
	Color *gradient;
	register Particle *p;
	register unsigned long count, g;

	if (!PyArg_ParseTuple(args, "fO:__init__", &td, &pgroup))
		return NULL;
	
	if (!GroupObject_Check(pgroup))
		return NULL;

	p = pgroup->plist->p;
	min_age = self->min_age;
	max_age = self->max_age;
	resolution = self->resolution;
	gradient = self->gradient;
	count = GroupObject_ActiveCount(pgroup);
	while (count--) {
		if (p->age >= min_age && p->age <= max_age) {
			g = (p->age - min_age) * resolution;
			p->color.r = gradient[g].r;
			p->color.g = gradient[g].g;
			p->color.b = gradient[g].b;
			p->color.a = gradient[g].a;
		}
		p++;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(ColorBlenderController__doc__, 
	"Changes particle color over time\n\n"
	"ColorBlender(color_times, resolution=30)\n\n"
	"color_times -- A sequence of 2 or more (time, color) pairs where color\n"
	"is a 3 or 4 tuple of floats representing the color and time is the\n"
	"particle age when it is assigned that color. This list is used to create\n"
	"a gradient of colors for the particles over time. If a particle's age\n"
	"is less than the smallest time or greater than the latest time, its\n"
	"color is not changed. Note that the time values can be specified in any\n"
	"order, but a particular time value should occur only once, a ValueError\n"
	"is raised if a time value is repeated\n\n"
	"resolution -- The number of colors per unit time in the cached gradient.\n"
	"A larger resolution will result in smoother color blending, but at\n"
	"the cost of memory and performance. A reasonable value is the expected\n"
	"frame rate of your application, or a lower value if the age span of\n"
	"color_times is especially long or if the color changes are not rapid.\n"
	"In general, pick the lowest value that gives acceptable visual results");

static PyTypeObject ColorBlenderController_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"controller.ColorBlender",		/*tp_name*/
	sizeof(ColorBlenderControllerObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)ColorBlenderController_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	(ternaryfunc)ColorBlenderController_call, /*tp_call*/
	0,                      /*tp_str*/
	PyObject_GenericGetAttr, /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	ColorBlenderController__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	0,  /*tp_methods*/
	0,  /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)ColorBlenderController_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

PyMODINIT_FUNC
init_controller(void)
{
	PyObject *m;

	/* Bind tp_new and tp_alloc here to appease certain compilers */
	GravityController_Type.tp_alloc = PyType_GenericAlloc;
	GravityController_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&GravityController_Type) < 0)
		return;

	/* Bind tp_new and tp_alloc here to appease certain compilers */
	MovementController_Type.tp_alloc = PyType_GenericAlloc;
	MovementController_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&MovementController_Type) < 0)
		return;

	/* Bind tp_new and tp_alloc here to appease certain compilers */
	LifetimeController_Type.tp_alloc = PyType_GenericAlloc;
	LifetimeController_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&LifetimeController_Type) < 0)
		return;

	/* Bind tp_new and tp_alloc here to appease certain compilers */
	ColorBlenderController_Type.tp_alloc = PyType_GenericAlloc;
	ColorBlenderController_Type.tp_new = PyType_GenericNew;
	if (PyType_Ready(&ColorBlenderController_Type) < 0)
		return;

	/* Create the module and add the types */
	m = Py_InitModule3("_controller", NULL, "Particle Controllers");
	if (m == NULL)
		return;

	Py_INCREF(&GravityController_Type);
	PyModule_AddObject(m, "Gravity", (PyObject *)&GravityController_Type);
	Py_INCREF(&MovementController_Type);
	PyModule_AddObject(m, "Movement", (PyObject *)&MovementController_Type);
	Py_INCREF(&LifetimeController_Type);
	PyModule_AddObject(m, "Lifetime", (PyObject *)&LifetimeController_Type);
	Py_INCREF(&ColorBlenderController_Type);
	PyModule_AddObject(m, "ColorBlender", (PyObject *)&ColorBlenderController_Type);
}
