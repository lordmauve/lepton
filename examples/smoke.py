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
"""Smoke simulation using alpha-blended point particles"""

__version__ = '$Id$'

from pyglet import image
from pyglet.gl import *
import sys

from lepton import Particle, ParticleGroup, default_system
from lepton.renderer import PointRenderer
from lepton.emitter import StaticEmitter
from lepton.controller import Gravity, Lifetime, Movement, Fader

if __name__ == '__main__':
	win = pyglet.window.Window(resizable=True, visible=False)
	win.clear()

	def resize(widthWindow, heightWindow):
		"""Initial settings for the OpenGL state machine, clear color, window size, etc"""
		glEnable(GL_BLEND)
		glEnable(GL_POINT_SMOOTH)
		glShadeModel(GL_SMOOTH)# Enables Smooth Shading
		glBlendFunc(GL_SRC_ALPHA,GL_ONE)#Type Of Blending To Perform
		glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);#Really Nice Perspective Calculations
		glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);#Really Nice Point Smoothing
		glDisable(GL_DEPTH_TEST)

	smoke = StaticEmitter(
		rate=50,
		template=Particle(
			position=(300,25,0), 
			velocity=(0,25,0), 
			color=(0.8,0.8,0.8,0.005),
		),
		deviation=Particle(position=(5,5,0), velocity=(2.5,5,0), color=(0.2,0.2,0.2,0.0))
	)

	default_system.add_global_controller(
		Lifetime(20),
		Gravity((0, -1, 0)), 
		Movement(), 
		Fader(fade_in_end=5.0, max_alpha=0.005, fade_out_start=12, fade_out_end=20),
	)
	group1 = ParticleGroup(controllers=[smoke], renderer=PointRenderer(point_size=45))

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
