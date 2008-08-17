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


class Cache:
	"""Simple, fast, bounded cache that gives approximate MRU behavior"""

	def __init__(self, max_size, load_factor=0.85):
		self.max_size = max_size
		self.max_recent_size = int(max_size * load_factor)
		self._recent = {} # Recently accessed bucket
		self._aged = {} # Less recently accessed bucket
		self.accesses = 0
		self.misses = 0
		self.adds = 0
		self.flips = 0
		self.purged = 0
	
	def __getitem__(self, key):
		#self.accesses += 1
		try:
			try:
				return self._recent[key]
			except KeyError:
				# Promote aged element to "recent"
				value = self._aged.pop(key)
				self._recent[key] = value
				return value
		except KeyError:
			#self.misses += 1
			raise
	
	def __len__(self):
		return len(self._recent) + len(self._aged)
	
	def __contains__(self, key):
		return key in self._recent or key in self._aged
	
	def __setitem__(self, key, value):
		assert value is not None
		#self.adds += 1
		if key in self._aged:
			del self._aged[key]
		if len(self._recent) >= self.max_recent_size:
			# Flip the cache discarding aged entries
			#self.flips += 1
			#print self.flips, 'cache flips in', self.adds, ' adds. ',
			#print self.misses, 'misses in', self.accesses, 'accesses (',
			#print (self.accesses - self.misses) * 100 / self.accesses, '% hit rate) ',
			#print 'with', self.purged, 'purged'
			self._aged = self._recent
			self._recent = {}
		self._recent[key] = value
		while self._aged and len(self) > self.max_size:
			# Over max size, purge aged entries
			#self.purged += 1
			self._aged.popitem()


class BlitRenderer(Renderer):
	"""Renders particles by blitting to a pygame surface"""

	surf_cache = Cache(200)

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
			cache = self.surf_cache
			surfid = id(psurface)
			for p in self.group:
				size = int(p.size.x)
				rot = int(p.rotation.x)
				cachekey = (surfid, size, rot)
				try:
					surface = cache[cachekey]
				except KeyError:
					scale = p.size.x / psurface.get_width()
					surface = cache[cachekey] = rotozoom(psurface, rot, scale)
				blit(surface, (p.position.x, p.position.y))

