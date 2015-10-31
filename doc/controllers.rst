Controllers
===========

.. module:: lepton.controller

A controller is a callable of the form::

    controller(timedelta, group)

Particle controllers are bound to particle groups and invoked via the group's
``update()`` method. A typical controller will iterate all particles in a
group, apply some logic and mutate the particles or remove or add particles to
the group.

Lepton's built-in controllers are written in C for performance, but Python
functions or classes with ``__call__()`` methods can also be used.


Movement
--------

Many of the controllers affect the position and velocity of particles.

.. autoclass:: Gravity
    :members:

.. autoclass:: Movement
    :members:

.. autoclass:: Bounce
    :members:

.. autoclass:: Magnet
    :members:

.. autoclass:: Drag
    :members:


Color
-----

These controllers affect the color or alpha (opacity) of particles.

.. autoclass:: Fader
    :members:

.. autoclass:: ColorBlender
    :members:


Size
----

Particles may grow or shrink over time.

.. autoclass:: Growth
    :members:


Death
-----

If you continue to emit particles into a group then the number of particles
can grow in an unbounded way. To avoid this you should ensure there is some
reliable way in which particles can be destroyed.

Particles may be destroyed after a certain per-particle lifetime, or perhaps
when they move into or out of a :doc:`domain <domains>`.


.. autoclass:: Lifetime
    :members:

.. autoclass:: Collector
    :members:
