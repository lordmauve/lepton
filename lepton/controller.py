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
"""Particle controllers

particle controllers are bound to particle groups and invoked
via the group's update() method. A controller is a callable
object that accepts a timedelta and group as its argument.

A typical controller will iterate all particles in a group,
apply some logic and mutate the particles or remove or add
particles to the group.
"""

__version__ = '$Id$'

from math import sqrt
from particle_struct import Color, Vec3
from _controller import Gravity, Movement, Lifetime, ColorBlender


def NoopController(time_delta, group):
	"""Do nothing controller"""


class Fader(object):
	"""Alters the alpha of particles to fade them in and out over time"""

	def __init__(self, start_alpha=0.0, fade_in_start=0.0, fade_in_end=0.0, max_alpha=1.0, 
		fade_out_start=None, fade_out_end=None, end_alpha=0.0):
		"""
		start_alpha -- Initial alpha value.
		fade_in_start -- Time to start fading in to max_alpha
		fade_in_end -- Time when alpha reaches max
		max_alpha -- Maximum alpha level
		fade_out_start -- Time to start fading out to end_alpha. If None
		no fade out is performed.
		fade_out_end -- Time when alpha reaches end
		end_alpha -- Ending alpha level
		"""
		assert fade_in_end >= fade_in_start, "Fader: expected fade_in_end >= fade_in_start"
		assert fade_out_start is None or (fade_out_end is not None and
			fade_out_end >= fade_out_start), "Fader: expected fade_out_end >= fade_out_start"
		self.start_alpha = start_alpha
		self.fade_in_start = fade_in_start
		self.fade_in_end = fade_in_end
		self.max_alpha = max_alpha
		self.fade_out_start = fade_out_start
		self.fade_out_end = fade_out_end
		self.end_alpha = end_alpha
	
	def __call__(self, td, group):
		in_start = self.fade_in_start
		in_end = self.fade_in_end
		in_time = in_end - in_start
		in_alpha = self.max_alpha - self.start_alpha
		out_start = self.fade_out_start
		out_end = self.fade_out_end
		out_time = out_end - out_start
		out_alpha = self.end_alpha - self.max_alpha

		for p in group:
			if p.age > in_end and (out_start is None or p.age <= out_start):
				p.color.a = self.max_alpha
			elif p.age > in_start and p.age < in_end:
				p.color.a = self.start_alpha + in_alpha * ((p.age - in_start) / in_time)
			elif out_start is not None and p.age > out_start and p.age < out_end:
				p.color.a = self.max_alpha + out_alpha * ((p.age - out_start) / out_time)
			elif out_end is not None and p.age >= out_end:
				p.color.a = self.end_alpha


class Collector(object):
	"""Controller that "collects" particles that enter or exit its domain

	Collected particles are killed and counted by the collector
	"""

	def __init__(self, domain, collect_inside=True, callback=None):
		"""
		domain -- particles inside this domain are killed. The domain must
		have a non-zero volume.

		collect_inside -- True to collect particles inside the domain, false
		to collect particles outside the domain.

		callback -- an optional function called after each particle is
		collected. Must have the signature: 
			callback(particle, group, collector)
		"""
		self.collected_count = 0
		self.collect_inside = collect_inside
		self.domain = domain
		self.callback = None
	
	def __call__(self, td, group):
		domain = self.domain
		collect_inside = self.collect_inside
		if self.callback is None:
			for p in group:
				if (p.position in domain) == collect_inside:
					group.kill(p)
					self.collected_count += 1
		else:
			callback = self.callback
			for p in group:
				if (p.position in domain) == collect_inside:
					group.kill(p)
					self.collected_count += 1
					callback(p, group, self)


class Bounce(object):
	"""Controller that bounces particles off the surface of a domain
	
	For best results this controller should be bound after other controllers
	that affect the position and velocity of particles.
	"""

	def __init__(self, domain, friction=0, callback=None):
		"""
		domain -- Particles that collide with the surface of this domain are
		redirected as if they bounced or reflected off the surface. This
		alters the position and velocity of the particle. The domain must have
		a non-zero area.

		friction -- The fraction of velocity lost after the bounce. If
		friction is negative, the particle will gain velocity. If friction is
		1, the particle will stop at the point of collision. If friction is
		greater than 1, the particle will appear to refract off of the surface
		rather than reflect.

		callback -- An optional function called when a particle collides
		with the domain. Must have the signature:
			callback(particle, group, controller, collision_point, collision_normal)
		collision_point is point on the domain where the collision occurred.
		collision_normal is the normal vector on the domain's surface at the
		point of collision.
		"""
		self.domain = domain
		self.friction = float(friction)
		self.callback = callback
	
	def __call__(self, td, group):
		domain = self.domain
		bounce = 1.0 - self.friction
		callback = self.callback
		for p in group:
			collide_point, normal = domain.intersect(p.last_position, p.position)
			if collide_point is not None:
				reflect_vec = (Vec3(*p.position) - collide_point).reflect(normal) * bounce
				p.velocity = Vec3(*p.velocity).reflect(normal) * bounce
				p.position = reflect_vec + collide_point
				if callback is not None:
					callback(p, group, self, collide_point, normal)


class Clumper(object):
	"""EXPERMENTAL: SUBJECT TO CHANGE
	
	Clumps objects in a group together or keeps them apart
	
	The center of the group is calculated by averaging all of the
	particle positions, and all particles are accelerated toward
	(or away) from this center point.
	"""

	def __init__(self, magnitude):
		"""
		magnitude -- The acceleration magnitude toward the
		group center. If negative, the acceleration is
		away from the center.
		"""
		self.magnitude = magnitude
	
	def __call__(self, td, group):
		if group:
			avgx = 0
			avgy = 0
			avgz = 0

			for p in group:
				px, py, pz = p.position
				avgx += px
				avgy += py
				avgz += pz

			avgx /= len(group)
			avgy /= len(group)
			avgz /= len(group)

			mag = self.magnitude * td

			for p in group:
				# calculate the position looking ahead one frame
				px, py, pz = p.position
				px += p.velocity.x
				py += p.velocity.y
				pz += p.velocity.z
				# calculate the acceleration vector
				dx = avgx - px
				dy = avgy - py
				dz = avgz - pz
				dmag = sqrt(dx**2 + dy**2 + dz**2)
				p.velocity.x += (dx / dmag) * mag
				p.velocity.y += (dy / dmag) * mag
				p.velocity.z += (dz / dmag) * mag
