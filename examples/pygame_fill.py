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
"""Lepton particle engine example using pygame with the FillRenderer"""

__version__ = '$Id$'

import pygame
from pygame.locals import *

from lepton import Particle, ParticleGroup, default_system
from lepton.pygame_renderer import FillRenderer
from lepton.emitter import StaticEmitter
from lepton.controller import Gravity, Lifetime, Movement, Fader, ColorBlender

if __name__ == '__main__':
	pygame.init()
	width, height = 800, 600
	display = pygame.display.set_mode((width, height))
	pygame.display.set_caption('Lepton pygame FillRenderer example', 'Lepton')
	clock = pygame.time.Clock()

	spray = StaticEmitter(
		rate=150,
		template=Particle(
			position=(width/2, height-50, 0),
			velocity=(0, -150, 0)),
		deviation=Particle(
			velocity=(10, 5, 0),
			color=(20, 20, 0)),
		color=[(0,0,255), (0,0,150), (150, 150, 200), (100, 100, 150)],
		size=[(1,1,0), (2,2,0)],
	)
	
	water = ParticleGroup(controllers=[spray], renderer=FillRenderer(display))
	
	default_system.add_global_controller(
		Lifetime(10),
		Gravity((0,30,0)), 
		Movement(), 
		#Fader(fade_in_end=5.0, max_alpha=0.005, fade_out_start=17, fade_out_end=20),
	)

	while 1:
		for event in pygame.event.get():
			if event.type == QUIT or (event.type == KEYDOWN and event.key == K_ESCAPE):
				raise SystemExit
		ms = clock.tick()
		default_system.update(ms / 1000.0)
		display.fill((0, 0, 0))
		default_system.draw()
		pygame.display.flip()

