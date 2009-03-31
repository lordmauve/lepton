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
	0,                      /*tp_getattro*/
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

	self->min_velocity  = 0.0;
	self->max_velocity  = FLT_MAX;
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
			self->damping.x = (float)PyFloat_AS_DOUBLE(damping_arg);
			self->damping.y = (float)PyFloat_AS_DOUBLE(damping_arg);
			self->damping.z = (float)PyFloat_AS_DOUBLE(damping_arg);
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
			Vec3_addi(&p->position, &v);
			Vec3_scalar_mul(&v, &p->rotation, td);
			Vec3_addi(&p->up, &v);
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
			Vec3_addi(&p->position, &v);
			Vec3_scalar_mul(&v, &p->rotation, td);
			Vec3_addi(&p->up, &v);
			p++;
		}
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMemberDef MovementControllerController_members[] = {
    {"min_velocity", T_FLOAT, offsetof(MovementControllerObject, min_velocity), 0,
        "Minimum particle velocity magnitude. All moving particles\n"
		"with velocities less than this minimum are sped up to min_velocity."},
    {"max_velocity", T_FLOAT, offsetof(MovementControllerObject, max_velocity), 0,
        "Maximum velocity magnitude, particle velocities magnitudes\n"
		"are clamped to this value."},
	{NULL}
};

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
	0,                      /*tp_getattro*/
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
	MovementControllerController_members,  /*tp_members*/
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

static PyTypeObject FaderController_Type;

typedef struct {
	PyObject_HEAD
	float start_alpha;
	float fade_in_start;
	float fade_in_end;
	float max_alpha;
	float fade_out_start;
	float fade_out_end;
	float end_alpha;
	float min_velocity;
	float max_velocity;
} FaderControllerObject;

static void
FaderController_dealloc(FaderControllerObject *self) {
	PyObject_Del(self);
}

static int
FaderController_init(FaderControllerObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"start_alpha", "fade_in_start", "fade_in_end", "max_alpha", "fade_out_start", "fade_out_end", "end_alpha", NULL};
	self->start_alpha   = 0.0; 
	self->fade_in_start = 0.0; 
	self->fade_in_end   = 0.0;
	self->max_alpha     = 1.0;
	self->end_alpha     = 0.0;
	self->fade_out_start = FLT_MAX;
	self->fade_out_end = FLT_MAX;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|fffffff:__init__", kwlist,
		&self->start_alpha, &self->fade_in_start, &self->fade_in_end, &self->max_alpha, 
		&self->fade_out_start, &self->fade_out_end, &self->end_alpha))
		return -1;

	return 0;
}

static struct PyMemberDef FaderControllerController_members[] = {
    {"start_alpha", T_FLOAT, offsetof(FaderControllerObject, start_alpha), 0,
        "Initial particle alpha value."},
    {"fade_in_start", T_FLOAT, offsetof(FaderControllerObject, fade_in_start), 0,
        "Time to start fading in to max_alpha."},
    {"fade_in_end", T_FLOAT, offsetof(FaderControllerObject, fade_in_end), 0,
        "Time when alpha reaches max_alpha."},
    {"max_alpha", T_FLOAT, offsetof(FaderControllerObject, fade_in_end), 0,
        "Max particle alpha value."},
    {"fade_out_start", T_FLOAT, offsetof(FaderControllerObject, fade_out_start), 0,
        "Time to start fading out to end_alpha."},
    {"fade_out_end", T_FLOAT, offsetof(FaderControllerObject, fade_out_end), 0,
        "Time when alpha reaches end_alpha"},
    {"end_alpha", T_FLOAT, offsetof(FaderControllerObject, end_alpha), 0,
        "Ending particle alpha level."},
	{NULL}
};

PyDoc_STRVAR(FaderController__doc__, 
	"Alters the alpha of particles to fade them in and out over time\n\n"
	"start_alpha -- Initial alpha value.\n"
	"fade_in_start -- Time to start fading in to max_alpha\n"
	"fade_in_end -- Time when alpha reaches max\n"
	"max_alpha -- Maximum alpha level\n"
	"fade_out_start -- Time to start fading out to end_alpha. If None\n"
	"no fade out is performed.\n"
	"fade_out_end -- Time when alpha reaches end.\n"
	"end_alpha -- Ending alpha level.\n");

