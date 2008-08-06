#############################################################################
#
# Copyright (c) 2008 by Casey Duncan and contributors
# All Rights Reserved.
#
# This software is subject to the provisions of the MIT License
# A copy of the license should accompany this distribution.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#
#############################################################################
"""Fire simulation using alpha-blended point particles"""

__version__ = '$Id$'

from pyglet import image
from pyglet.gl import *
import sys
sys.path.append('../..')

from particle import Particle, ParticleGroup, default_system
from particle.renderer import PointRenderer
from particle.emitter import StaticEmitter
from particle.controller import Gravity, Lifetime, Movement, Fader, ColorBlender

if __name__ == '__main__':
	"""This file is meant to show how to use controller, renderer, system, particle, group to implement
	the effect in pyparticle.py"""
	win = pyglet.window.Window(resizable=True, visible=False)


	def resize(widthWindow, heightWindow):
		"""Initial settings for the OpenGL state machine, clear color, window size, etc"""
		glEnable(GL_BLEND)
		glEnable(GL_POINT_SMOOTH)
		glShadeModel(GL_SMOOTH)# Enables Smooth Shading
		glBlendFunc(GL_SRC_ALPHA,GL_ONE)#Type Of Blending To Perform
		glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);#Really Nice Perspective Calculations
		glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);#Really Nice Point Smoothing
		glDisable(GL_DEPTH_TEST)

	flame = StaticEmitter(
		rate=200,
		template=Particle(
			position=(300,25,0), 
			velocity=(0,0,0), 
		),
		deviation=Particle(position=(8,0,0), velocity=(7,20,0), age=1.5)
	)

	default_system.add_global_controller(
		Lifetime(6),
		Gravity((0,30,0)), 
		Movement(), 
		ColorBlender(
			[(0, (0,0,1,0)), 
			(0.6, (0,0,1,0.04)), 
			(1.85, (1,1,0,0.04)), 
			(3.0, (1,0,0,0.04)), 
			(5, (0.8,0.8,0.8,0.01)), 
			(6, (0.8,0.8,0.8,0))]
		),
		#Fader(fade_in_end=5.0, max_alpha=0.005, fade_out_start=17, fade_out_end=20),
	)
	group = ParticleGroup(controllers=[flame], renderer=PointRenderer(point_size=30))

	win.resize = resize
	win.set_visible(True)
	win.resize(win.width, win.height)
	pyglet.clock.schedule_interval(default_system.update, (1.0/30.0))

	@win.event
	def on_draw():
		win.clear()
		glLoadIdentity()
		default_system.draw()

	pyglet.app.run()
