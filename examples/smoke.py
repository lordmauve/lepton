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
from lepton.texturizer import SpriteTexturizer, create_point_texture
from lepton.emitter import StaticEmitter
from lepton.controller import Gravity, Lifetime, Movement, Fader

win = pyglet.window.Window(resizable=True, visible=False)
win.clear()

glEnable(GL_BLEND)
glShadeModel(GL_SMOOTH)
glBlendFunc(GL_SRC_ALPHA,GL_ONE)
glDisable(GL_DEPTH_TEST)

smoke = StaticEmitter(
	rate=10,
	template=Particle(
		position=(300,25,0), 
		velocity=(0,35,0), 
		color=(0.8,0.8,0.8,0.005),
	),
	deviation=Particle(
		position=(10,5,0), 
		velocity=(3,6,0), 
		color=(0.05,0.05,0.05,0.0),
	)
)

default_system.add_global_controller(
	Lifetime(20),
	Gravity((0, -2, 0)), 
	Movement(), 
	Fader(fade_in_end=1.5, max_alpha=0.05, fade_out_start=12, fade_out_end=20),
)
group1 = ParticleGroup(controllers=[smoke],
	renderer=PointRenderer(64, SpriteTexturizer(create_point_texture(64, 1))))

win.set_visible(True)
pyglet.clock.schedule_interval(default_system.update, (1.0/30.0))

@win.event
def on_draw():
	win.clear()
	glLoadIdentity()
	default_system.draw()

if __name__ == '__main__':
	pyglet.app.run()
