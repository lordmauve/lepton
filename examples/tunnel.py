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
"""Infinite tunnel using textured billboard quads"""

__version__ = '$Id$'

import os
import math
from pyglet import image
from pyglet.gl import *

from lepton import Particle, ParticleGroup, default_system
from lepton.renderer import BillboardRenderer
from lepton.texturizer import SpriteTexturizer
from lepton.emitter import StaticEmitter
from lepton.controller import Gravity, Movement, Fader, Growth, Collector
from lepton import domain

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

disc = domain.Disc((0,0,-50), (0, 0, 1), 1.5, 1.5)
viewer_plane = domain.Plane((0,0,0), (0,0,-1))

jet = StaticEmitter(
	rate=2000,
	position=disc,
	template=Particle(
		color=(1,1,0),
	),
	deviation=Particle(
		velocity=(0,0,15), 
		up=(0,0,math.pi),
		color=(0.1, 0.1, 0.1))
)

default_system.add_global_controller(
	Movement(max_velocity=10),
	Collector(viewer_plane),
	Gravity((0,0,15)),
	Growth(0.17),
	Fader(fade_in_end=0, max_alpha=0.3, fade_out_start=0, fade_out_end=8.0),
)

texture = image.load(os.path.join(os.path.dirname(__file__), 'Particle.bmp')).get_texture()
group = ParticleGroup(controllers=[jet], 
	renderer=BillboardRenderer(SpriteTexturizer(texture.id)))

default_system.run_ahead(5, 40)

pyglet.clock.schedule_interval(default_system.update, (1.0/30.0))
pyglet.clock.set_fps_limit(None)
time = 0

def ring(dt):
	"""Emit a ring of particles periodically"""
	jet.emit(1000, group)
pyglet.clock.schedule_interval(ring, 5)

def vary_radius(dt):
	"""Vary the disc radius over time"""
	global time
	time += dt
	disc.inner_radius = disc.outer_radius = 2.5 + math.sin(time / 2.0) * 1.5
pyglet.clock.schedule_interval(vary_radius, 1.0/10.0)
vary_radius(0)

@win.event
def on_draw():
	win.clear()
	glLoadIdentity()
	default_system.draw()

if __name__ == '__main__':
	win.set_visible(True)
	pyglet.app.run()
