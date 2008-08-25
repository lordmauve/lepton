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
/* Python interface to particle group
 *
 * $Id $ */

#include <Python.h>
#include <structmember.h>
#include "group.h"

static PyTypeObject ParticleGroup_Type;
static PyTypeObject ParticleIter_Type;
static PyTypeObject ParticleProxy_Type;

static PyObject *InvalidParticleRefError;

#define GroupObject_CHECK(v) ((v)->ob_type == &ParticleGroup_Type)
#define ParticleProxy_CHECK(v) ((v)->ob_type == &ParticleProxy_Type)

static void
ParticleGroup_dealloc(GroupObject *self)
{
	Py_CLEAR(self->controllers);
	Py_CLEAR(self->renderer);
	Py_CLEAR(self->system);
	PyMem_Free(self->plist);
	self->plist = NULL;
	PyObject_Del(self);
}

static PyObject *
ParticleGroup_set_renderer(GroupObject *self, PyObject *new_renderer);

static int
ParticleGroup_init(GroupObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *particle_module, *r;
	PyObject *controllers = NULL, *renderer = NULL, *system = NULL;

	static char *kwlist[] = {"controllers", "renderer", "system", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOO:__init__", kwlist,
		&controllers, &renderer, &system))
		return -1;

	self->iteration = 0;
	self->plist = (ParticleList *)PyMem_Malloc(
		sizeof(ParticleList) + sizeof(Particle) * GROUP_MIN_ALLOC);
	if (self->plist == NULL) {
		PyErr_NoMemory();
		return -1;
	}
	self->plist->palloc = GROUP_MIN_ALLOC;
	self->plist->pactive = 0;
	self->plist->pnew = 0;
	self->plist->pkilled = 0;
	self->controllers = NULL;
	self->renderer = NULL;
	self->system = NULL;

	if (controllers != NULL) {
		controllers = PySequence_Tuple(controllers);
		if (controllers == NULL)
			goto error;
	}
	self->controllers = controllers;

	if (renderer != NULL) {
		r = ParticleGroup_set_renderer(self, renderer);
		Py_XDECREF(r);
		if (r == NULL || PyErr_Occurred())
			goto error;
	}

	if (system == NULL) {
		/* grab the global default particle system */
		particle_module = PyImport_ImportModule("lepton");
		if (particle_module == NULL)
			goto error;
		system = PyObject_GetAttrString(particle_module, "default_system");
		Py_DECREF(particle_module);
		if (system == NULL)
			goto error;
	} else {
		Py_INCREF(system);
	}
	self->system = system;
	if (system != Py_None) {
		r = PyObject_CallMethod(system, "add_group", "O", self);
		Py_XDECREF(r);
		if (r == NULL || PyErr_Occurred())
			goto error;
	}

	return 0;

error:
	Py_XDECREF(self->controllers);
	Py_XDECREF(self->renderer);
	Py_XDECREF(self->system);
	PyMem_Free(self->plist);
	return -1;
}

/* Group methods */

inline ParticleRefObject *
ParticleRefObject_New(GroupObject *pgroup, Particle *p);

/* Create a new particle in the group from a template */
static ParticleRefObject *
ParticleGroup_new(GroupObject *self, PyObject *ptemplate)
{
	long pindex;
	Particle *pnew;
	int success;
	
	pindex = Group_new_p(self);
	if (pindex < 0) {
		PyErr_NoMemory();
		return NULL;
	}

	pnew = self->plist->p + pindex;
	success = (
		get_Vec3(&pnew->position, ptemplate, "position") &&
		get_Vec3(&pnew->velocity, ptemplate, "velocity") &&
		get_Vec3(&pnew->size, ptemplate, "size") &&
		get_Vec3(&pnew->up, ptemplate, "up") &&
		get_Vec3(&pnew->rotation, ptemplate, "rotation") &&
		get_Color(&pnew->color, ptemplate, "color") &&
		get_Float(&pnew->age, ptemplate, "age") &&
		get_Float(&pnew->mass, ptemplate, "mass"));
	if (!success)
		return NULL;

	return ParticleRefObject_New(self, pnew);
}

static inline int
ParticleRefObject_IsValid(ParticleRefObject *pref);

/* Kill a particle in the group */
static PyObject *
ParticleGroup_kill(GroupObject *self, ParticleRefObject *pref)
{
	if (!ParticleProxy_CHECK(pref)) {
		PyErr_SetString(PyExc_TypeError, 
			"Expected particle reference first argument");
		return NULL;
	}
	if (!ParticleRefObject_IsValid(pref)) 
		return NULL;

	Group_kill_p(self, pref->p);
	Py_INCREF(Py_None);
	return Py_None;
}

/* Return the number of active particles */
static Py_ssize_t
ParticleGroup_length(GroupObject *self)
{
	return (Py_ssize_t)self->plist->pactive;
}

/* Return the number of new particles */
static PyObject *
ParticleGroup_new_count(GroupObject *self)
{
	return PyInt_FromLong(self->plist->pnew);
}

/* Return the number of unreclaimed, killed particles */
static PyObject *
ParticleGroup_killed_count(GroupObject *self)
{
	return PyInt_FromLong(self->plist->pkilled);
}

/* Return a new particle group iterator */
static PyObject *
ParticleGroup_iter(PyObject *self)
{
	GroupObject *group = (GroupObject *)self;
	ParticleRefObject *piter;

	piter = PyObject_New(ParticleRefObject, &ParticleIter_Type);
	if (piter == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	piter->pgroup = group;
	Py_INCREF(group);
	piter->p = group->plist->p;
	piter->iteration = group->iteration;
	return (PyObject *)piter;
}

/* Perform an update iteration */
static PyObject *
ParticleGroup_update(GroupObject *self, PyObject *args)
{
	float td;
	unsigned long head, tail, pnew;
	Particle *p;
	PyObject *ctrlr, *ctrlr_seq, *ctrlr_iter[2], *ctrlr_args;
	PyObject *r;
	int i;

	if (!PyArg_ParseTuple(args, "f:update",  &td))
		return NULL;
	
	self->iteration++; /* invalidate proxies and group iterators */

	/* consolidate active and new particles, reclaim some killed in the
	 * process. The goal here is to strike a balance between consolidation
	 * cost and keeping killed particles at bay. New particles are moved into
	 * killed particle slots, and any killed particles at the end of the plist
	 * are reclaimed. Care is taken not to reorder active particles to avoid
	 * popping artifacts for renderers that draw in group-order. The order for
	 * newly incorporated particles is arbitrary. This implementation never
	 * moves active particles, but that is not a guarantee of the API, thus we
	 * still invalidate proxies and particles iters beforehand.
	 */
	p = self->plist->p;
	pnew = self->plist->pnew;
	head = 0;
	tail = GroupObject_ActiveCount(self) + pnew;
	/* Incorporate new particles and update last* and age particle attributes */
	while (head < tail) {
		if (!Particle_IsAlive(p[head])) {
			if (pnew > 0) {
				if (Particle_IsAlive(p[tail - 1])) {
					memcpy(&p[head], &p[--tail], sizeof(Particle));
					self->plist->pactive++;
				} else {
					tail--;
				}
				pnew--;
			} else {
				head++;
			}
		}
		/* This loop visits all active particles */
		while (head < tail && Particle_IsAlive(p[head])) {
			/* Update some universal particle state */
			p[head].age += td;
			p[head].last_position = p[head].position;
			p[head].last_velocity = p[head].velocity;
			head++;
		}
	}
	/* reclaim killed particles at the end */
	while (tail > 0 && !Particle_IsAlive(p[tail - 1]))
		tail--;
    self->plist->pactive += pnew;
	self->plist->pkilled = tail - self->plist->pactive;
	self->plist->pnew = 0;

	/* invoke the controllers */
	ctrlr_seq = PyObject_GetAttrString(self->system, "controllers");
	if (ctrlr_seq == NULL)
		return NULL;
	ctrlr_iter[0] = PyObject_GetIter(ctrlr_seq);
	Py_CLEAR(ctrlr_seq);
	if (ctrlr_iter[0] == NULL)
		return NULL;
	if (self->controllers != NULL) 
		ctrlr_iter[1] = PyObject_GetIter(self->controllers);
	else
		ctrlr_iter[1] = NULL;
	ctrlr_args = Py_BuildValue("fO", td, self);
	if (ctrlr_args == NULL)
		goto error;
	
	for (i = 0; i <= 1; i++) {
		if (ctrlr_iter[i] != NULL) {
			while ((ctrlr = PyIter_Next(ctrlr_iter[i]))) {
				r = PyObject_CallObject(ctrlr, ctrlr_args);
				Py_DECREF(ctrlr);
				Py_XDECREF(r);
				if (r == NULL || PyErr_Occurred())
					goto error;
			}
			Py_CLEAR(ctrlr_iter[i]);
		}
	}
	
	Py_DECREF(ctrlr_args);
	Py_INCREF(Py_None);
	return Py_None;
error:
	Py_XDECREF(ctrlr_iter[0]);
	Py_XDECREF(ctrlr_iter[1]);
	Py_XDECREF(ctrlr_args);
	return NULL;
}

/* Bind one or more controllers to the group */
static PyObject *
ParticleGroup_bind_controller(GroupObject *self, PyObject *args)
{
	PyObject *new_list;

	if (self->controllers != NULL) {
		new_list = PySequence_Concat(self->controllers, args);
		Py_DECREF(self->controllers);
		self->controllers = new_list;
	} else {
		Py_INCREF(args);
		self->controllers = args;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

/* Set the group's renderer */
static PyObject *
ParticleGroup_unbind_controller(GroupObject *self, PyObject *ctrlr)
{
	PyObject *new_ctrlrs, *item;
	int i, n, ctrlr_count;

	if (self->controllers == NULL || !PySequence_Contains(self->controllers, ctrlr)) {
		PyErr_SetString(PyExc_ValueError, "renderer not bound");
		return NULL;
	}
	ctrlr_count = PyTuple_Size(self->controllers);
	new_ctrlrs = PyTuple_New(ctrlr_count - 1);
	n = 0;
	for (i = 0; i < ctrlr_count; i++) {
		item = PyTuple_GetItem(self->controllers, i);
		if (item == NULL)
			return NULL;
		if (item != ctrlr) {
			Py_INCREF(item);
			PyTuple_SET_ITEM(new_ctrlrs, n++, item);
		}
	}
	Py_DECREF(self->controllers);
	self->controllers = new_ctrlrs;
	Py_INCREF(Py_None);
	return Py_None;
}

/* Set the group's renderer */
static PyObject *
ParticleGroup_set_renderer(GroupObject *self, PyObject *new_renderer)
{
	PyObject *r;

	if (new_renderer != Py_None) {
		/* call renderer.set_group(self) */
		r = PyObject_CallMethod(new_renderer, "set_group", "O", self);
		Py_XDECREF(r);
		if (r == NULL || PyErr_Occurred())
			return NULL;
	}

	if (self->renderer != NULL && self->renderer != Py_None) {
		/* call self.renderer.set_group(None) */
		r = PyObject_CallMethod(self->renderer, "set_group", "O", Py_None);
		Py_XDECREF(r);
		if (r == NULL || PyErr_Occurred())
			return NULL;
	}

	Py_INCREF(new_renderer);
	Py_XDECREF(self->renderer);
	self->renderer = new_renderer;

	Py_INCREF(Py_None);
	return Py_None;
}

/* Draw the group using its renderer (if any) */
static PyObject *
ParticleGroup_draw(GroupObject *self)
{
	PyObject *r;

	if (self->renderer != NULL && self->renderer != Py_None) {
		r = PyObject_CallMethod(self->renderer, "draw", NULL);
		Py_XDECREF(r);
		if (r == NULL || PyErr_Occurred())
			return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMemberDef ParticleGroup_members[] = {
    {"controllers", T_OBJECT, offsetof(GroupObject, controllers), RO,
        "Controllers bound to this group"},
    {"renderer", T_OBJECT, offsetof(GroupObject, renderer), RO,
        "Renderer bound to this group"},
    {"system", T_OBJECT, offsetof(GroupObject, system), RO,
        "Particle system this group belongs to"},
	{NULL}
};

static PyMethodDef ParticleGroup_methods[] = {
	{"new", (PyCFunction)ParticleGroup_new, METH_O,
		PyDoc_STR("new(template) -> particle reference\n"
			"Create a new particle in the group with attributes\n"
			"copied from the specified template particle.\n"
			"Note new particles are not visible until\n"
			"they are incorporated by calling the update()\n"
			"method.")},
	{"kill", (PyCFunction)ParticleGroup_kill, METH_O,
		PyDoc_STR("kill(particle) -> None\n"
			"Destroy a particle in the group.")},
	{"new_count", (PyCFunction)ParticleGroup_new_count, METH_NOARGS,
		PyDoc_STR("new_count() -> Number of new particles not yet incorporated")},
	{"killed_count", (PyCFunction)ParticleGroup_killed_count, METH_NOARGS,
		PyDoc_STR("killed_count() -> Number of killed particles not yet reclaimed")},
	{"update", (PyCFunction)ParticleGroup_update, METH_VARARGS,
		PyDoc_STR("update(time_delta) -> None\n"
			"Incorporate new particles added since the last update,\n"
			"and optimize the particle list. Then invoke the controllers\n"
			"bound to the group to update the particles")},
	{"draw", (PyCFunction)ParticleGroup_draw, METH_NOARGS,
		PyDoc_STR("Draw the group using its renderer (if any)")},
	{"set_renderer", (PyCFunction)ParticleGroup_set_renderer, METH_O,
		PyDoc_STR("Set the renderer for the group. This replaces any existing\n"
			"renderer. Pass None to disable rendering")},
	{"bind_controller", (PyCFunction)ParticleGroup_bind_controller, METH_VARARGS,
		PyDoc_STR("Bind one or more controllers to the group")},
	{"unbind_controller", (PyCFunction)ParticleGroup_unbind_controller, METH_O,
		PyDoc_STR("Unbind a controller from the group so it is no longer\n"
			"invoked on update. If the controller is not bound to the group\n"
			"raise ValueError")},
	{NULL,		NULL}		/* sentinel */
};

static PySequenceMethods ParticleGroup_as_sequence = {
	(lenfunc)ParticleGroup_length	/* sq_length */
};
	
PyDoc_STRVAR(ParticleGroup__doc__, 
	"Group of particles that share behavior via controllers\n"
	"and are rendered as a unit\n\n"
	"ParticleGroup(controllers=(), renderer=None, system=particle.default_system)\n\n"
	"Initialize the particle group, binding the supplied\n"
	"controllers to it and setting the renderer.\n\n"
	"If a system is specified, the group is added to that particle system\n"
	"automatically. By default, the group is added to the default particle\n"
	"system (particle.default_system). If you do not wish to bind the group to a\n"
	"system immediately, pass None for the system.");

static PyTypeObject ParticleGroup_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"group.ParticleGroup",		/*tp_name*/
	sizeof(GroupObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)ParticleGroup_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	&ParticleGroup_as_sequence,	/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	0,                      /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	ParticleGroup__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	ParticleGroup_iter, /*tp_iter*/
	0,                      /*tp_iternext*/
	ParticleGroup_methods,  /*tp_methods*/
	ParticleGroup_members,  /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	(initproc)ParticleGroup_init, /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

static PyTypeObject Vector_Type;

static void
Vector_dealloc(VectorObject *self)
{
	if (self->pgroup != NULL)
		Py_CLEAR(self->pgroup);
	PyObject_Del(self);
}

/* Create a new vector object for the group and vector struct specified */
VectorObject *
Vector_new(GroupObject *pgroup, Vec3 *vec, int length)
{
	VectorObject *newvec;

	if (length !=3 && length != 4) {
		PyErr_SetString(PyExc_ValueError, "expected length 3 or 4");
		return NULL;
	}

	newvec = PyObject_New(VectorObject, &Vector_Type);
	if (newvec == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	newvec->pgroup = pgroup;
	if (pgroup != NULL) {
		Py_INCREF(pgroup);
		newvec->iteration = pgroup->iteration;
	} else {
		newvec->iteration = 0;
	}
	newvec->length = length;
	newvec->vec = vec;
	return newvec;
}

static int
Vector_setattr(VectorObject *self, char *name, PyObject *v)
{
	int result;
	if (self->pgroup != NULL && self->iteration != self->pgroup->iteration) {
		PyErr_SetString(InvalidParticleRefError, "Invalid particle reference");
		return -1;
	}
	
	if (strlen(name) != 1) {
		PyErr_SetString(PyExc_AttributeError, name);
		return -1;
	}

	v = PyNumber_Float(v);
	if (v == NULL)
		return -1;
	
	result = 0;
	switch (name[0]) {
		case 'r': case 'x': 
			self->vec->x = PyFloat_AS_DOUBLE(v);
			break;
		case 'g': case 'y':
			self->vec->y = PyFloat_AS_DOUBLE(v);
			break;
		case 'b': case 'z':
			self->vec->z = PyFloat_AS_DOUBLE(v);
			break;
		case 'a':
			self->color->a = PyFloat_AS_DOUBLE(v);
			break;
		default:
			PyErr_SetString(PyExc_AttributeError, name);
			result = -1;
	}
	
	Py_DECREF(v);
	return result;
}

static PyObject *
Vector_repr(VectorObject *self)
{
	const int bufsize = 255;
	char buf[bufsize];
	if (self->pgroup == NULL || self->iteration == self->pgroup->iteration) {
		buf[0] = 0; /* paranoid */
		if (self->length == 3) 
			snprintf(buf, bufsize, "Vector(%.1f, %.1f, %.1f)",
				self->vec->x, self->vec->y, self->vec->z);
		else
			snprintf(buf, bufsize, "Color(%.1f, %.1f, %.1f, %.1f)",
				self->color->r, self->color->g, self->color->b, self->color->a);
		return PyString_FromString(buf);
	} else {
		return PyString_FromFormat(
			"<INVALID vector of group %p>", self->pgroup);
	}
}

static Py_ssize_t
Vector_length(VectorObject *self)
{
	return (Py_ssize_t)self->length;
}

static PyObject *
Vector_getitem(VectorObject *self, Py_ssize_t index)
{
	if (self->pgroup != NULL && self->iteration != self->pgroup->iteration) {
		PyErr_SetString(InvalidParticleRefError, "Invalid particle reference");
		return NULL;
	}

	switch (index) {
		case 0: return PyFloat_FromDouble(self->vec->x);
		case 1: return PyFloat_FromDouble(self->vec->y);
		case 2: return PyFloat_FromDouble(self->vec->z);
		case 3: if (self->length == 4) return PyFloat_FromDouble(self->color->a);
	}
	PyErr_Format(PyExc_IndexError, "%d", (int)index);
	return NULL;
}

static int
Vector_assitem(VectorObject *self, Py_ssize_t index, PyObject *v)
{
	switch (index) {
		case 0: return Vector_setattr(self, "x", v);
		case 1: return Vector_setattr(self, "y", v);
		case 2: return Vector_setattr(self, "z", v);
		case 3: if (self->length == 4) return Vector_setattr(self, "a", v);;
	}
	PyErr_Format(PyExc_IndexError, "%d", (int)index);
	return -1;
}

static PyObject *
Vector_clamp(VectorObject *self, PyObject *args)
{
	float min, max;

	if (!PyArg_ParseTuple(args, "ff:clamp",  &min, &max))
		return NULL;

	if (min > max) {
		PyErr_Format(PyExc_ValueError, "clamp: Expected min <= max");
		return NULL;
	}

	if (self->length == 3) {
		self->vec->x = (self->vec->x < min) ? min : self->vec->x;
		self->vec->x = (self->vec->x > max) ? max : self->vec->x;
		self->vec->y = (self->vec->y < min) ? min : self->vec->y;
		self->vec->y = (self->vec->y > max) ? max : self->vec->y;
		self->vec->z = (self->vec->z < min) ? min : self->vec->z;
		self->vec->z = (self->vec->z > max) ? max : self->vec->z;
	} else {
		self->color->r = (self->color->r < min) ? min : self->color->r;
		self->color->r = (self->color->r > max) ? max : self->color->r;
		self->color->g = (self->color->g < min) ? min : self->color->g;
		self->color->g = (self->color->g > max) ? max : self->color->g;
		self->color->b = (self->color->b < min) ? min : self->color->b;
		self->color->b = (self->color->b > max) ? max : self->color->b;
		self->color->a = (self->color->a < min) ? min : self->color->a;
		self->color->a = (self->color->a > max) ? max : self->color->a;
	}

	Py_INCREF(self);
	return (PyObject *)self;
}

static PySequenceMethods Vector_as_sequence = {
	(lenfunc)Vector_length,	/* sq_length */
	0,		/*sq_concat*/
	0,		/*sq_repeat*/
	(ssizeargfunc)Vector_getitem,		/*sq_item*/
	0,		/* sq_slice */
	(ssizeobjargproc)Vector_assitem,	/* sq_ass_item */
};

static PyMethodDef Vector_methods[] = {
	{"clamp", (PyCFunction)Vector_clamp, METH_VARARGS,
		PyDoc_STR("clamp(min, max) -> vector\n"
			"Clamp all values of the vector between min and max\n"
			"return the resulting vector")},
	{NULL, NULL}
};

static PyObject *
Vector_getattr(VectorObject *self, PyObject *o)
{
	char *name = PyString_AS_STRING(o);

	if (self->pgroup != NULL && self->iteration != self->pgroup->iteration) {
		PyErr_SetString(InvalidParticleRefError, "Invalid particle reference");
		return NULL;
	}
	
	if (strlen(name) == 1) {
		switch (name[0]) {
			case 'r': case 'x': return PyFloat_FromDouble(self->vec->x);
			case 'g': case 'y': return PyFloat_FromDouble(self->vec->y);
			case 'b': case 'z': return PyFloat_FromDouble(self->vec->z);
			case 'a': return PyFloat_FromDouble(self->color->a);
		}
	}

	return Py_FindMethod(Vector_methods, (PyObject *)self, name);
}

PyDoc_STRVAR(Vector__doc__, "Vector swizzler");

static PyTypeObject Vector_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			                /*ob_size*/
	"group.Vector",		/*tp_name*/
	sizeof(VectorObject),	/*tp_basicsize*/
	0,			                /*tp_itemsize*/
	/* methods */
	(destructor)Vector_dealloc, /*tp_dealloc*/
	0,			            /*tp_print*/
	0,                      /*tp_getattr*/
	(setattrfunc)Vector_setattr, /*tp_setattr*/
	0,			            /*tp_compare*/
	(reprfunc)Vector_repr,  /*tp_repr*/
	0,			            /*tp_as_number*/
	&Vector_as_sequence,    /*tp_as_sequence*/
	0,			            /*tp_as_mapping*/
	0,			            /*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	(getattrofunc)Vector_getattr, /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	Vector__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	0,                      /*tp_iternext*/
	Vector_methods,         /*tp_methods*/
	0,                      /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	0, /*(initproc)Vector_init,*/ /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */


static void
ParticleProxy_dealloc(ParticleRefObject *self)
{
	if (self->pgroup != NULL)
		Py_CLEAR(self->pgroup);
	PyObject_Del(self);
}

/* Create a new particle reference object for the given group and index
 * no range checking is done
 */
inline ParticleRefObject *
ParticleRefObject_New(GroupObject *pgroup, Particle *p)
{
	ParticleRefObject *pproxy;
	pproxy = PyObject_New(ParticleRefObject, &ParticleProxy_Type);
	if (pproxy == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	pproxy->pgroup = pgroup;
	if (pgroup != NULL) {
		Py_INCREF(pgroup);
		pproxy->iteration = pgroup->iteration;
	} else {
		pproxy->iteration = 0;
	}
	pproxy->p = p;
	return pproxy;
}

static inline int
ParticleRefObject_IsValid(ParticleRefObject *pref) {
	if (pref->pgroup == NULL || pref->iteration == pref->pgroup->iteration) {
		return 1;
	} else {
		PyErr_SetString(InvalidParticleRefError, "Invalid particle reference");
		return 0;
	}
}

static const char *ParticleProxy_attrname[] = {
	"position", "velocity", "size", "up", "rotation", 
	"last_position", "last_velocity", "color", "mass", "age", 
	NULL
};

static PyObject *
ParticleProxy_getattr(ParticleRefObject *self, char *name)
{
	int attr_no;

	if (!ParticleRefObject_IsValid(self))
		return NULL;
	
	for (attr_no = 0; ParticleProxy_attrname[attr_no]; attr_no++) {
		if (!strcmp(name, ParticleProxy_attrname[attr_no]) )
			break;
	}
	if (!ParticleProxy_attrname[attr_no]) {
		PyErr_SetString(PyExc_AttributeError, name);
		return NULL;
	}
	switch (attr_no) {
		case 0: return (PyObject *)Vector_new(self->pgroup, &self->p->position, 3);
		case 1: return (PyObject *)Vector_new(self->pgroup, &self->p->velocity, 3);
		case 2: return (PyObject *)Vector_new(self->pgroup, &self->p->size, 3);
		case 3: return (PyObject *)Vector_new(self->pgroup, &self->p->up, 3);
		case 4: return (PyObject *)Vector_new(self->pgroup, &self->p->rotation, 3);
		case 5: return (PyObject *)Vector_new(self->pgroup, &self->p->last_position, 3);
		case 6: return (PyObject *)Vector_new(self->pgroup, &self->p->last_velocity, 3);
		case 7: return (PyObject *)Vector_new(self->pgroup, (Vec3 *)&self->p->color, 4);
		case 8: return Py_BuildValue("f", self->p->mass);
		case 9: return Py_BuildValue("f", self->p->age);
	};
	return NULL; /* shouldn't get here */
}

static int
ParticleProxy_setattr(ParticleRefObject *self, char *name, PyObject *v)
{
	int attr_no, result = 0;

	if (!ParticleRefObject_IsValid(self))
		return -1;
	
	for (attr_no = 0; ParticleProxy_attrname[attr_no]; attr_no++) {
		if (!strcmp(name, ParticleProxy_attrname[attr_no]) )
			break;
	}
	if (!ParticleProxy_attrname[attr_no] || v == NULL) {
		PyErr_SetString(PyExc_AttributeError, name);
		return -1;
	}
	if (attr_no <= 7) {
		v = PySequence_Tuple(v);
	} else {
		v = PyNumber_Float(v);
	}
	if (v == NULL)
		return -1;
	
	switch (attr_no) {
		case 0: 
			result = PyArg_ParseTuple(v, "fff;3 floats expected", 
				&self->p->position.x, 
				&self->p->position.y, 
				&self->p->position.z) - 1;
			break;
		case 1: 
			result = PyArg_ParseTuple(v, "fff;3 floats expected", 
				&self->p->velocity.x, 
				&self->p->velocity.y, 
				&self->p->velocity.z) - 1;
			break;
		case 2: 
			result = PyArg_ParseTuple(v, "fff;3 floats expected", 
				&self->p->size.x, 
				&self->p->size.y, 
				&self->p->size.z) - 1;
			break;
		case 3: 
			result = PyArg_ParseTuple(v, "fff;3 floats expected", 
				&self->p->up.x, 
				&self->p->up.y, 
				&self->p->up.z) - 1;
			break;
		case 4: 
			result = PyArg_ParseTuple(v, "fff;3 floats expected", 
				&self->p->rotation.x, 
				&self->p->rotation.y, 
				&self->p->rotation.z) - 1;
			break;
		case 5: 
			result = PyArg_ParseTuple(v, "fff;3 floats expected", 
				&self->p->last_position.x, 
				&self->p->last_position.y, 
				&self->p->last_position.z) - 1;
			break;
		case 6: 
			result = PyArg_ParseTuple(v, "fff;3 floats expected", 
				&self->p->last_velocity.x, &self->p->last_velocity.y, 
				&self->p->last_velocity.z) - 1;
			break;
		case 7: 
			self->p->color.a = 1.0f;
			result = PyArg_ParseTuple(v, "fff|f;3 or 4 floats expected", 
				&self->p->color.r, 
				&self->p->color.g, 
				&self->p->color.b, 
				&self->p->color.a) - 1;
			break;
		case 8: self->p->mass = PyFloat_AS_DOUBLE(v);
			break;
		case 9: self->p->age = PyFloat_AS_DOUBLE(v);
	};

	Py_XDECREF(v);
	return result;
}

static PyObject *
ParticleProxy_repr(ParticleRefObject *self)
{
	const int bufsize = 1024;
	char buf[bufsize];

	if (ParticleRefObject_IsValid(self)) {
		buf[0] = 0; /* paranoid */
		snprintf(buf, bufsize, "<Particle %lu of group 0x%lx: "
			"position=(%.1f, %.1f, %.1f) velocity=(%.1f, %.1f, %.1f) "
			"color=(%.1f, %.1f, %.1f, %.1f) size=(%.1f, %.1f, %.1f) "
			"up=(%.1f, %.1f, %.1f) rotation=(%.1f, %.1f, %.1f) "
			"last_position=(%.1f, %.1f, %.1f) last_velocity=(%.1f, %.1f, %.1f) "
			"mass=%.1f age=%.1f>",
			(unsigned long)(self->p), (unsigned long)self->pgroup,
			self->p->position.x, self->p->position.y, self->p->position.z,
			self->p->velocity.x, self->p->velocity.y, self->p->velocity.z,
			self->p->color.r, self->p->color.g, self->p->color.b, self->p->color.a,
			self->p->size.x, self->p->size.y, self->p->size.z,
			self->p->up.x, self->p->up.y, self->p->up.z,
			self->p->rotation.x, self->p->rotation.y, self->p->rotation.z,
			self->p->last_position.x, self->p->last_position.y, self->p->last_position.z,
			self->p->last_velocity.x, self->p->last_velocity.y, self->p->last_velocity.z,
			self->p->mass, self->p->age);
		return PyString_FromString(buf);
	} else {
		return PyString_FromFormat("<INVALID Particle %lu of group %p>",
			(unsigned long)(self->p), self->pgroup);
	}
}


PyDoc_STRVAR(ParticleProxy__doc__, "Group particle accessor");

static PyTypeObject ParticleProxy_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			                /*ob_size*/
	"group.ParticleProxy",		/*tp_name*/
	sizeof(ParticleRefObject),	/*tp_basicsize*/
	0,			                /*tp_itemsize*/
	/* methods */
	(destructor)ParticleProxy_dealloc, /*tp_dealloc*/
	0,			            /*tp_print*/
	(getattrfunc)ParticleProxy_getattr, /*tp_getattr*/
	(setattrfunc)ParticleProxy_setattr, /*tp_setattr*/
	0,			            /*tp_compare*/
	(reprfunc)ParticleProxy_repr, /*tp_repr*/
	0,			            /*tp_as_number*/
	0,			            /*tp_as_sequence*/
	0,			            /*tp_as_mapping*/
	0,			            /*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	0,                      /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	ParticleProxy__doc__,   /*tp_doc*/
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
	0, /*(initproc)ParticleProxy_init,*/ /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */


static void
ParticleIter_dealloc(ParticleRefObject *self)
{
	Py_CLEAR(self->pgroup);
	PyObject_Del(self);
}

static PyObject *
ParticleIter_next(ParticleRefObject *self)
{
	Particle *lastp;

	if (!ParticleRefObject_IsValid(self))
		return NULL;

	lastp = &self->pgroup->plist->p[GroupObject_ActiveCount(self->pgroup)];

	/* Scan to the next active particle */
	while (self->p < lastp && !Particle_IsAlive(*self->p)) {
		self->p++;
	}
	
	if (self->p < lastp) {
		return (PyObject *)ParticleRefObject_New(self->pgroup, self->p++);
	} else {
		/* End of iteration */
		return NULL;
	}
}

PyDoc_STRVAR(ParticleIter__doc__, "Group particle iterator");

static PyTypeObject ParticleIter_Type = {
	/* The ob_type field must be initialized in the module init function
	 * to be portable to Windows without using C++. */
	PyObject_HEAD_INIT(NULL)
	0,			                /*ob_size*/
	"group.ParticleIter",		/*tp_name*/
	sizeof(ParticleRefObject),	/*tp_basicsize*/
	0,			                /*tp_itemsize*/
	/* methods */
	(destructor)ParticleIter_dealloc, /*tp_dealloc*/
	0,			            /*tp_print*/
	0,                      /*tp_getattr*/
	0,                      /*tp_setattr*/
	0,			            /*tp_compare*/
	0,                      /*tp_repr*/
	0,			            /*tp_as_number*/
	0,			            /*tp_as_sequence*/
	0,			            /*tp_as_mapping*/
	0,			            /*tp_hash*/
	0,                      /*tp_call*/
	0,                      /*tp_str*/
	0,                      /*tp_getattro*/
	0,                      /*tp_setattro*/
	0,                      /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,     /*tp_flags*/
	ParticleIter__doc__,   /*tp_doc*/
	0,                      /*tp_traverse*/
	0,                      /*tp_clear*/
	0,                      /*tp_richcompare*/
	0,                      /*tp_weaklistoffset*/
	0,                      /*tp_iter*/
	(iternextfunc)ParticleIter_next,  /*tp_iternext*/
	0,                      /*tp_methods*/
	0,                      /*tp_members*/
	0,                      /*tp_getset*/
	0,                      /*tp_base*/
	0,                      /*tp_dict*/
	0,                      /*tp_descr_get*/
	0,                      /*tp_descr_set*/
	0,                      /*tp_dictoffset*/
	0,                      /*tp_init*/
	0,                      /*tp_alloc*/
	0,                      /*tp_new*/
	0,                      /*tp_free*/
	0,                      /*tp_is_gc*/
};

/* --------------------------------------------------------------------- */

PyMODINIT_FUNC
initgroup(void)
{
	PyObject *m;

	/* Bind external consts here to appease certain compilers */
	ParticleGroup_Type.tp_alloc = PyType_GenericAlloc;
	ParticleGroup_Type.tp_new = PyType_GenericNew;
	ParticleGroup_Type.tp_getattro = PyObject_GenericGetAttr;
	if (PyType_Ready(&ParticleGroup_Type) < 0)
		return;

	ParticleProxy_Type.tp_alloc = PyType_GenericAlloc;
	if (PyType_Ready(&ParticleProxy_Type) < 0)
		return;

	ParticleIter_Type.tp_alloc = PyType_GenericAlloc;
	ParticleIter_Type.tp_getattro = PyObject_GenericGetAttr;
	ParticleIter_Type.tp_iter = PyObject_SelfIter;
	if (PyType_Ready(&ParticleIter_Type) < 0)
		return;

	Vector_Type.tp_alloc = PyType_GenericAlloc;
	if (PyType_Ready(&Vector_Type) < 0)
		return;

	/* Create the module and add the types */
	m = Py_InitModule3("group", NULL, "Particle Groups");
	if (m == NULL)
		return;

	if (InvalidParticleRefError == NULL) {
		InvalidParticleRefError = PyErr_NewException(
			"group.InvalidParticleRefError", NULL, NULL);
		if (InvalidParticleRefError == NULL)
			return;
	}
	Py_INCREF(InvalidParticleRefError);
	PyModule_AddObject(m, "InvalidParticleRefError", InvalidParticleRefError);

	Py_INCREF(&ParticleGroup_Type);
	PyModule_AddObject(m, "ParticleGroup", (PyObject *)&ParticleGroup_Type);
	Py_INCREF(&ParticleProxy_Type);
	PyModule_AddObject(m, "ParticleProxy", (PyObject *)&ParticleProxy_Type);
	Py_INCREF(&Vector_Type);
	PyModule_AddObject(m, "Vector", (PyObject *)&Vector_Type);
}
