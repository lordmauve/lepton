#############################################################################
#
# Copyright (c) 2009 by Casey Duncan and contributors
# All Rights Reserved.
#
# This software is subject to the provisions of the MIT License
# A copy of the license should accompany this distribution.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#
#############################################################################
"""Raining letters demonstrates using texture atlases
with a SpriteTexturizer
"""

__version__ = '$Id$'

import os
import string
import pyglet
from pyglet.gl import *

from lepton import Particle, ParticleGroup, default_system
from lepton.renderer import BillboardRenderer
from lepton.texturizer import SpriteTexturizer
from lepton.emitter import StaticEmitter
from lepton.domain import AABox, Plane
from lepton.controller import Gravity, Movement, Collector, Fader

win = pyglet.window.Window(resizable=True, visible=False)
win.clear()

def on_resize(width, height):
	"""Setup 3D projection for window"""
	glViewport(0, 0, width, height)
	glMatrixMode(GL_PROJECTION)
	glLoadIdentity()
	gluPerspective(40, 1.0*width/height, 0.1, 400.0)
	glMatrixMode(GL_MODELVIEW)
	glLoadIdentity()
win.on_resize = on_resize

glEnable(GL_BLEND)
glShadeModel(GL_SMOOTH)
glBlendFunc(GL_SRC_ALPHA,GL_ONE)
glDisable(GL_DEPTH_TEST)

emitter = StaticEmitter(
	rate=250,
	template=Particle(
		size=(6,6,0),
		velocity=(0,-10,20),
	),
	color=[(1,0,0), (0,1,0), (0,0,1), (1,1,0), (1,0.5,0), (0.5,0,1)],
	rotation=[(0,0,0.3), (0,0,-0.3)],
	position=AABox((-100, 70, -100), (100, 70, -300)),
	deviation=Particle(
		color=(0.1, 0.1, 0.1, 0),
		rotation=(0,0,0.1),
		velocity=(0,5,0),
	)
)

default_system.add_global_controller(
	Movement(),
	Collector(Plane((0, 0, 0), (0, 0, -1))),
	Fader(fade_in_end=15.0),
)

font = pyglet.font.load(size=72)
# Try to force all glyphs into a single texture
font.texture_width = font.texture_height = 1024
letter_textures = font.get_glyphs(string.ascii_lowercase)
texturizer = SpriteTexturizer(
	letter_textures[0].texture.id,
	coords=[tex.tex_coords for tex in letter_textures],
	aspect_adjust_width=True)

group = ParticleGroup(controllers=[emitter],
	renderer=BillboardRenderer(texturizer))

win.set_visible(True)
pyglet.clock.schedule_interval(default_system.update, (1.0/40.0))
pyglet.clock.set_fps_limit(None)

@win.event
def on_draw():
	win.clear()
	glLoadIdentity()
	default_system.draw()

if __name__ == '__main__':
	pyglet.app.run()

