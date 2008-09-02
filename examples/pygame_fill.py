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
"""Lepton particle engine example using pygame with the FillRenderer

Note the usage of 3D domains in a 2D environment
"""

__version__ = '$Id$'

import pygame
from pygame.locals import *

from lepton import Particle, ParticleGroup, default_system, domain
from lepton.pygame_renderer import FillRenderer
from lepton.emitter import StaticEmitter
from lepton.controller import Gravity, Lifetime, Movement, Fader, ColorBlender, Bounce, Collector

if __name__ == '__main__':
	pygame.init()
	width, height = 800, 600
	display = pygame.display.set_mode((width, height))
	pygame.display.set_caption('Lepton pygame FillRenderer example', 'Lepton')
	clock = pygame.time.Clock()
	spout_radius = 10

	spray = StaticEmitter(
		rate=250,
		template=Particle(
			position=(width/2, 0, 0),
			velocity=(0, 0, 0)),
		deviation=Particle(
			velocity=(2, 15, 2),
			color=(10, 10, 0)),
		position=domain.AABox(
			(width/2 - spout_radius, 0, -spout_radius), 
			(width/2 + spout_radius, 0, spout_radius)),
		color=[(0,0,255), (0,0,150), (150, 150, 200), (100, 100, 150)],
		size=[(1,2,0), (2,2,0), (2,3,0)],
	)
	
	radius = width/3
	sphere = domain.Sphere((width/2, height, 0), radius)
	screen_box = domain.AABox((0,0,-width), (width,height,width))
	
	water = ParticleGroup(controllers=[spray], renderer=FillRenderer(display))
	
	default_system.add_global_controller(
		Gravity((0,300,0)), 
		Movement(), 
		Bounce(sphere, bounce=0.5, friction=0.02),
		Collector(screen_box, collect_inside=False),
	)
	sphere_rect = (width/2 - radius, height - radius, radius * 2, radius * 2)

	while 1:
		for event in pygame.event.get():
			if event.type == QUIT or (event.type == KEYDOWN and event.key == K_ESCAPE):
				raise SystemExit
		ms = clock.tick()
		default_system.update(ms / 1000.0)
		display.fill((0, 0, 0))
		pygame.draw.ellipse(display, (15,40,40), sphere_rect)
		default_system.draw()
		pygame.display.flip()

