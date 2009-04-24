#############################################################################
#
# Copyright (c) 1008 by Casey Duncan and contributors
# All Rights Reserved.
#
# This software is subject to the provisions of the MIT License
# A copy of the license should accompany this distribution.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#
#############################################################################
"""Visual test of domain generated vectors"""

__version__ = '$Id$'

import os
import math
from pyglet import image
from pyglet.gl import *

from lepton import Particle, ParticleGroup, default_system
from lepton.texturizer import SpriteTexturizer, create_point_texture
from lepton.renderer import PointRenderer
from lepton.emitter import StaticEmitter
from lepton.controller import Lifetime, Fader
from lepton import domain

win = pyglet.window.Window(resizable=True, visible=False)
win.clear()

def on_resize(width, height):
	"""Setup 3D projection"""
	glViewport(0, 0, width, height)
	glMatrixMode(GL_PROJECTION)
	glLoadIdentity()
	gluPerspective(30, 1.0*width/height, 0.1, 1000.0)
	glMatrixMode(GL_MODELVIEW)
	glLoadIdentity()
win.on_resize = on_resize

glEnable(GL_BLEND)
glShadeModel(GL_SMOOTH)
glBlendFunc(GL_SRC_ALPHA,GL_ONE)
glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
glDisable(GL_DEPTH_TEST)

renderer = PointRenderer(7, SpriteTexturizer(create_point_texture(16, 10)))

domains = [
	domain.Sphere((0,0,0), 1),
	domain.Disc((0,0,0), (-1,0,0), 1),
	domain.Cylinder((-0.5,0,0), (0.5,0,0), 1),
	domain.Cone((-0.5,0,0), (0.5,0,0), 1),
	]

groups = [
	ParticleGroup(
		controllers=[
			StaticEmitter(
				rate=15000,
				position=domain,
				template=Particle(
					color=(1,1,1),
					size=(.1,.1,0),
				)
			)
			],
		renderer=renderer)
	for domain in domains]

default_system.add_global_controller(
	Lifetime(0.5),
	Fader(max_alpha=0.7,fade_out_start=0.1, fade_out_end=0.5),
)

pyglet.clock.schedule_interval(default_system.update, (1.0/40.0))
pyglet.clock.set_fps_limit(None)

translations = [(-1.1,-1.1), (1.1,-1.1), (-1.1,1.1), (1.1,1.1)]
rot = 0

@win.event
def on_draw():
	global rot
	win.clear()
	for (xt, yt), group in zip(translations, groups):
		glLoadIdentity()
		glTranslatef(xt, yt, -8)
		glRotatef(rot, 0, 1, 0)
		group.draw()
	for domain in domains:
		domain.inner_radius = math.sin(rot * 0.05) * 0.5 + 0.5
	rot += 0.75

if __name__ == '__main__':
	win.set_visible(True)
	pyglet.app.run()
