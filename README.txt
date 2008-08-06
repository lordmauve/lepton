Lepton
======

http://code.google.com/p/py-lepton/

Lepton: A high-performance, pluggable particle engine and API for Python

Requirements
------------

Lepton is platform-independent and should run on any operating system
supporting Python and OpenGL.

The following are required to build and install Lepton:

* Python2.5
* OpenGL
* A C compiler (GCC recommended)

Binary releases will also be available in time.

Many of the examples provided with Lepton require pyglet
(http://www.pyglet.com), but it is not required to use the library.

Alpha Software
--------------

Lepton is currently under development, and the APIs may change in future
versions. That said, it has many useful features already and the developers
will attempt to minimize breakage where possible or provide migration
documentation for new versions.

If you find a bug, or desire a feature, please use the issue tracker at:

http://code.google.com/p/py-lepton/issues/list

Installation
------------

If you're reading this README from a source distribution, install lepton
with::

    python setup.py install

If you just want to test it out without installing it, you can use::

    python setup.py build_ext --inplace

Then add this directory to your PYTHONPATH.

Support
-------

If you find a bug, use the issue tracker above. If you'd like to discuss
feature enhancements or shower the developer with compliments, you
can email him at casey dot duncan at gmail dot com.
