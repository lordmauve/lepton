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

from lepton import Particle, ParticleGroup, default_system, domain
from lepton.renderer import PointRenderer
from lepton.texturizer import SpriteTexturizer, create_point_texture
from lepton.emitter import StaticEmitter, PerParticleEmitter
from lepton.controller import Gravity, Lifetime, Movement, Fader, ColorBlender

spark_tex = image.load(os.path.join(os.path.dirname(__file__), 'flare3.png')).get_texture()
spark_texturizer = SpriteTexturizer(spark_tex.id)
trail_texturizer = SpriteTexturizer(create_point_texture(8, 50))

class Kaboom:
	
	lifetime = 5

	def __init__(self):
		color=(uniform(0,1), uniform(0,1), uniform(0,1), 1)
		while max(color[:3]) < 0.9:
			color=(uniform(0,1), uniform(0,1), uniform(0,1), 1)

		spark_emitter = StaticEmitter(
			template=Particle(
				position=(uniform(-50, 50), uniform(-30, 30), uniform(-30, 30)), 
				color=color), 
			deviation=Particle(
				velocity=(gauss(0, 5), gauss(0, 5), gauss(0, 5)),
				age=1.5),
			velocity=domain.Sphere((0, gauss(40, 20), 0), 60, 60))

		self.sparks = ParticleGroup(
			controllers=[
				Lifetime(self.lifetime * 0.75),
				Movement(damping=0.93),
				ColorBlender([(0, (1,1,1,1)), (2, color), (self.lifetime, color)]),
				Fader(fade_out_start=1.0, fade_out_end=self.lifetime * 0.5),
			],
			renderer=PointRenderer(abs(gauss(10, 3)), spark_texturizer))

		spark_emitter.emit(int(gauss(60, 40)) + 50, self.sparks)

		spread = abs(gauss(0.4, 1.0))
		self.trail_emitter = PerParticleEmitter(self.sparks, rate=uniform(5,30),
			template=Particle(
				color=color),
			deviation=Particle(
				velocity=(spread, spread, spread),
				age=self.lifetime * 0.75))

		self.trails = ParticleGroup(
			controllers=[
				Lifetime(self.lifetime * 1.5),
				Movement(damping=0.83),
				ColorBlender([(0, (1,1,1,1)), (1, color), (self.lifetime, color)]),
				Fader(max_alpha=0.75, fade_out_start=0, fade_out_end=gauss(self.lifetime, self.lifetime*0.3)),
				self.trail_emitter
			],
			renderer=PointRenderer(10, trail_texturizer))

		pyglet.clock.schedule_once(self.die, self.lifetime * 2)
	
	def reduce_trail(self, dt=None):
		if self.trail_emitter.rate > 0:
			self.trail_emitter.rate -= 1
	
	def die(self, dt=None):
		default_system.remove_group(self.sparks)
		default_system.remove_group(self.trails)

win = pyglet.window.Window(resizable=True, visible=False)
win.clear()

def on_resize(width, height):
	"""Setup 3D projection for window"""
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
	'''
	glBindTexture(GL_TEXTURE_2D, 1)
	glEnable(GL_TEXTURE_2D)
	glEnable(GL_POINT_SPRITE)
	glPointSize(100);
	glBegin(GL_POINTS)
	glVertex2f(0,0)
	glEnd()
	glBindTexture(GL_TEXTURE_2D, 2)
	glEnable(GL_TEXTURE_2D)
	glEnable(GL_POINT_SPRITE)
	glPointSize(100);
	glBegin(GL_POINTS)
	glVertex2f(50,0)
	glEnd()
	glBindTexture(GL_TEXTURE_2D, 0)
	'''


if __name__ == '__main__':
	pyglet.app.run()
