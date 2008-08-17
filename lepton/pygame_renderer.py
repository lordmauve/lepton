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
"""Pygame particle renderers.

(Obviously) requires pygame
"""

__version__ = '$Id$'

from pygame.transform import rotozoom
from math import degrees

class Renderer(object):
	"""Renderer base class"""

	group = None

	def set_group(self, group):
		"""Set the renderer's group"""
		if self.group is not None and self.group is not group:
			self.group.set_renderer(None)
		self.group = group
		# Subclasses may wish to extend this to reset state
	
	def draw(self):
		"""Render the group's particles"""
		raise NotImplementedError


class FillRenderer(Renderer):
	"""Renders particles to a pygame surface using simple fills"""

	def __init__(self, surface, flags=None):
		"""
		surface -- pygame surface to render particles to
		flags -- Special fill flags (pygame 1.8+ required)
		"""
		self.surface = surface
		self.flags = flags
	
	def draw(self):
		fill = self.surface.fill
		if self.flags is None:
			for p in self.group:
				fill(p.color.clamp(0, 255), 
					(p.position.x, p.position.y, p.size.x, p.size.y))
		else:
			flags = self.flags
			for p in self.group:
				fill(p.color.clamp(0, 255), 
					(p.position.x, p.position.y, p.size.x, p.size.y), flags)


class BlitRenderer(Renderer):
	"""Renders particles by blitting to a pygame surface"""

	def __init__(self, surface, particle_surface, rotate_and_scale=False):
		"""
		surface -- pygame surface to render particles to
		particle_surface -- surface blit to draw each particle.
		rotate_and_scale -- If true, the particles surfaces are rotated and scaled
		before blitting.
		"""
		self.surface = surface
		self.particle_surface = particle_surface
		self.rotate_and_scale = rotate_and_scale
	
	def draw(self):
		blit = self.surface.blit
		psurface = self.particle_surface
		if not self.rotate_and_scale:
			for p in self.group:
				blit(psurface, (p.position.x, p.position.y))
		else:
			last_rot = 0
			last_size = psurface.get_width()
			last_psurface = psurface
			for p in self.group:
				if p.rotation.x != last_rot or p.size.x != last_size:
					last_rot = p.rotation.x
					last_size = p.size.x
					scale = p.size.x / psurface.get_width()
					last_psurface = rotozoom(psurface, last_rot, scale)
				blit(last_psurface, (p.position.x, p.position.y))
				
