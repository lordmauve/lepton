Spatial Domains
===============

.. module:: lepton.domain

Domains provide a way to describe regions of space in Lepton. These regions can
be used for things like collision detection or to constrain the initial values
for attributes of new particles created via a :class:`ParticleEmitter`.

Built-in domains
----------------

A number of built-in classes define common spatial domains.

.. autoclass:: Point
   :members:

.. autoclass:: AABox
   :members:

.. autoclass:: Cone
   :members:

.. autoclass:: Cylinder
   :members:

.. autoclass:: Disc
   :members:

.. autoclass:: Line
   :members:

.. autoclass:: Plane
   :members:

.. autoclass:: Point
   :members:

.. autoclass:: Sphere
   :members:


Writing your own domains
------------------------

You can write your own domains in Python.

Domains should fulfil the following interface contract:

.. autoclass:: Domain
   :members:
