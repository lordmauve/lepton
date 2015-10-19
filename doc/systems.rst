Systems and Groups
==================

Systems
-------

A ParticleSystem is the top-level interface to Lepton. A system can contain
:class:`ParticleGroups`, and you can bind global :doc:`controllers
<controllers>` to it to be applied to all groups in the system. The system has
methods for conveniently updating and rendering all of the groups it contains.

A default particle system is created for you when you import lepton, available
as ``lepton.default_system``.

The ParticleSystem object contains an update method which is designed to be
scheduled to regularly invoke the groups' controllers. It also has a draw
method which can be called within the main loop or "on draw" event handler of
the application. This allows the application to remain decoupled from the
individual groups, controllers and renderers which may change dynamically at
run-time.

.. module:: lepton.system

.. autoclass:: ParticleSystem
   :members:


Groups
------

ParticleGroup objects are the lowest level construct. Groups consist of an
arbitrary number of Particles. Although in many ways a group is like a particle
container, particles cannot exist independently outside of a group. All
particles are created and destroyed via their group's methods. A particle
spends its entire existence within a single group.

By default, groups are automatically added to the default particle system when
they are created. Most applications can simply use the default particle system
object, but more complex applications can create their own systems as needed. A
multi-window application might need separate systems for each window, or
separately systems can be used if different graphics are updated in different
timelines.

All particles in a group have the same general behavior and are rendered
together using the same renderer.

.. module:: lepton.group

.. autoclass:: ParticleGroup
   :members:

Accessing individual particles
''''''''''''''''''''''''''''''

Groups may be iterated to access individual particles. The objects returned
through iteration are :class:`ParticleProxy` objects that serve as a convenient
way to manipulate individual particles within the group.::

    for particle in group:
        if particle.position.y < 0:
            group.kill(particle)


.. class:: ParticleProxy

    A reference to a particle within a group. Various attributes of the proxy
    can be read or set to update the underlying particle.

    .. attribute:: position

       The position of the particle, as a 3-item :class:`Vector`.

    .. attribute:: velocity

       The velocity of the particle, as a 3-item :class:`Vector`.

    .. attribute:: size

       The size of the particle, as a 3-item :class:`Vector`.

    .. attribute:: up

       The current rotation the particle, as a 3-item :class:`Vector` of euler
       angles.

       If using the :class:`Billboard` renderer, only the ``z`` component is in
       fact relevant; the other components are discarded.

       The other renderers do not support rotation.

    .. attribute:: rotation

       The rotation vector of the particle, as a 3-item :class:`Vector`
       of euler angular velocities.

       If using the :class:`Billboard` renderer, only the ``z`` component is in
       fact relevant.

    .. attribute:: color

       The color of the particle, as a 4-item :class:`Vector`.

    .. attribute:: mass

       The mass of the particle as a float.

    .. attribute:: age

       The age of the particle as a float.


.. class:: Vector

    A namedtuple-like proxy for access to vector attributes of a lepton object.
    Like a :class:`ParticleProxy`, these refer to underlying memory within a
    group. You cannot create Vector instances.

    Vectors behave as 3- or 4-item tuples. You can also access components of
    the vector with the attributes ``x``, ``y``, ``z``, or ``r``, ``g``, ``b``,
    and ``a``.

    .. method:: clamp(min, max)

        Clamp all values of the vector between ``min`` and ``max``, in place.

        Returns self.


.. note::

    Particles are not independent first-class objects; the particle data is
    actually stored in a contiguous memory array for efficiency.

    ParticleProxy and Vector objects may become invalid at any point. You
    should not keep references to these objects outside of an update loop.