static PyObject *
FaderController_call(FaderControllerObject *self, PyObject *args)
{
	float td;
	GroupObject *pgroup;
	register Particle *p;
	float in_start, in_end, in_time, in_alpha, out_start, out_end, out_time, out_alpha;
	register unsigned long count;

	if (!PyArg_ParseTuple(args, "fO:__init__", &td, &pgroup))
		return NULL;
	
	if (!GroupObject_Check(pgroup))
		return NULL;

	p = pgroup->plist->p;
	in_start = self->fade_in_start;
	in_end = self->fade_in_end;
	in_time = in_end - in_start;
	in_alpha = self->max_alpha - self->start_alpha;
	out_start = self->fade_out_start;
	out_end = self->fade_out_end;
	out_time = out_end - out_start;
	out_alpha = self->end_alpha - self->max_alpha;
	count = GroupObject_ActiveCount(pgroup);
	while (count--) {
		if ((p->age > in_end) && (p->age <= out_start)) {
			p->color.a = self->max_alpha;
		} else if ( (p->age > in_start) && (p->age < in_end)) {
			p->color.a = self->start_alpha + in_alpha * ((p->age - in_start) / in_time);
		} else if ((p->age >= out_start) && (p->age < out_end)) {
			p->color.a = self->max_alpha + out_alpha * ((p->age - out_start) / out_time);
		} else if (p->age >= out_end) {
			p->color.a = self->end_alpha;
		}
		p++;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyTypeObject FaderController_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"controller.Fader",		/*tp_name*/
	sizeof(FaderControllerObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)FaderController_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	(ternaryfunc)FaderController_call, /*tp_call*/
	0,                      /*tp_str*/
	0,                      /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	FaderController__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	0,  /*tp_methods*/
	FaderControllerController_members,  /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)FaderController_init, /*tp_init*/
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
	register unsigned long count;

	if (!PyArg_ParseTuple(args, "fO:__init__", &td, &pgroup))
		return NULL;
	
	if (!GroupObject_Check(pgroup))
		return NULL;

	p = pgroup->plist->p;
	max_age = self->max_age;
	count = GroupObject_ActiveCount(pgroup);
	while (count--) {
		if (p->age > max_age)
			Group_kill_p(pgroup, p);
		p++;
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
	0,                      /*tp_getattro*/
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
	Py_ssize_t color_times_len, cti;
	unsigned long i, count;

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

	self->length = (unsigned long)((self->max_age - self->min_age) * self->resolution);
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
			g = (unsigned long)((p->age - min_age) * resolution);
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

static struct PyMemberDef ColorBlenderController_members[] = {
    {"resolution", T_ULONG, offsetof(ColorBlenderControllerObject, resolution), READONLY,
        "The number of colors per unit time in the cached gradient."},
	{NULL}
};

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
	0,                      /*tp_getattro*/
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
	ColorBlenderController_members,  /*tp_members*/
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

/* --------------------------------------------------------------------- */

static PyTypeObject GrowthController_Type;

typedef struct {
	PyObject_HEAD
	Vec3 growth;
	Vec3 damping;
} GrowthControllerObject;

static void
GrowthController_dealloc(GrowthControllerObject *self) {
	PyObject_Del(self);
}

static int
GrowthController_init(GrowthControllerObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *damping_arg = NULL;
	PyObject *growth_arg = NULL;

	static char *kwlist[] = {"growth", "damping", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:__init__", kwlist,
		&growth_arg, &damping_arg))
		return -1;

	if (PySequence_Check(growth_arg)) {
		growth_arg = PySequence_Tuple(growth_arg);
		if (growth_arg == NULL)
			return -1;
		if (!PyArg_ParseTuple(growth_arg, "fff", 
			&self->growth.x, &self->growth.y, &self->growth.z)) {
			Py_DECREF(growth_arg);
			return -1;
		}
	} else {
		/* scalar growth value */
		growth_arg = PyNumber_Float(growth_arg);
		if (growth_arg == NULL)
			return -1;
		self->growth.x = (float)PyFloat_AS_DOUBLE(growth_arg);
		self->growth.y = (float)PyFloat_AS_DOUBLE(growth_arg);
		self->growth.z = (float)PyFloat_AS_DOUBLE(growth_arg);
	}
	Py_DECREF(growth_arg);

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
			self->damping.x = (float)PyFloat_AS_DOUBLE(damping_arg);
			self->damping.y = (float)PyFloat_AS_DOUBLE(damping_arg);
			self->damping.z = (float)PyFloat_AS_DOUBLE(damping_arg);
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
GrowthController_call(GrowthControllerObject *self, PyObject *args)
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
	g.x = self->growth.x * td;
	g.y = self->growth.y * td;
	g.z = self->growth.z * td;
	count = GroupObject_ActiveCount(pgroup);
	while (count--) {
		Vec3_addi(&p->size, &g);
		p++;
	}
	Vec3_muli(&self->growth, &self->damping);
	
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(GrowthController__doc__, 
	"Changes the size of particles over time\n\n"
	"Growth(growth, damping=1.0)\n\n"
	"growth -- Change in particle size per unit time.\n"
	"May be specified as a 3-tuple or scalar\n\n"
	"damping -- Growth multiplier to accelerate or\n"
	"decelerate growth over time. Also a 3-tuple or scalar.");

static PyTypeObject GrowthController_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"controller.Growth",		/*tp_name*/
	sizeof(GrowthControllerObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)GrowthController_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	(ternaryfunc)GrowthController_call, /*tp_call*/
	0,                      /*tp_str*/
	0,                      /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	GrowthController__doc__,   /*tp_doc*/
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
	(initproc)GrowthController_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject CollectorController_Type;

typedef struct {
	PyObject_HEAD
	PyObject *domain;
	int collect_inside;
	int collected_count;
	PyObject *callback;
} CollectorControllerObject;

static void
CollectorController_dealloc(CollectorControllerObject *self) {
	if (self->domain !=NULL)
		Py_CLEAR(self->domain);
	if (self->callback !=NULL)
		Py_CLEAR(self->callback);
	PyObject_Del(self);
}

static int
CollectorController_init(CollectorControllerObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"domain", "collect_inside", "callback", NULL};

	self->callback = NULL;
	self->collect_inside = 1;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iO:__init__", kwlist,
		&self->domain, &self->collect_inside, &self->callback))
		return -1;
	Py_INCREF(self->domain);
	if (self->callback != NULL)
		Py_INCREF(self->callback);
	self->collected_count = 0;
	return 0;
}

static PyObject *
CollectorController_call(CollectorControllerObject *self, PyObject *args)
{
	float td;
	GroupObject *pgroup;
	VectorObject *vector = NULL;
	ParticleRefObject *particleref = NULL;
	PyObject *result;
	int in_domain, collect_inside;
	register Particle *p;
	register unsigned long count;

	if (!PyArg_ParseTuple(args, "fO:__init__", &td, &pgroup))
		return NULL;
	
	if (!GroupObject_Check(pgroup))
		return NULL;

	collect_inside = self->collect_inside ? 1 : 0;
	p = pgroup->plist->p;
	count = GroupObject_ActiveCount(pgroup);
	vector = Vector_new(NULL, &p->position, 3);
	particleref = ParticleRefObject_New((PyObject *)pgroup, p);
	if (vector == NULL || particleref == NULL)
		goto error;
	while (count--) {
		vector->vec = &p->position;
		in_domain = PySequence_Contains(self->domain, (PyObject *)vector);
		if (in_domain == -1)
			goto error;
		if (Particle_IsAlive(*p) && (in_domain == collect_inside)) {
			if (self->callback != NULL && self->callback != Py_None) {
				particleref->p = p;
				result = PyObject_CallFunctionObjArgs(
					self->callback, (PyObject *)particleref, (PyObject *)pgroup, 
					(PyObject *)self, NULL);
				if (result == NULL) {
					goto error;
				}
				Py_DECREF(result);
			}
			Group_kill_p(pgroup, p);
			self->collected_count++;
		}
		p++;
	}
	Py_DECREF(particleref);
	Py_DECREF(vector);
	
	Py_INCREF(Py_None);
	return Py_None;

error:
	Py_XDECREF(particleref);
	Py_XDECREF(vector);
	return NULL;
}

static struct PyMemberDef CollectorController_members[] = {
    {"collect_inside", T_INT, offsetof(CollectorControllerObject, collect_inside), 0,
        "True to collect particles inside the domain, False "
		"to collect particles outside the domain."},
    {"domain", T_OBJECT, offsetof(CollectorControllerObject, domain), 0,
        "Particles inside or outside this domain are killed depending "
		"on the value of collect_inside"},
    {"callback", T_OBJECT, offsetof(CollectorControllerObject, callback), 0,
        "A function called called for each collected particle, or None\n"
		"Must have the signature:\n"
		"    callback(particle, group, collector)"},
    {"collected_count", T_INT, offsetof(CollectorControllerObject, collected_count), 0,
        "Total number of particles collected"},
	{NULL}
};

PyDoc_STRVAR(CollectorController__doc__, 
	"domain -- particles inside or outside this domain are killed.\n"
	"The domain must have a non-zero volume.\n\n"
	"collect_inside -- True to collect particles inside the domain, False\n"
	"to collect particles outside the domain.\n\n"
	"callback -- an optional function called for each collected particle\n"
	"Must have the signature:\n"
	"    callback(particle, group, collector)");

static PyTypeObject CollectorController_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"controller.Collector",		/*tp_name*/
	sizeof(CollectorControllerObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)CollectorController_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	(ternaryfunc)CollectorController_call, /*tp_call*/
	0,                      /*tp_str*/
	0,                      /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	CollectorController__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	0,  /*tp_methods*/
	CollectorController_members,  /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)CollectorController_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject BounceController_Type;

