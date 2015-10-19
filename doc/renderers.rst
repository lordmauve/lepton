Renderers
=========

A renderer object can be bound to a group, defining how the particles of the
group are drawn. Renderers can be invoked via the ParticleSystem, each
ParticleGroup or individually as desired. Like :doc:`controllers`, renderers
have a simple API and are easy to create for yourself.

OpenGL Renderers
----------------

.. module:: lepton.renderer

.. autoclass:: PointRenderer
    :members:

.. autoclass:: BillboardRenderer
    :members:


Texturizers
'''''''''''

.. module:: lepton.texturizer

.. autoclass:: FlipBookTexturizer
    :members:

.. autoclass:: SpriteTexturizer
    :members:


Pygame Renderers
----------------

.. module:: lepton.pygame_renderer


.. autoclass:: FillRenderer
    :members:

.. autoclass:: BlitRenderer
    :members:

