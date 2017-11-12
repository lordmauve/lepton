Lepton
======

Lepton: A high-performance, pluggable particle engine and API for Python

![Bubbles Example](https://raw.githubusercontent.com/lordmauve/lepton/master/doc/_static/bubbles.png)


Documentation
-------------

See http://pythonhosted.org/lepton/ for full documentation.

The examples provided with Lepton require either pyglet
(http://www.pyglet.org), or pygame (http://www.pygame.org),
but neither are required to use the library.


Installation
------------

Lepton is a CPython C API extension, which means it can be difficult to install.
On some systems, lepton can be installed just by running

```
$ pip install lepton
```

A current project is to provide **binary wheel packages** which will ensure
this works (and quickly) on Linux, Mac and Windows (x86 architectures). This
work is currently incomplete, but the CI pages are here and may contain wheels
which work for you:

* [Travis CI](https://travis-ci.org/lordmauve/lepton) - Linux and Mac builds
* [Appveyor](https://ci.appveyor.com/project/lordmauve/lepton) - Windows builds

If you'd like to contribute to making these work, please submit a pull request!


Building
--------

Lepton is platform-independent and should run on any operating system
supporting Python and OpenGL. Let us know if you try to build
and run Lepton on an unusual platform.

The following are required to build and install Lepton:

* Python 2.6+, 3.3+
* OpenGL
* A C compiler (GCC or Visual C++)

To build on Linux (and possibly other unix-like OSes) You will need to have
the Python and libxext headers installed. On systems using the apt packaging
system, this means you need to have the python-dev-all and libxext-dev
packages installed. Specifically on Debian and Ubuntu, you will likely
need the following packages installed:

xorg-dev
libgl1-mesa-dev
libglu1-mesa-dev

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


Credits
-------

Casey Duncan -- Lepton creator and primary developer
Jussi Lepistö -- Windows maintainer
Andrew Charles -- Code contributor
Harry Tormey -- Inspiration and code contributor
Daniel Pope -- Maintainer

And thanks to others for their contributions of ideas and bugfixes!
