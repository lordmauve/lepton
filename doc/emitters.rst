Emitters
========

.. module:: lepton.emitter

Emitters are a special type of controller that create new particles in a group.
They can be configured to emit particles at a specific rate, or can be used to
emit a burst of particles at will.

As factories for particles, emitters have several features for specifying the
initial particle parameters. A template particle is provided as a basis for the
particles emitted. Specified alone, each particle emitted is an exact clone of
this template. The basic template may be augmented by a deviation template,
which specifies the statisical deviation of the Particle attributes. This
allows you to easily express how much, and in what way each particle differs
from one another.

Particle attribute values may also be expressed as a sequence of discrete
values (e.g., the colors of the rainbow, discrete sizes, etc). Additionally,
particle attribute values may be derived from a spacial Domain, allowing
expressive control over the desired range of composite attributes, such as
position, velocity and color vectors.

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