typedef struct {
	PyObject_HEAD
	PyObject *domain;
	float bounce;
	float friction;
	int bounce_limit;
	PyObject *callback;
} BounceControllerObject;

static void
BounceController_dealloc(BounceControllerObject *self) {
	if (self->domain !=NULL)
		Py_CLEAR(self->domain);
	if (self->callback !=NULL)
		Py_CLEAR(self->callback);
	PyObject_Del(self);
}

static int
BounceController_init(BounceControllerObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"domain", "bounce", "friction", "bounce_limit", "callback", NULL};

	self->callback = NULL;
	self->bounce = 1.0f;
	self->friction = 0.0f;
	self->bounce_limit = 5;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|ffiO:__init__", kwlist,
		&self->domain, &self->bounce, &self->friction, &self->bounce_limit, &self->callback))
		return -1;
	Py_INCREF(self->domain);
	if (self->callback != NULL)
		Py_INCREF(self->callback);
	return 0;
}

static PyObject *
BounceController_call(BounceControllerObject *self, PyObject *args)
{
	float td;
	GroupObject *pgroup;
	VectorObject *start_pos = NULL, *end_pos = NULL;
	PyObject *collide_vec = NULL, *normal_vec = NULL;
	ParticleRefObject *particleref = NULL;
	PyObject *result = NULL, *t = NULL, *intersect_str = NULL;
	float tangent_scale, d;
	Vec3 collide_point, normal, penetration, deflect, slide;
	int bounces, started_inside, inside;
	register Particle *p;
	register unsigned long count;

	if (!PyArg_ParseTuple(args, "fO:__init__", &td, &pgroup))
		return NULL;
	
	if (!GroupObject_Check(pgroup))
		return NULL;
	
	intersect_str = PyString_InternFromString("intersect");
	if (intersect_str == NULL)
		goto error;

	p = pgroup->plist->p;
	tangent_scale = 1.0f - self->friction;
	count = GroupObject_ActiveCount(pgroup);
	start_pos = Vector_new(NULL, &p->last_position, 3);
	end_pos = Vector_new(NULL, &p->position, 3);
	if (start_pos == NULL || end_pos == NULL)
		goto error;
	while (count--) {
		if (Particle_IsAlive(*p)) {
			start_pos->vec = &p->last_position;
			end_pos->vec = &p->position;
			started_inside = PySequence_Contains((PyObject *)self->domain, (PyObject *)start_pos);
			if (started_inside == -1)
				goto error;
			bounces = self->bounce_limit;
			while (bounces--) {
				end_pos->vec = &p->position;
				result = PyObject_CallMethodObjArgs(self->domain, intersect_str,
					(PyObject *)start_pos, (PyObject *)end_pos, NULL);
				if (result == NULL)
					goto error;
				t = PySequence_Tuple(result);
				if (t == NULL)
					goto error;
				Py_CLEAR(result);
				if (PySequence_Fast_GET_SIZE(t) && PySequence_Fast_GET_ITEM(t, 0) != Py_None) {
					if (!PyArg_ParseTuple(t, "(fff)(fff);domain.intersect() returned invalid value",
						&collide_point.x, &collide_point.y, &collide_point.z,
						&normal.x, &normal.y, &normal.z))
						goto error;
					Vec3_sub(&penetration, &p->position, &collide_point);
					d = Vec3_dot(&penetration, &normal);
					Vec3_scalar_mul(&deflect, &normal, d);
					Vec3_sub(&slide, &penetration, &deflect);
					Vec3_scalar_muli(&deflect, self->bounce);
					Vec3_scalar_muli(&slide, tangent_scale);
					Vec3_sub(&p->position, &collide_point, &deflect);
					Vec3_addi(&p->position, &slide);
					d = Vec3_dot(&p->velocity, &normal);
					Vec3_scalar_mul(&deflect, &normal, d);
					Vec3_sub(&slide, &p->velocity, &deflect);
					Vec3_scalar_muli(&deflect, self->bounce);
					Vec3_scalar_muli(&slide, tangent_scale);
					Vec3_sub(&p->velocity, &slide, &deflect);
					start_pos->vec = &collide_point;
					if (self->callback != NULL && self->callback != Py_None) {
						particleref = ParticleRefObject_New((PyObject *)pgroup, p);
						collide_vec = Py_BuildValue(
							"(fff)", collide_point.x, collide_point.y, collide_point.z);
						normal_vec = Py_BuildValue("(fff)", normal.x, normal.y, normal.z);
						if (particleref == NULL || collide_vec == NULL || normal_vec == NULL)
							goto error;
						result = PyObject_CallFunctionObjArgs(
							self->callback, (PyObject *)particleref, (PyObject *)pgroup, 
							(PyObject *)self, collide_vec, normal_vec, NULL);
						if (result == NULL) {
							goto error;
						}
						Py_CLEAR(result);
						Py_CLEAR(particleref);
						Py_CLEAR(collide_vec);
						Py_CLEAR(normal_vec);
					}
					inside = PySequence_Contains((PyObject *)self->domain, (PyObject *)end_pos);
					if (inside == -1)
						goto error;
					if ((started_inside == inside) | (self->bounce <= 0)) {
						/* We started inside or outside and ended the same, we're done
						   This is the common case
						   XXX This is not perfect and may return a false
						   positive for hollow domains with sharp angles */
						break;
					}
				} else {
					/* No collision */
					break;
				}
			}
			Py_CLEAR(t);
		}
		p++;
	}
	Py_DECREF(intersect_str);
	Py_DECREF(start_pos);
	Py_DECREF(end_pos);
	
	Py_INCREF(Py_None);
	return Py_None;

error:
	Py_XDECREF(result);
	Py_XDECREF(t);
	Py_XDECREF(intersect_str);
	Py_XDECREF(particleref);
	Py_XDECREF(start_pos);
	Py_XDECREF(end_pos);
	Py_XDECREF(collide_vec);
	Py_XDECREF(normal_vec);
	return NULL;
}

static struct PyMemberDef BounceController_members[] = {
    {"domain", T_OBJECT, offsetof(BounceControllerObject, domain), 0,
        "Particles are deflected when they collide with the domain boundary"},
    {"bounce", T_FLOAT, offsetof(BounceControllerObject, bounce), 0,
        "Multiplied by the normal component of the collision velocity,\n"
		"this determines the amount of deflection from the domain's boundary.\n"
		"If positive, the particle will bounce off of the domain. If zero\n"
		"the particle will \"stick\" to the boundary. If negative, the particle\n"
		"will be refracted across the boundary"},
	{"friction", T_FLOAT, offsetof(BounceControllerObject, friction), 0,
		"Multiplied by the tangental component of the collision velocity,\n"
		"this determines the resistance to sliding tangentially across the\n"
		"domain boundary. 1 - friction is multiplied by the tangental component\n"
		"of the particle's velocity."},
	{"bounce_limit", T_INT, offsetof(BounceControllerObject, bounce_limit), 0,
		"The maximum number of bounces deflections calculated per particle\n"
		"per iteration. Important for fast moving particles contained inside\n"
		"small domains, or domains with corners. Small values may reduce CPU\n"
		"cost, but also may allow particles to escape containment."},
    {"callback", T_OBJECT, offsetof(BounceControllerObject, callback), 0,
        "A function called called when a particle collides with the domain, or None\n"
		"Must have the signature:\n"
		"    callback(particle, group, controller, collision_point, collision_normal)"},
	{NULL}
};

PyDoc_STRVAR(BounceController__doc__, 
	"Bounce(domain, bounce=1.0, friction=0, bounce_limit=5, callback=None)\n\n"
	"domain -- Particles that collide with the surface of this domain are\n"
	"redirected as if they bounced or reflected off the surface. This\n"
	"alters the position and velocity of the particle. The domain must have\n"
	"a non-zero area.\n\n"
	"bounce -- The coefficient of restitution multiplied by the normal\n"
	"component of the collision velocity. This determines the deflection\n"
	"velocity of the particle. If 1.0 (default) the particle is deflected\n"
	"with the same energy it struck the domain with, if zero, the particle\n"
	"will stick to the domain's surface and not bounce off.\n\n"
	"friction -- The resistance presented by the domain surface to sliding\n"
	"particle movement tangental to the domain. 1 - friction is multiplied\n"
	"by the tangental component of the particles velocity. A value of 0\n"
	"means no friction.  if friction is negative, the particle will gain\n"
	"velocity.\n\n"
	"bounce_limit -- The maximum number of bounces deflections calculated per\n"
	"particle per iteration. Higher values can result in more accuracy\n"
	"at potentially higher CPU cost. A value of -1 is effectively infinite.\n\n"
	"callback -- An optional function called when a particle collides\n"
	"with the domain. Must have the signature:\n"
	"	callback(particle, group, controller, collision_point, collision_normal)\n"
	"collision_point is point on the domain where the collision occurred.\n"
	"collision_normal is the normal vector on the domain's surface at the\n"
	"point of collision."
);

static PyTypeObject BounceController_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"controller.Bounce",		/*tp_name*/
	sizeof(BounceControllerObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)BounceController_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	(ternaryfunc)BounceController_call, /*tp_call*/
	0,                      /*tp_str*/
	0,                      /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	BounceController__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	0,  /*tp_methods*/
	BounceController_members,  /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)BounceController_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject MagnetController_Type;

typedef struct {
	PyObject_HEAD
	PyObject *domain;
	float charge;
	float exponent;
	float epsilon;
	float outer_cutoff;
} MagnetControllerObject;

static void
MagnetController_dealloc(MagnetControllerObject *self) {
	if (self->domain !=NULL)
		Py_CLEAR(self->domain);
	PyObject_Del(self);
}

static int
MagnetController_init(MagnetControllerObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"domain", "charge", "exponent", "epsilon", "outer_cutoff", NULL};

	self->epsilon = EPSILON;
	self->outer_cutoff = FLT_MAX;
	self->exponent = 2.0f;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Of|fff:__init__", kwlist,
		&self->domain, &self->charge, &self->exponent, &self->epsilon, &self->outer_cutoff))
		return -1;
	if (!PyObject_HasAttrString(self->domain, "closest_point_to")) {
		PyErr_Format(PyExc_TypeError, "Specified domain does not implement closest_point_to()");
		return -1;
	}
	Py_INCREF(self->domain);
	return 0;
}

static PyObject *
MagnetController_call(MagnetControllerObject *self, PyObject *args)
{
	float k, a_plus_1, d, td, dist2, mag_over_dist, outer_co2;
	GroupObject *pgroup;
	VectorObject *position = NULL;
	PyObject *closest_pt_to = NULL, *res = NULL, *pt = NULL;
	Vec3 vec;
	register Particle *p;
	register unsigned long count;

	if (!PyArg_ParseTuple(args, "fO:__call__", &td, &pgroup))
		return NULL;
	
	if (!GroupObject_Check(pgroup))
		return NULL;

	outer_co2 = self->outer_cutoff*self->outer_cutoff;
	k = self->charge * td;
	a_plus_1 = self->exponent + 1.0f;
	p = pgroup->plist->p;
	count = GroupObject_ActiveCount(pgroup);
	position = Vector_new(NULL, &p->position, 3);
	closest_pt_to = PyObject_GetAttrString(self->domain, "closest_point_to");
	if (position == NULL || closest_pt_to == NULL)
		goto error;
	while (count--) {
		if (Particle_IsAlive(*p)) {
			position->vec = &p->position;
			res = PyObject_CallFunctionObjArgs(closest_pt_to, position, NULL);
			if (res == NULL)
				goto error;
			pt = PySequence_GetItem(res, 0);
			if (pt == NULL || !Vec3_FromSequence(&vec, pt))
				goto error;
			Py_CLEAR(res);
			Py_CLEAR(pt);
			Vec3_subi(&vec, &p->position);
			dist2 = Vec3_len_sq(&vec);
			if (dist2 <= outer_co2) {
				d = sqrtf(dist2) + self->epsilon;
				mag_over_dist = k / powf(d, a_plus_1);
				Vec3_scalar_muli(&vec, mag_over_dist);
				Vec3_addi(&p->velocity, &vec);
			}
		}
		p++;
	}
	Py_DECREF(position);
	Py_DECREF(closest_pt_to);
	
	Py_INCREF(Py_None);
	return Py_None;

error:
	Py_XDECREF(position);
	Py_XDECREF(res);
	Py_XDECREF(pt);
	Py_XDECREF(closest_pt_to);
	return NULL;
}

static struct PyMemberDef MagnetController_members[] = {
    {"domain", T_OBJECT, offsetof(MagnetControllerObject, domain), 0,
        "Particles are attracted or repulsed from the domain's surface."},
    {"charge", T_FLOAT, offsetof(MagnetControllerObject, charge), 0,
        "Determines the magnitude of the magnetic force from the domain.\n"
		"A positive charge is attractive, a negative charge is repulsive."},
    {"exponent", T_FLOAT, offsetof(MagnetControllerObject, exponent), 0,
		"The magnetic force falls off proportional to the particle distance "
		"from the domain raised to this power."},
    {"epsilon", T_FLOAT, offsetof(MagnetControllerObject, epsilon), 0,
		"As the distance between the particle and domain surface decreases "
		"the magnet's force tends toward infinity. To improve stability, "
		"the value of epsilon is always added to the distance value when "
		"calculating the magnet force. The default value for epsilon is "
		"very close to zero, and provides a realistic simulation while "
		"avoiding infinite forces. Larger values may be desireable for certain "
		"visual effects where increased stability is more desirable than "
		"realistic physics. Using a value of zero for epsilon is not recommended "
		"to avoid infinite particle velocities."},
    {"outer_cutoff", T_FLOAT, offsetof(MagnetControllerObject, outer_cutoff), 0,
		"No force is exerted on particles with a distance from the domain "
		"greater than this value. Useful as an optimization to avoid calculating "
		"trivial forces on distant particles."},
	{NULL}
};

PyDoc_STRVAR(MagnetController__doc__, 
	"Magnet(domain, charge, exponent=2, epsilon=0.00001, outer_cutoff=inf)");

static PyTypeObject MagnetController_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"controller.Magnet",		/*tp_name*/
	sizeof(MagnetControllerObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)MagnetController_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	(ternaryfunc)MagnetController_call, /*tp_call*/
	0,                      /*tp_str*/
	0,                      /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	MagnetController__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	0,  /*tp_methods*/
	MagnetController_members,  /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)MagnetController_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

static PyTypeObject DragController_Type;

typedef struct {
	PyObject_HEAD
	float c1;
	float c2;
	Vec3 fluid_velocity;
	PyObject *domain;
} DragControllerObject;

static void
DragController_dealloc(DragControllerObject *self) {
	if (self->domain != NULL)
		Py_CLEAR(self->domain);
	PyObject_Del(self);
}

static int
DragController_init(DragControllerObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"c1", "c2", "fluid_velocity", "domain", NULL};
	PyObject *fvel = NULL;

	self->c2 = 0.0f;
	self->fluid_velocity.x = self->fluid_velocity.y = self->fluid_velocity.z = 0.0f;
	self->domain = NULL;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "f|fOO:__init__", kwlist,
		&self->c1, &self->c2, &fvel, &self->domain))
		return -1;
	if (fvel != NULL) {
		if (!Vec3_FromSequence(&self->fluid_velocity, fvel))
			return -1;
	}
	if (self->domain == Py_None)
		self->domain = NULL; /* Avoid having to test for NULL and None */
	if (self->domain != NULL)
		Py_INCREF(self->domain);
	return 0;
}

static PyObject *
DragController_call(DragControllerObject *self, PyObject *args)
{
	float td, rmag, drag;
	Vec3 fvel, rvel, force;
	VectorObject *position = NULL;
	int in_domain;
	GroupObject *pgroup;
	register Particle *p;
	register unsigned long count;

	if (!PyArg_ParseTuple(args, "fO:__init__", &td, &pgroup))
		return NULL;
	
	if (!GroupObject_Check(pgroup))
		return NULL;

	Vec3_scalar_mul(&fvel, &self->fluid_velocity, td);
	p = pgroup->plist->p;
	position = Vector_new(NULL, &p->position, 3);
	if (position == NULL)
		goto error;

	count = GroupObject_ActiveCount(pgroup);
	while (count--) {
		position->vec = &p->position;
		in_domain = self->domain == NULL || PySequence_Contains(
			self->domain, (PyObject *)position);
		if (in_domain == -1)
			goto error;

		if (Particle_IsAlive(*p) && in_domain) {
			/* Use the last velocity so controller order doesn't matter */
			Vec3_scalar_mul(&rvel, &p->last_velocity, td);
			Vec3_subi(&rvel, &fvel);
			rmag = Vec3_len_sq(&rvel);
			if (rmag > EPSILON) {
				Vec3_scalar_div(&force, &rvel, rmag);
				drag = self->c1*rmag + self->c2*rmag*rmag;
				Vec3_scalar_muli(&force, drag);
				Vec3_scalar_div(&force, &force, p->mass);
				Vec3_subi(&p->velocity, &force);
			}
		}
		p++;
	}
	
	Py_DECREF(position);
	Py_INCREF(Py_None);
	return Py_None;
error:
	Py_XDECREF(position);
	return NULL;
}

static struct PyMemberDef DragController_members[] = {
    {"domain", T_OBJECT, offsetof(DragControllerObject, domain), 0,
        "Only particles contained in this domain are affected by the controller"},
    {"c1", T_FLOAT, offsetof(DragControllerObject, c1), 0,
        "Linear particle drag coefficient\n"},
    {"c2", T_FLOAT, offsetof(DragControllerObject, c2), 0,
        "Squared particle drag coefficient\n"},
	{NULL}
};

static PyGetSetDef DragController_descriptors[] = {
	{"fluid_velocity", (getter)Vector_get, (setter)Vector_set, 
		"Fluid velocity vector", (void *)offsetof(DragControllerObject, fluid_velocity)},
	{NULL}
};

PyDoc_STRVAR(DragController__doc__, 
	"Simulate viscous drag in a fluid\n\n"
	"Drag(c1, c2, fluid_velocity, domain)\n\n"
	"c1 -- Linear drag coefficient\n\n"
	"c2 -- Squared drag coefficient\n\n"
	"fluid_velocity -- Fluid velocity vector\n"
	"used to simulate a moving fluid\n\n"
	"domain -- If specified, only particles\n"
	"inside this domain are affected by the\n"
	"fluid drag.");

static PyTypeObject DragController_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"controller.Drag",		/*tp_name*/
	sizeof(DragControllerObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)DragController_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,	        /*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	(ternaryfunc)DragController_call, /*tp_call*/
	0,                      /*tp_str*/
	0,                      /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	DragController__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	0,                      /*tp_methods*/
	DragController_members,  /*tp_members*/
	DragController_descriptors,/*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)DragController_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

PyMODINIT_FUNC
init_controller(void)
{
	PyObject *m;

	/* Bind external consts here to appease certain compilers */
	GravityController_Type.tp_alloc = PyType_GenericAlloc;
	GravityController_Type.tp_new = PyType_GenericNew;
	GravityController_Type.tp_getattro = PyObject_GenericGetAttr;
	if (PyType_Ready(&GravityController_Type) < 0)
		return;

	MovementController_Type.tp_alloc = PyType_GenericAlloc;
	MovementController_Type.tp_new = PyType_GenericNew;
	MovementController_Type.tp_getattro = PyObject_GenericGetAttr;
	if (PyType_Ready(&MovementController_Type) < 0)
		return;

	FaderController_Type.tp_alloc = PyType_GenericAlloc;
	FaderController_Type.tp_new = PyType_GenericNew;
	FaderController_Type.tp_getattro = PyObject_GenericGetAttr;
	if (PyType_Ready(&FaderController_Type) < 0)
		return;

	LifetimeController_Type.tp_alloc = PyType_GenericAlloc;
	LifetimeController_Type.tp_new = PyType_GenericNew;
	LifetimeController_Type.tp_getattro = PyObject_GenericGetAttr;
	if (PyType_Ready(&LifetimeController_Type) < 0)
		return;

	ColorBlenderController_Type.tp_alloc = PyType_GenericAlloc;
	ColorBlenderController_Type.tp_new = PyType_GenericNew;
	ColorBlenderController_Type.tp_getattro = PyObject_GenericGetAttr;
	if (PyType_Ready(&ColorBlenderController_Type) < 0)
		return;

	GrowthController_Type.tp_alloc = PyType_GenericAlloc;
	GrowthController_Type.tp_new = PyType_GenericNew;
	GrowthController_Type.tp_getattro = PyObject_GenericGetAttr;
	if (PyType_Ready(&GrowthController_Type) < 0)
		return;

	CollectorController_Type.tp_alloc = PyType_GenericAlloc;
	CollectorController_Type.tp_new = PyType_GenericNew;
	CollectorController_Type.tp_getattro = PyObject_GenericGetAttr;
	if (PyType_Ready(&CollectorController_Type) < 0)
		return;

	BounceController_Type.tp_alloc = PyType_GenericAlloc;
	BounceController_Type.tp_new = PyType_GenericNew;
	BounceController_Type.tp_getattro = PyObject_GenericGetAttr;
	if (PyType_Ready(&BounceController_Type) < 0)
		return;

	MagnetController_Type.tp_alloc = PyType_GenericAlloc;
	MagnetController_Type.tp_new = PyType_GenericNew;
	MagnetController_Type.tp_getattro = PyObject_GenericGetAttr;
	if (PyType_Ready(&MagnetController_Type) < 0)
		return;

	DragController_Type.tp_alloc = PyType_GenericAlloc;
	DragController_Type.tp_new = PyType_GenericNew;
	DragController_Type.tp_getattro = PyObject_GenericGetAttr;
	if (PyType_Ready(&DragController_Type) < 0)
		return;

	/* Create the module and add the types */
	m = Py_InitModule3("_controller", NULL, "Particle Controllers");
	if (m == NULL)
		return;

	Py_INCREF(&GravityController_Type);
	PyModule_AddObject(m, "Gravity", (PyObject *)&GravityController_Type);
	Py_INCREF(&MovementController_Type);
	PyModule_AddObject(m, "Fader", (PyObject *)&FaderController_Type);
	Py_INCREF(&FaderController_Type);
	PyModule_AddObject(m, "Movement", (PyObject *)&MovementController_Type);
	Py_INCREF(&LifetimeController_Type);
	PyModule_AddObject(m, "Lifetime", (PyObject *)&LifetimeController_Type);
	Py_INCREF(&ColorBlenderController_Type);
	PyModule_AddObject(m, "ColorBlender", (PyObject *)&ColorBlenderController_Type);
	Py_INCREF(&GrowthController_Type);
	PyModule_AddObject(m, "Growth", (PyObject *)&GrowthController_Type);
	Py_INCREF(&CollectorController_Type);
	PyModule_AddObject(m, "Collector", (PyObject *)&CollectorController_Type);
	Py_INCREF(&BounceController_Type);
	PyModule_AddObject(m, "Bounce", (PyObject *)&BounceController_Type);
	Py_INCREF(&MagnetController_Type);
	PyModule_AddObject(m, "Magnet", (PyObject *)&MagnetController_Type);
	Py_INCREF(&DragController_Type);
	PyModule_AddObject(m, "Drag", (PyObject *)&DragController_Type);
}
