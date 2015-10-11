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
"""Particles animated using a flip book texture"""

import os
import sys
import glob
import random
import pyglet
from pyglet.gl import *

from lepton import Particle, ParticleGroup, default_system
from lepton.renderer import BillboardRenderer
from lepton.texturizer import FlipBookTexturizer
from lepton.emitter import StaticEmitter
from lepton.domain import AABox, Plane
from lepton.controller import Gravity, Movement, Bounce, Fader


if sys.version_info < (3,):
    range = xrange

win = pyglet.window.Window(resizable=True, visible=False)
win.clear()

def on_resize(width, height):
	"""Setup 3D projection for window"""
	glViewport(0, 0, width, height)
	glMatrixMode(GL_PROJECTION)
	glLoadIdentity()
	gluPerspective(40, 1.0*width/height, 0.1, 1000.0)
	glMatrixMode(GL_MODELVIEW)
	glLoadIdentity()
win.on_resize = on_resize

glClearColor(1,1,1,1)
glEnable(GL_BLEND)
glShadeModel(GL_SMOOTH)
glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA)
glDisable(GL_DEPTH_TEST)

logo_files = glob.glob(os.path.join(os.path.dirname(__file__), 'logo_frames/logo*.png'))
images = [pyglet.image.load(f) for f in sorted(logo_files)]
texturizer = FlipBookTexturizer.from_images(images + images[-1::-1], 0.075)

group = ParticleGroup(
	controllers=[
		Fader(fade_out_start=50.0, fade_out_end=58.5)
		],
	renderer=BillboardRenderer(texturizer))
template = Particle(
	size=(1,1,0),
	color=(1,1,1,1),
	)
positions = set([((x - 25) * 1.4 + (y % 2.0) * 0.7,
                 (y - 25) * 1.4,
				 (z - 25) * 1.4 + (y % 2.0) * 0.7)
	for x in range(50) for y in range(50) for z in range(50)])
group.new(template)
for i in range(12000):
	group.new(template, position=positions.pop(), age=49.35)

win.set_visible(True)
pyglet.clock.schedule_interval(default_system.update, (1.0/40.0))
pyglet.clock.set_fps_limit(None)

label = pyglet.text.Label("Lepton", font_name="Helvetica", font_size=72, bold=True,
	color=(0,0,0,0), anchor_x="center", x=0, y=-200)

def fade_in_label(dt):
	global label
	alpha = label.color[-1] + 2
	label.color = label.color[:-1] + (min(alpha, 255),)
	pyglet.clock.schedule_once(fade_in_label, 1.0/40.0)
pyglet.clock.schedule_once(fade_in_label, 6)

t = 90
d = -80
approach = 0.6
rot = 1.0


@win.event
def on_draw():
	global t, d, approach, rot
	win.clear()
	glLoadIdentity()
	glTranslatef(0, 0, -950)
	label.draw()
	glLoadIdentity()
	t += rot
	rot = rot * 0.995
	d += approach
	approach *= 0.9924
	glTranslatef(0, 0, min(d, -4.1))
	glRotatef(t, -1, -1, -1)
	default_system.draw()

if __name__ == '__main__':
	pyglet.app.run()

