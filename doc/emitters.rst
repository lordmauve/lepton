Emitters
========


Emitters are a special type of controller that create new particles in a group.
They can be configured to emit particles at a specific rate, or can be used to
emit a burst of particles at will.

As factories for particles, emitters have several features for specifying the
initial particle parameters. A template particle is provided as a basis for the
particles emitted. Specified alone, each particle emitted is an exact clone of
this template. The basic template may be augmented by a deviation template,
which specifies the statisical deviation of the :class:`Particle` attributes. This
allows you to easily express how much, and in what way each particle differs
from one another.

.. class:: lepton.Particle(**kwargs)

    A template for particle creation. Parameters are specified as keyword
    arguments to the constructor:

    .. attribute:: position

       The position of the particle, as a 3-item tuple.

    .. attribute:: velocity

       The velocity of the particle, as a 3-item tuple.

    .. attribute:: size

       The size of the particle, as a 3-item tuple.

    .. attribute:: up

       The current orientation of the particle, as a 3-item tuple of euler
       angles.

       If using the :class:`.Billboard` renderer, only the ``z`` component is in
       fact relevant; the other components are discarded.

       The other renderers do not support rotation.

    .. attribute:: rotation

       The rotation of the particle, as a 3-item tuple of euler angular
       velocities.

    .. attribute:: color

       The color of the particle, as a 4-item tuple.

    .. attribute:: mass

       The mass of the particle as a float.

    .. attribute:: age

       The age of the particle as a float.

Particle attribute values may also be expressed as a sequence of discrete
values (e.g., the colors of the rainbow, discrete sizes, etc). This is done by
passing a list of values as a keyword argument to the emitter, for example::

    emitter = StaticEmitter(
        ...
        position=[(0, 0, 0), (100, 0, 0), (200, 0, 0)]
    )

Particle attribute values may also be generated randomly within a :doc:`Domain
<domains>`::

    jet = StaticEmitter(
        rate=2000,
        position=domain.Disc(
            (0, 0, -50),  # center
            (0, 0, 1),  # normal
            1.5,  # inner radius
            1.5  # outer radius
        ),
    )

This flexibility allows powerful control over the desired range of composite
attributes, such as position, velocity and color vectors.

.. module:: lepton.emitter

Two emitters are currently included with lepton:

* :class:`StaticEmitter` -- Emits particles at a regular rate over time, or can
  emit an arbitrary number at once
* :class:`PerParticleEmitter` -- Emits particles originating from the positions
  of all existing particles in a group. Useful for creating trails of
  particles, like, for example, fireworks.


.. autoclass:: StaticEmitter
    :members:

.. autoclass:: PerParticleEmitter
    :members:
