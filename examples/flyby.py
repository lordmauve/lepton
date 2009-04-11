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
"""Flying comet using textured billboard quads"""

__version__ = '$Id$'

import os
import math
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
	"""Initial settings for the OpenGL state machine, clear color, window size, etc"""
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

comet = StaticEmitter(
	rate=600,
	template=Particle(
		size=(2,2,0),
		color=(1,1,0),
	),
	deviation=Particle(
		velocity=(0.7,0.7,0.7), 
		up=(0,0,math.pi),
		rotation=(0,0,math.pi),
		color=(0.5, 0.5, 0.5))
)

default_system.add_global_controller(
	Lifetime(3.0),
	Movement(min_velocity=5), 
	Fader(max_alpha=0.7, fade_out_start=1, fade_out_end=3.0),
)

images = [image.load(os.path.join(os.path.dirname(__file__), 'flare%s.png' % (i+1))) 
	for i in range(4)]
group = ParticleGroup(controllers=[comet], 
	renderer=BillboardRenderer(SpriteTexturizer.from_images(images)))

pyglet.clock.schedule_interval(default_system.update, (1.0/30.0))
pyglet.clock.set_fps_limit(None)
arc_radius = 150
angle = math.pi * 0.7
speed = 1.0

def move_comet(td):
	global angle, arc_radius, speed
	comet.template.position = (
		-math.sin(angle) * arc_radius * 0.3, 
		 math.sin(angle * 0.7) * arc_radius * 0.03,
		-math.cos(angle) * arc_radius - arc_radius * 1.05)
	comet.template.velocity = (
		comet.template.position.x*0.05 - comet.template.last_position.x,
		comet.template.position.y*0.05 - comet.template.last_position.y,
		comet.template.position.z*0.05 - comet.template.last_position.z)
	angle -= td * speed
pyglet.clock.schedule_interval(move_comet, (1.0/30.0))
move_comet(0)

@win.event
def on_draw():
	win.clear()
	glLoadIdentity()
	default_system.draw()

if __name__ == '__main__':
	win.set_visible(True)
	pyglet.app.run()
