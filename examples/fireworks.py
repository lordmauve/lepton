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
"""Fireworks simulation to show off the per-particle emitter"""

__version__ = '$Id$'

import os
import math
from random import expovariate, uniform, gauss
from pyglet import image
from pyglet.gl import *

from lepton import Particle, ParticleGroup, default_system
from lepton.renderer import BillboardRenderer
from lepton.emitter import StaticEmitter, PerParticleEmitter
from lepton.controller import Gravity, Lifetime, Movement, Fader, ColorBlender

class Kaboom:
	
	lifetime = 5

	def __init__(self):
		color=(uniform(0,1), uniform(0,1), uniform(0,1), 1)
		while sum(color) < 2.5:
			color=(uniform(0,1), uniform(0,1), uniform(0,1), 1)

		spark_emitter = StaticEmitter(
			template=Particle(
				position=(uniform(-50, 50), uniform(-30, 30), uniform(-30, 30)), 
				velocity=(0, gauss(40, 20), 0),
				color=color, 
				size=(2.5,2.5,0)),
			deviation=Particle(
				velocity=(gauss(35,10)+2,gauss(35,10)+2,gauss(35,10)+2), 
				age=1.5))

		self.sparks = ParticleGroup(
			controllers=[
				Lifetime(self.lifetime * 0.75),
				Movement(max_velocity=90, damping=0.93),
				Fader(fade_out_start=1.0, fade_out_end=self.lifetime * 0.5),
			],
			renderer=BillboardRenderer())

		spark_emitter.emit(int(gauss(50, 40)) + 50, self.sparks)

		spread = uniform(0.2, 4)
		self.trail_emitter = PerParticleEmitter(self.sparks, rate=uniform(5,20),
			template=Particle(
				color=color,
				size=(1,1,0)),
			deviation=Particle(
				velocity=(spread, spread, spread),
				size=(0.05,0.05,0),
				age=self.lifetime * 0.75))

		self.trails = ParticleGroup(
			controllers=[
				Lifetime(self.lifetime * 1.5),
				Movement(damping=0.83),
				ColorBlender([(0, (1,1,1,1)), (1, color), (self.lifetime, color)]),
				Fader(fade_out_start=0, fade_out_end=gauss(self.lifetime, self.lifetime*0.3)),
				self.trail_emitter
			],
			renderer=BillboardRenderer())

		pyglet.clock.schedule_once(self.die, self.lifetime * 2)
	
	def reduce_trail(self, dt=None):
		if self.trail_emitter.rate > 0:
			self.trail_emitter.rate -= 1
	
	def die(self, dt=None):
		default_system.remove_group(self.sparks)
		default_system.remove_group(self.trails)



if __name__ == '__main__':
	win = pyglet.window.Window(resizable=True, visible=False)
	win.clear()

	def on_resize(width, height):
		"""Initial settings for the OpenGL state machine, clear color, window size, etc"""
		glViewport(0, 0, width, height)
		glMatrixMode(GL_PROJECTION)
		glLoadIdentity()
		gluPerspective(70, 1.0*width/height, 0.1, 1000.0)
		glMatrixMode(GL_MODELVIEW)
		glLoadIdentity()
	win.on_resize = on_resize

	yrot = 0.0

	@win.event
	def on_mouse_motion(x, y, dx, dy):
		global yrot
		yrot += dx * 0.3

	glEnable(GL_BLEND)
	glShadeModel(GL_SMOOTH)
	glBlendFunc(GL_SRC_ALPHA,GL_ONE)
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	glDisable(GL_DEPTH_TEST)
	
	spark_tex = image.load(os.path.join(os.path.dirname(__file__), 'flare3.png')).texture
	glEnable(spark_tex.target)
	glTexParameteri(spark_tex.target, GL_TEXTURE_WRAP_S, GL_CLAMP)
	glTexParameteri(spark_tex.target, GL_TEXTURE_WRAP_T, GL_CLAMP)
	glTexParameteri(spark_tex.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
	glTexParameteri(spark_tex.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
	glBindTexture(spark_tex.target, spark_tex.id)

	default_system.add_global_controller(
		Gravity((0,-15,0))
	)

	MEAN_FIRE_INTERVAL = 3.0
	
	def fire(dt=None):
		Kaboom()
		pyglet.clock.schedule_once(fire, expovariate(1.0 / (MEAN_FIRE_INTERVAL - 1)) + 1)

	fire()
	win.set_visible(True)
	pyglet.clock.schedule_interval(default_system.update, (1.0/30.0))
	pyglet.clock.set_fps_limit(None)

	@win.event
	def on_draw():
		global yrot
		win.clear()
		glLoadIdentity()
		glTranslatef(0, 0, -100)
		glRotatef(yrot, 0.0, 1.0, 0.0)
		default_system.draw()

	pyglet.app.run()
