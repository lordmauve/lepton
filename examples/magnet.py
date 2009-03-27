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
""" Magnet.py

	Demos the magnet controller. Electrons orbit protons.
"""

__version__ = '$Id: magnet.py 104 2008-11-08 06:49:41Z andrew.charles $'

from pyglet import image
from pyglet.gl import *

import os, math

from lepton import Particle, ParticleGroup, default_system
from lepton.renderer import BillboardRenderer
from lepton.emitter import StaticEmitter, PerParticleEmitter
from lepton.controller import Movement, Magnet, Collector, Lifetime, Fader
from lepton.domain import Sphere, Point, Disc
from random import expovariate

if __name__ == '__main__':
	win = pyglet.window.Window(resizable=True, visible=False)
	win.clear()
	glEnable(GL_BLEND)
	glEnable(GL_POINT_SMOOTH)
	glShadeModel(GL_SMOOTH)
	glBlendFunc(GL_SRC_ALPHA,GL_ONE)
	glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	glDisable(GL_DEPTH_TEST)

	def resize(widthWindow, heightWindow):
		"""Setup 3D projection for window"""
		glViewport(0, 0, widthWindow, heightWindow)
		glMatrixMode(GL_PROJECTION)
		glLoadIdentity()
		gluPerspective(70, 1.0*widthWindow/heightWindow, 0.001, 10000.0)
		glMatrixMode(GL_MODELVIEW)
		glLoadIdentity()
	win.on_resize = resize

	electron_lifetime = 22 
	max_electrons = 6
	trail_lifetime = 4.5 

	nucleus = Sphere((0, 0, 0), 5)
	protons = ParticleGroup(renderer=BillboardRenderer(),
		controllers=[
			Movement(),
		]
	)
	proton_emitter = StaticEmitter(
		template=Particle(
			size=(30, 30, 0),
			color=(0.5, 1.0, 0.2, 0.5),
		),
		size=[(26, 26, 0), (30, 30, 0), (34, 34, 0)],
		deviation=Particle(
			rotation=(0, 0, math.pi / 6),
		))
			
	proton_emitter.emit(3, protons)

	electrons = ParticleGroup(renderer=BillboardRenderer(),
		controllers=[
			Movement(min_velocity=10),
			Lifetime(electron_lifetime * 1.5),
			Magnet(nucleus, charge=15000.0),
			Magnet(nucleus, charge=-15000.0, exponent=3),
			Fader(fade_in_end=1,
				fade_out_start=electron_lifetime * 1.4,
				fade_out_end=electron_lifetime * 1.5),
		]
	)
	   
	electron_emitter = StaticEmitter(
		template=Particle(
			position=(-20, 0, 0),
			size=(25, 25, 25),
			color=(0.1, 0.1, 1.0),
		),
		velocity=Disc((0,0,0), (-1,0,0), 36, 36), 
	)

	# Trails for electrons 
	trail_emitter = PerParticleEmitter(electrons, rate=80,
		template=Particle(
			color=(1, 0, 0 ,1),
			size=(4.25, 4.25, 0)
		),
		deviation=Particle(
			up=(0, 0, math.pi),
			rotation=(0, 0, math.pi),
			size=(0.5, 0.5, 0),
			velocity=(1, 1, 1),
			color=(0, 1, 0),
			age=trail_lifetime / 2.0),)

	trails = ParticleGroup(
		controllers=[
			Lifetime(trail_lifetime * 1.5),
			Movement(damping=0.7, max_velocity=60),
			Magnet(nucleus, charge=17000.0),
			Magnet(nucleus, charge=-17000.0, exponent=2.5),
			Collector(Sphere((0, 0, 0), 1)),
			Fader(fade_in_end=0.75, max_alpha=0.3, fade_out_start=0, fade_out_end=trail_lifetime),
			trail_emitter
		],
		renderer=BillboardRenderer())

	texture = image.load(os.path.join(os.path.dirname(__file__),'flare3.png')).texture
	glEnable(texture.target)
	glTexParameteri(texture.target, GL_TEXTURE_WRAP_S, GL_CLAMP)
	glTexParameteri(texture.target, GL_TEXTURE_WRAP_T, GL_CLAMP)
	glTexParameteri(texture.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
	glTexParameteri(texture.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
	glBindTexture(texture.target,texture.id)
			
	win.set_visible(True)
	pyglet.clock.schedule_interval(default_system.update, (1.0/30.0))

	yrot = 0.0
	xrot = 0.0

	@win.event
	def on_mouse_motion(x, y, dx, dy):
		global yrot, xrot
		yrot += dx * 0.3
		xrot -= dy * 0.3

	def summon(dt=None):
		if len(electrons) < max_electrons:
			electron_emitter.emit(1 ,electrons)
		pyglet.clock.schedule_once(summon, expovariate(1.0)+1.0)

	summon()

	@win.event
	def on_draw():
		global i
		global yrot,xrot
		win.clear()
		glLoadIdentity()
		glTranslatef(0, 0, -50)
		glRotatef(yrot, 0.0, 1.0, 0.0)
		glRotatef(xrot, 1.0, 0.0, 0.0)
		default_system.draw()

	pyglet.app.run()

