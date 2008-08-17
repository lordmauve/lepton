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
"""Lepton particle engine example using pygame with the BlitRenderer"""

__version__ = '$Id$'

import os
import pygame
from pygame.locals import *

from lepton import Particle, ParticleGroup, default_system, domain
from lepton.pygame_renderer import BlitRenderer
from lepton.emitter import StaticEmitter
from lepton.controller import Gravity, Lifetime, Movement, Growth

if __name__ == '__main__':
	pygame.init()
	width, height = 800, 600
	display = pygame.display.set_mode((width, height))
	pygame.display.set_caption('Lepton pygame BlitRenderer example', 'Lepton')
	clock = pygame.time.Clock()

	bubbler = StaticEmitter(
		rate=80,
		template=Particle(position=(width/2, height-50, 0)),
		deviation=Particle(
			velocity=(5, 15, 0),
			size=(5, 5, 0)),
		position=[((width*i/4), height, 0) for i in range(5)]
	)

	bubble = pygame.image.load(os.path.join(os.path.dirname(__file__), 'bubble.png'))
	
	water = ParticleGroup(controllers=[bubbler], 
		renderer=BlitRenderer(display, bubble, rotate_and_scale=True))
	
	default_system.add_global_controller(
		Lifetime(7),
		Gravity((0,-30,0)), 
		Movement(), 
		Growth(4.5),
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

