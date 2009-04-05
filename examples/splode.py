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
"""Explosion using textured billboard quads"""

__version__ = '$Id$'

import os
import math
import time
from pyglet import image
from pyglet.gl import *

from lepton import Particle, ParticleGroup, default_system
from lepton.renderer import BillboardRenderer
from lepton.texturizer import SpriteTexturizer
from lepton.emitter import StaticEmitter
from lepton.controller import Gravity, Lifetime, Movement, Fader, ColorBlender

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

glEnable(GL_BLEND)
glShadeModel(GL_SMOOTH)
glBlendFunc(GL_SRC_ALPHA,GL_ONE)
glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
glDisable(GL_DEPTH_TEST)

spark_tex = image.load(os.path.join(os.path.dirname(__file__), 'flare3.png')).get_texture()

sparks = ParticleGroup(
	controllers=[
		Lifetime(3),
		Movement(damping=0.93),
		Fader(fade_out_start=0.75, fade_out_end=3.0),
	],
	renderer=BillboardRenderer(SpriteTexturizer(spark_tex.id)))

spark_emitter = StaticEmitter(
	template=Particle(
		position=(0,0,-100), 
		color=(1,1,1), 
		size=(2,2,0)),
	deviation=Particle(
		position=(1,1,1), 
		velocity=(75,75,75), 
		size=(0.2,0.2,0),
		age=1.5))
spark_emitter.emit(400, sparks)

fire_tex = image.load(os.path.join(os.path.dirname(__file__), 'puff.png')).get_texture()

fire = ParticleGroup(
	controllers=[
		Lifetime(4),
		Movement(damping=0.95),
		Fader(fade_in_start=0, start_alpha=0, fade_in_end=0.5, max_alpha=0.4, 
			fade_out_start=1.0, fade_out_end=4.0)
	],
	renderer=BillboardRenderer(SpriteTexturizer(fire_tex.id)))

fire_emitter = StaticEmitter(
	template=Particle(
		position=(0,0,-100), 
		size=(20,20,0)),
	deviation=Particle(
		position=(2,2,2), 
		velocity=(20,20,20), 
		size=(5,5,0),
			up=(0,0,math.pi*2), 
			rotation=(0,0,math.pi*0.03),),
		color=[(0.5,0,0), (0.5,0.5,0.5), (0.4,0.1,0.1), (0.85,0.3,0)],
	)
fire_emitter.emit(400, fire)

@win.event
def on_draw():
	win.clear()
	glLoadIdentity()
	default_system.draw()

if __name__ == '__main__':
	win.set_visible(True)
	time.sleep(0.5)
	pyglet.clock.schedule_interval(default_system.update, (1.0/30.0))
	pyglet.app.run()
