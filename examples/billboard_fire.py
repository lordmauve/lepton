"""Fire simulation using alpha-blended point particles"""

__version__ = '$Id$'

import os
from pyglet import image
from pyglet.gl import *
import sys
sys.path.append('../..')

from particle import Particle, ParticleGroup, default_system
from particle.renderer import BillboardRenderer
from particle.emitter import StaticEmitter
from particle.controller import Gravity, Lifetime, Movement, Fader, ColorBlender

if __name__ == '__main__':
	"""This file is meant to show how to use controller, renderer, system, particle, group to implement
	the effect in pyparticle.py"""
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
			[(0, (0,0.5,0,0)), 
			(0.5, (0,0.5,0,0.2)), 
			(1.0, (0,1,0.5,0.08)), 
			(1.5, (1,0,1,0.12)), 
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

	def print_fps(dt):
		print len(group), group.killed_count(), pyglet.clock.get_fps()

	win.resize = resize
	win.set_visible(True)
	win.resize(win.width, win.height)
	pyglet.clock.schedule_interval(default_system.update, (1.0/30.0))
	pyglet.clock.schedule_interval(print_fps, 1.0)
	pyglet.clock.set_fps_limit(None)

	#fps_display = pyglet.clock.ClockDisplay()

	@win.event
	def on_draw():
		win.clear()
		glLoadIdentity()
		#fps_display.draw()
		default_system.draw()

	pyglet.app.run()
