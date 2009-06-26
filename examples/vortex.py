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
"""Fire simulation using textured billboard quads"""

__version__ = '$Id$'

import os
import math
from pyglet import image
from pyglet.gl import *

from lepton import Particle, ParticleGroup, default_system
from lepton.renderer import PointRenderer
from lepton.texturizer import SpriteTexturizer
from lepton.emitter import StaticEmitter, PerParticleEmitter
from lepton.domain import AABox, Cylinder, Disc, Cone
from lepton.controller import Gravity, Lifetime, Movement, Fader, Magnet, Drag, Collector

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
glBlendFunc(GL_SRC_ALPHA, GL_ONE)
glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST)
glDisable(GL_DEPTH_TEST)

texture = image.load(os.path.join(os.path.dirname(__file__), 'flare3.png')).get_texture()
texturizer = SpriteTexturizer(texture.id)

source = Disc((0, -30, 0), (0, 1, 0), 2, 2)

dust_emitter = StaticEmitter(
	rate=40,
	template=Particle(
		velocity=(0,0,0), 
		mass=1.0,
		color=(1,1,1,0.25),
	),
	position=source,
	deviation=Particle(
		velocity=(20,0,20), 
		color=(0.0, 1.0, 1.0),
		age=0.5)
)
vortex = Cone((0, -30, 0), (0, 28, 0), 16, 0)
front = AABox((-100, -50, -50), (100, 25, 0))
back = AABox((-100, -50, 50), (100, 25, 0))

dust = ParticleGroup(
	controllers=[
		dust_emitter,
		Lifetime(8),
		Gravity((0,-20,0)), 
		Drag(0.0, 0.10, fluid_velocity=(80, 0, 0), domain=front),
		Drag(0.0, 0.10, fluid_velocity=(-80, 0, 0), domain=back),
		Magnet(charge=500, domain=vortex, exponent=0.75, epsilon=0.5),
		Movement(),
		],
	renderer=PointRenderer(16, texturizer),
	)


trail_emitter = PerParticleEmitter(dust, rate=30,
	template=Particle(
		color=(1,1,1),
		),
	)

trail = ParticleGroup(
	controllers=[trail_emitter, 
		Lifetime(.5), 
		Fader(max_alpha=0.09, fade_out_start=0, fade_out_end=0.5)],
	renderer=PointRenderer(14, texturizer),
	)

win.set_visible(True)
win.on_resize(win.width, win.height)
pyglet.clock.schedule_interval(default_system.update, (1.0/60.0))

time = 0
def move_vortex(dt):
	global time, vortex
	time += dt
	if time > 10:
		x, y, z = vortex.apex
		offset = math.sin((time - 10) * 0.3)
		source.center = vortex.apex = (offset * 30, y, z)
		vortex.base = (offset * 10, vortex.base.y, vortex.base.z)
pyglet.clock.schedule_interval(move_vortex, (1.0/10.0))

@win.event
def on_draw():
	win.clear()
	glLoadIdentity()
	glTranslatef(0, 0, -50)
	default_system.draw()

if __name__ == '__main__':
	pyglet.app.run()
