#!/usr/bin/env python

# $Id$

import sys
from distutils.core import setup, Extension

extra_link_args = []

if sys.platform.startswith('linux'):
	include_dirs = ['/usr/include', '/usr/local/include/', 
		'/usr/X11/include', '/usr/X11R6/include']
	library_dirs = ['/usr/lib', '/usr/local/lib', 
		'/usr/X11/lib', '/usr/X11R6/lib']
	libraries = ['GL', 'X11', 'Xext']
elif  sys.platform == 'cygwin':
	include_dirs = ['/usr/include',  '/usr/include/win32api/']
	library_dirs = ['/usr/lib']
	libraries = ['opengl32']
elif sys.platform == 'win32':
	include_dirs = ['../include']
	library_dirs = ['../lib']
	libraries = ['opengl32']
elif sys.platform == 'darwin':
	include_dirs = ['/System/Library/Frameworks/OpenGL.framework/Headers', 
		'/System/Library/Frameworks/GLUT.framework/Headers']
	library_dirs = []
	libraries = []
	extra_link_args = ['-framework:OpenGL']
else:
	print >>sys.stderr, "Platform", sys.platform, "not really supported just yet"
	include_dirs = []
	library_dirs = []
	libraries = []

compile_args = [] # disable compile args for now

setup(
	name='lepton',
    version='0.6a', # *** REMEMBER TO UPDATE __init__.py ***
	description='Lepton: A high-performance, pluggable particle engine and API for Python',
	long_description='''\
Lepton is designed to make complex and beautiful particle effects possible,
and even easy from Python programs.

Lepton is under development, but even now it has some useful features:

- Native-code core for high-performance particle dynamics and rendering
- Pluggable particle controllers for specifying particle behavior
- Two pluggable OpenGL renderers, and two pygame renderers
- Spacial domains, used to control particle emission and behavior
- Modular architecture that lets you easily configure and customize the engine

The code includes several examples of how you can use the engine (using
pyglet). Note the engine itself does not depend on any other 3rd-party
libraries and simply requires the application to setup an OpenGL context in
order to render particles.
''',
	author='Casey Duncan, Harry Tormey & Contributors',
	author_email='py-lepton-users@googlegroups.com',
	url='http://code.google.com/p/py-lepton/',
	license='MIT',
    classifiers = [
        'Development Status :: 3 - Alpha',
        'Topic :: Multimedia :: Graphics',
        'License :: OSI Approved :: MIT License',
		'Intended Audience :: Developers',
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX',
    ],

    package_dir={'lepton': 'lepton'},
    packages=['lepton'],
	ext_modules=[
		Extension('lepton.group', 
			['lepton/group.c', 'lepton/vector.c', 
			 'lepton/groupmodule.c'], 
			include_dirs=include_dirs,
			library_dirs=library_dirs,
			libraries=libraries,
			extra_link_args=extra_link_args,
			extra_compile_args=compile_args,
		),
		Extension('lepton.renderer', 
			['lepton/group.c', 'lepton/renderermodule.c', 'lepton/vector.c'], 
			include_dirs=include_dirs,
			library_dirs=library_dirs,
			libraries=libraries,
			extra_link_args=extra_link_args,
			extra_compile_args=compile_args,
		),
		Extension('lepton._controller', 
			['lepton/group.c', 'lepton/vector.c', 
			 'lepton/controllermodule.c'], 
			include_dirs=include_dirs,
			library_dirs=library_dirs,
			libraries=libraries,
			extra_link_args=extra_link_args,
			extra_compile_args=compile_args,
		),
		Extension('lepton.emitter', 
			['lepton/group.c', 'lepton/groupmodule.c', 'lepton/vector.c', 
			 'lepton/fastrng.c', 'lepton/emittermodule.c'], 
			include_dirs=include_dirs,
			library_dirs=library_dirs,
			libraries=libraries,
			extra_link_args=extra_link_args,
			extra_compile_args=compile_args,
		),
	],
)
