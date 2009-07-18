#!/usr/bin/env python

# $Id$

import sys
from distutils.core import setup, Extension

extra_link_args = []
macros = [('GLEW_STATIC', None)]

if sys.platform.startswith('linux'):
	include_dirs = ['/usr/include', '/usr/local/include/', 
		'/usr/X11/include', '/usr/X11R6/include', 'glew/include']
	library_dirs = ['/usr/lib', '/usr/local/lib', 
		'/usr/X11/lib', '/usr/X11R6/lib']
	libraries = ['GL', 'X11', 'Xext']
elif sys.platform == 'cygwin':
	include_dirs = ['/usr/include', '/usr/include/win32api/', 'glew/include']
	library_dirs = ['/usr/lib']
	libraries = ['opengl32']
elif sys.platform == 'win32':
	include_dirs = ['../include', 'glew/include']
	library_dirs = ['../lib']
	libraries = ['opengl32']
elif sys.platform == 'darwin':
	include_dirs = ['/System/Library/Frameworks/OpenGL.framework/Headers', 
		'/System/Library/Frameworks/GLUT.framework/Headers', 'glew/include']
	library_dirs = []
	libraries = []
	extra_link_args = ['-framework:OpenGL']
else:
	print >>sys.stderr, "Platform", sys.platform, "not really supported just yet"
	include_dirs = ['glew/include']
	library_dirs = []
	libraries = []

compile_args = [] # disable compile args for now

setup(
	name='lepton',
    version='1.0b2', # *** REMEMBER TO UPDATE __init__.py ***
	description='Lepton: A high-performance, pluggable particle engine and API for Python',
	long_description='''\
Lepton is designed to make complex and beautiful particle effects possible,
and even easy from Python programs.

Lepton provides the following core features:

- Native-code core for high-performance particle dynamics and rendering
- Pluggable particle controllers for specifying particle behavior
- Two pluggable OpenGL renderers, and two pygame renderers
- Spacial domains, used to control particle emission and behavior
- Easy to use and powerful texture support, including animation
- Modular architecture that lets you easily configure and customize the engine

The code includes several examples of how you can use the engine (using
pyglet, and pygame). Note the engine itself does not depend on any other 3rd-party
libraries and simply requires the application to setup an OpenGL context in
order to render particles.

You can download binary releases or browse the source code at our Google code site.
''',
	author='Casey Duncan, Harry Tormey & Contributors',
	author_email='py-lepton-users@googlegroups.com',
	url='http://code.google.com/p/py-lepton/',
	license='MIT',
    classifiers = [
        'Development Status :: 4 - Beta',
        'Topic :: Multimedia :: Graphics',
        'License :: OSI Approved :: MIT License',
		'Intended Audience :: Developers',
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX',
    ],

    package_dir={'lepton': 'lepton', 'lepton.examples': 'examples'},
	package_data={'lepton.examples': ['*.png', '*.bmp', 'logo_frames/*.png']},
    packages=['lepton', 'lepton.examples'],
	ext_modules=[
		Extension('lepton.group', 
			['lepton/group.c', 'lepton/groupmodule.c'], 
			include_dirs=include_dirs,
			library_dirs=library_dirs,
			libraries=libraries,
			extra_link_args=extra_link_args,
			extra_compile_args=compile_args,
			define_macros=macros,
		),
		Extension('lepton.renderer', 
			['lepton/group.c', 'lepton/renderermodule.c',
			 'lepton/controllermodule.c', 'lepton/groupmodule.c',
			 'glew/src/glew.c'], 
			include_dirs=include_dirs,
			library_dirs=library_dirs,
			libraries=libraries,
			extra_link_args=extra_link_args,
			extra_compile_args=compile_args,
			define_macros=macros,
		),
		Extension('lepton._texturizer', 
			['lepton/group.c', 'lepton/texturizermodule.c', 
			 'lepton/renderermodule.c', 'lepton/controllermodule.c', 
			 'lepton/groupmodule.c', 'glew/src/glew.c'], 
			include_dirs=include_dirs,
			library_dirs=library_dirs,
			libraries=libraries,
			extra_link_args=extra_link_args,
			extra_compile_args=compile_args,
			define_macros=macros,
		),
		Extension('lepton._controller', 
			['lepton/group.c', 'lepton/groupmodule.c', 
			 'lepton/controllermodule.c'], 
			include_dirs=include_dirs,
			library_dirs=library_dirs,
			libraries=libraries,
			extra_link_args=extra_link_args,
			extra_compile_args=compile_args,
			define_macros=macros,
		),
		Extension('lepton.emitter', 
			['lepton/group.c', 'lepton/groupmodule.c',
			 'lepton/fastrng.c', 'lepton/emittermodule.c'], 
			include_dirs=include_dirs,
			library_dirs=library_dirs,
			libraries=libraries,
			extra_link_args=extra_link_args,
			extra_compile_args=compile_args,
			define_macros=macros,
		),
		Extension('lepton._domain', 
			['lepton/group.c', 'lepton/groupmodule.c',
			 'lepton/fastrng.c', 'lepton/domainmodule.c'], 
			include_dirs=include_dirs,
			library_dirs=library_dirs,
			libraries=libraries,
			extra_link_args=extra_link_args,
			extra_compile_args=compile_args,
			define_macros=macros,
		),
	],
)
