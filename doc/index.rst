lepton - High-performance, pluggable particle engine and API for Python
=======================================================================

**Lepton is designed to make complex and beautiful particle effects possible, and
even easy from Python programs.**

Lepton provides the following core features:

* Native-code core for high-performance particle dynamics and rendering
* Pluggable particle controllers for specifying particle behavior
* Two pluggable OpenGL renderers, and two pygame renderers
* Spacial domains, used to control particle emission and behavior
* Easy to use and powerful texture support, including animation
* Modular architecture that lets you easily configure and customize the engine

The code includes several examples of how you can use the engine (using pyglet
and pygame). Note the engine itself does not depend on any other 3rd-party
Python libraries and simply requires the application to setup an OpenGL context
in order to render particles.


Contents:

.. toctree::
    :maxdepth: 2

    systems
    emitters
    controllers
    domains
    renderers


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

