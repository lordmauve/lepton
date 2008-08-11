/* Particle groups
 *
 * $id$
 */

#include "vector.h"

#ifndef _GROUP_H_
#define _GROUP_H_

typedef struct {
	/* Note order is important for alignment */
	Vec3	position;
	Color	color;
	Vec3	velocity;
	Vec3	size;
	Vec3	up;
	Vec3	rotation;
	Vec3	last_position;
	Vec3	last_velocity;
	float	age;
	float	mass;
	float	scratch1; /* for alignment and temporary use */
	float	scratch2;
} Particle;

#define Particle_IsAlive(p) ((p).age >= 0)

/* A ParticleList is a dynamic array arranged as follows:
 * |<----- active and killed ----->|<- new ->|            |
 * |<--------- allocated slots -------------------------->|
 *
 * A particle list consists of particles in three states: new, active and
 * killed.
 *
 * New particles are appended to the "new" section and are not accessible
 * until they are incorporated in the next update iteration. After
 * incorporation, they are moved to the "active" section. At the start of each
 * iteration, the "new" section is empty. If more particles are added than
 * there are allocated slots available, the list is reallocated to expand the
 * allocated slots.
 *
 * As particles are killed during the iteration, They are marked killed, but
 * their slots are not immediately reclaimed. When iterating the particles in
 * the list, the iterating code can choose to test each particle's state and
 * ignore killed particles or consider them all ignoring their state. The
 * latter may be less expensive than testing each particle, and modifying
 * killed particles has no effect anyhow since they are not rendered. The
 * high-level particle iterators for use in Python code, test and omit killed
 * particles from iteration.
 *
 * At the start of each update iteration, a collection/incorporation sweep
 * occurs. This sweep moves the new particles into the "active" section
 * reclaiming some killed particle slots. The list is then truncated to the
 * right-most active particle, reclaiming any killed particles at the end of
 * the list. The number of killed particle slots left will depend on the
 * birth/death rate and order.
 */
typedef struct {
	unsigned long	palloc;    /* Total particle slots allocated */
	unsigned long	pactive;   /* Active particle count */
	unsigned long	pkilled;   /* Total particles killed and not collected */
	unsigned long	pnew;      /* New unincorporated particles */
	Particle		p[];
} ParticleList;

/* The particle group object */
typedef struct {
	PyObject_HEAD
	PyObject		*controllers;
	PyObject		*renderer;
	PyObject		*system;
	unsigned long	iteration; /* update iteration count */ 
	ParticleList	*plist;
} GroupObject;

#define GroupObject_ActiveCount(group) \
	((group)->plist->pactive + (group)->plist->pkilled)

/* Particle reference object are used for Particle proxies and iterators.
 *
 * Particle proxy objects are used to access and manipulate individual
 * particles from Python. Since particles are not first-class objects, the
 * proxy is necessary to allow python to hold references to them.  Particle
 * proxies refer to a particle using a reference to the group and the
 * particles index in the group's particle list. Since particle indices can
 * change at the start of each update iteration, a particle proxy is only
 * valid for that iteration. Since particles are typically only accessed
 * briefly while iterating the group's particles, this tradeoff seems
 * acceptable.
 *
 * Particle group iterators are used from Python and are returned by the
 * ParticleGroup.__iter__() method. Since the order of particles may
 * change at each update, the iterator is only valid during the update
 * iteration it is created in, just like particle proxies
 */
typedef struct {
	PyObject_HEAD
	GroupObject		*pgroup;
	unsigned long	iteration; /* update iteration reference is valid for */ 
	Particle		*p; /* pointer to particle in group */
} ParticleRefObject;

/* Vector objects are used to manipulate Vec3/Color structs from Python
 *
 * Right now these are little more than stubs to be compatible with the
 * original Vec3 and Color ctypes class. 
 */
typedef struct {
	PyObject_HEAD
	GroupObject		*pgroup;
	unsigned long	iteration; /* update iteration vector is valid for */ 
	int				length; /* 3 or 4 */
	union {
		Vec3		*vec;
		Color		*color;
	};
} VectorObject;

#define GROUP_MIN_ALLOC 100

/* Return an index for a new particle in the group, allocating space for it if
 * necessary.
 */
long
Group_new_p(GroupObject *group);

/* Kill the particle at the index specified. Does nothing if the index does
 * not point to a valid particle
 */
void inline
Group_kill_p(GroupObject *group, Particle *p);

/* Return true if o is a bon-a-fide GroupObject */
int
GroupObject_Check(GroupObject *o);

/* Get a vector from an attrbute of the template and store it in vec */
int
get_Vec3(Vec3 *vec, PyObject *template, const char *attrname);

/* Get a color from an attrbute of the template and store it in vec */
int
get_Color(Color *color, PyObject *template, const char *attrname);

/* Get a number from an attrbute of the template and store it in f */
int
get_Float(float *f, PyObject *template, const char *attrname);

/* Create a new particle reference object for the given group and index
 * no range checking is done
 */
inline ParticleRefObject *
ParticleRefObject_New(GroupObject *pgroup, Particle *p);

#endif
