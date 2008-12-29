Lepton
======

http://code.google.com/p/py-lepton/

Lepton: A high-performance, pluggable particle engine and API for Python

Requirements
------------

Lepton is platform-independent and should run on any operating system
supporting Python and OpenGL. Let us know if you try to build
and run Lepton on an unusual platform.

The following are required to build and install Lepton:

* Python2.5 or 2.6
* OpenGL
* A C compiler (GCC recommended)

To build on Linux (and possibly other unix-like OSes) You will need to have
the Python and libxext headers installed. On systems using the apt packaging
system, this means you need to have the python-dev-all and libxext-dev
packages installed.

Binary releases will also be available in time. If you'd like to help
out by contributing a binary distribution for you platform, you're more
than welcome to!

The examples provided with Lepton require either pyglet
(http://www.pyglet.com), or pygame (http://www.pygame.org),
but neither are required to use the library.

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
usage, new features or contribute to Lepton, join our google group at:

http://groups.google.com/group/py-lepton-users
