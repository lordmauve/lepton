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
"""Fire simulation using point sprites"""

__version__ = '$Id$'

import os
from pyglet import image
from pyglet.gl import *

from lepton import Particle, ParticleGroup, default_system
from lepton.renderer import PointRenderer
from lepton.texturizer import SpriteTexturizer, create_point_texture
from lepton.emitter import StaticEmitter
from lepton.domain import Line
from lepton.controller import Gravity, Lifetime, Movement, Fader, ColorBlender

win = pyglet.window.Window(resizable=True, visible=False)
win.clear()

glEnable(GL_BLEND)
glShadeModel(GL_SMOOTH)
glBlendFunc(GL_SRC_ALPHA,GL_ONE)
glDisable(GL_DEPTH_TEST)

flame = StaticEmitter(
	rate=500,
	template=Particle(
		position=(300,25,0), 
		velocity=(0,0,0), 
		color=(1,1,1,1),
	),
	position=Line((win.width/2 - 85, -15, 0), (win.width/2 + 85, -15, 0)),
	deviation=Particle(position=(10,0,0), velocity=(7,50,0), age=0.75)
)

default_system.add_global_controller(
	Lifetime(6),
	Gravity((0,20,0)), 
	Movement(), 
	ColorBlender(
		[(0, (0,0,0.5,0)), 
		(0.5, (0,0,0.5,0.2)), 
		(0.75, (0,0.5,1,0.6)), 
		(1.5, (1,1,0,0.2)), 
		(2.7, (0.9,0.2,0,0.4)), 
		(3.2, (0.6,0.1,0.05,0.2)), 
		(4.0, (0.8,0.8,0.8,0.1)),
		(6.0, (0.8,0.8,0.8,0)), ]
	),
)

group = ParticleGroup(controllers=[flame], 
	renderer=PointRenderer(64, SpriteTexturizer(create_point_texture(64, 5))))

win.set_visible(True)
pyglet.clock.schedule_interval(default_system.update, (1.0/30.0))
pyglet.clock.set_fps_limit(None)

@win.event
def on_draw():
	win.clear()
	glLoadIdentity()
	default_system.draw()

if __name__ == '__main__':
	pyglet.app.run()
