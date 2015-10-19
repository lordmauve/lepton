Controllers
===========

.. automodule:: lepton.controller

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

Because the maximum number of particles in a group is capped, particle systems
should include some mechanism for destroying particles so that the effect can
continue.

Particles may be destroyed after a certain per-particle lifetime, or perhaps
when they move into or out of a :doc:`domain <domains>`.


.. autoclass:: Lifetime
    :members:

.. autoclass:: Collector
    :members:
