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
"""Fire simulation using textured billboard quads"""

__version__ = '$Id$'

import os
from pyglet import image
from pyglet.gl import *

from lepton import Particle, ParticleGroup, default_system
from lepton.renderer import BillboardRenderer
from lepton.emitter import StaticEmitter
from lepton.controller import Gravity, Lifetime, Movement, Fader, ColorBlender

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

	flame = StaticEmitter(
		rate=150,
		template=Particle(
			position=(300,25,0), 
			velocity=(0,0,0), 
			size=(85,85,0),
		),
		deviation=Particle(position=(20,0,0), velocity=(10,50,0), age=0.5)
	)

	default_system.add_global_controller(
		Lifetime(6),
		Gravity((0,20,0)), 
		Movement(), 
		ColorBlender(
			[(0, (0,0,0.5,0)), 
			(0.5, (0,0,0.5,0.2)), 
			(1.0, (0,0.5,1,0.08)), 
			(1.5, (1,1,0,0.12)), 
			(3.0, (1,0,0,0.12)), 
			(5.0, (0.8,0.8,0.8,0.05)),
			(6.0, (0.8,0.8,0.8,0)), ]
		),
		#Fader(fade_in_end=5.0, max_alpha=0.005, fade_out_start=17, fade_out_end=20),
	)
	group = ParticleGroup(controllers=[flame], renderer=BillboardRenderer())
	
	texture = image.load(os.path.join(os.path.dirname(__file__), 'Particle.bmp')).texture
	glEnable(texture.target)
	glTexParameteri(texture.target, GL_TEXTURE_WRAP_S, GL_CLAMP)
	glTexParameteri(texture.target, GL_TEXTURE_WRAP_T, GL_CLAMP)
	glTexParameteri(texture.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
	glTexParameteri(texture.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
	glBindTexture(texture.target, texture.id)

	win.resize = resize
	win.set_visible(True)
	win.resize(win.width, win.height)
	pyglet.clock.schedule_interval(default_system.update, (1.0/30.0))
	pyglet.clock.set_fps_limit(None)

	@win.event
	def on_draw():
		win.clear()
		glLoadIdentity()
		default_system.draw()

	pyglet.app.run()
