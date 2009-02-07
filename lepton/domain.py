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
"""Domains represent regions of space and are used for generating vectors
(positions, velocities, colors). Domains are also used by controllers to test
for collision.  Colliding with domains can then influence particle
behavior
"""

__version__ = '$Id$'


from random import random, uniform, choice
from math import sqrt, sin, cos, pi
from particle_struct import Vec3
from _domain import Line, Plane, AABox, Sphere


class Domain(object):
	"""Domain abstract base class"""

	def generate(self):
		"""Return a point within the domain as a 3-tuple. For domains with a
		non-zero volume, 'point in domain' is guaranteed to return true. 
		"""
		raise NotImplementedError
	
	def __contains__(self, point):
		"""Return true if point is inside the domain, false if not."""
		raise NotImplementedError
	
	def intersect(self, start_point, end_point):
		"""For the line segment defined by the start and end point specified
		(coordinate 3-tuples), return the point closest to the start point
		where the line segment intersects surface of the domain, and the
		surface normal unit vector at that point as a 2-tuple.  If the line
		segment does not intersect the domain, return the 2-tuple (None,
		None).

		Only 2 or 3 dimensional domains may be intersected.

		Note performance is more important than absolute accuracy with this
		method, so approximations are acceptable.
		"""
		raise NotImplementedError

EPSILON = 0.001

def Box(*args, **kw):
	"""Axis-aligned box domain (same as AABox for now)

	WARNING: Deprecated, use AABox instead. This domain will mean something different
	in future versions of lepton
	"""
	import warnings
	warnings.warn("lepton.domain.Box is deprecated, use AABox instead. "
		"This domain class will mean something different in future versions of lepton",
		stacklevel=2)
	return AABox(*args, **kw)


class Disc(object):

	def __init__(self, center, normal, outer_radius, inner_radius=0):
		assert outer_radius >= inner_radius
		self.center = Vec3(*center)
		self.normal = normal
		self.d = self.center.dot(self.normal)
		self.outer_radius = outer_radius
		self.inner_radius = inner_radius
		self.shell = abs(self.outer_radius - self.inner_radius) < EPSILON
		# create a rotation matrix based on the normal
		# The normal is used as the axis of rotation
		# And the angle is hardcoded at pi/2.0
	
	def _set_normal(self, normal):
		out = Vec3(*normal).normalize()
		world_up = Vec3(0, 0, 1)
		up = world_up - out * world_up.dot(out)
		if up.dot(up) < EPSILON:
			# Try another world up axis
			world_up = Vec3(0, 1, 0)
			up = world_up - out * world_up.dot(out)
			if up.dot(up) < EPSILON:
				# Try another world up axis
				world_up = Vec3(1, 0, 0)
				up = world_up - out * world_up.dot(out)
				if up.dot(up) < EPSILON:
					raise ValueError("Invalid normal vector")
		up = up.normalize()
		right = up.cross(out)
		self.rotmatrix = [
			right.x,
			right.y,
			right.z,
			up.x,
			up.y,
			up.z,
			out.x,
			out.y,
			out.z,
		]
		self._normal = out
	
	def _get_normal(self):
		return self._normal
	
	normal = property(_get_normal, _set_normal)

	def generate(self):
		if not self.shell:
			# find a candidate point
			outer_sq = self.outer_radius * self.outer_radius
			inner_sq = self.inner_radius * self.inner_radius
			minima = sqrt(inner_sq / 2.0)
			maxima = self.outer_radius
			while 1:
				tx = random()
				ty = random()
				if random() > 0.5:
					tx = tx * maxima * choice((-1, 1))
					ty = (minima + ty * (maxima - minima)) * choice((-1, 1))
				else:
					ty = ty * maxima * choice((-1, 1))
					tx = (minima + tx * (maxima - minima)) * choice((-1, 1))
				dist_sq = tx*tx + ty*ty
				if (dist_sq <= outer_sq and dist_sq >= inner_sq):
					break
		else:
			theta = uniform(0, pi * 2.0)
			tx = cos(theta) * self.outer_radius
			ty = sin(theta) * self.outer_radius
		# rotate it using the rotation matrix and offset using center
		m = self.rotmatrix
		cx, cy, cz = self.center
		x = m[0]*tx + m[3]*ty + cx
		y = m[1]*tx + m[4]*ty + cy
		z = m[2]*tx + m[5]*ty + cz

		return x, y, z

	def __contains__(self, point):
		point = Vec3(*point)
		from_center = self.center - point
		if abs(from_center.dot(self._normal)) < EPSILON:
			# Point is co-planar to disc
			if self.shell:
				return abs(from_center.length() - self.outer_radius) < EPSILON
			else:
				return self.inner_radius <= from_center.length() <= self.outer_radius
		return False

	def intersect(self, (sx, sy, sz), (ex, ey, ez)):
		"""Intersect the line segment with the disc return the intersection
		point and normal vector pointing into space on the same side of the
		disc's plane as the start point.
		
		If the line does not intersect, or lies completely in the disc
		return (None, None)
		"""
		vx = ex - sx
		vy = ey - sy
		vz = ez - sz
		nx, ny, nz = self.normal
		num = self.d - nx*sx - ny*sy - nz*sz
		denom = nx*vx + ny*vy + nz*vz
		if denom:
			outer_sq = self.outer_radius * self.outer_radius
			inner_sq = self.inner_radius * self.inner_radius
			t = num / denom
			if 0.0 <= t <= 1.0:
				vx *= t
				vy *= t
				vz *= t
				# Calculate the intersection point
				px = sx + vx
				py = sy + vy
				pz = sz + vz
				# Calc distance from center
				cx, cy, cz = self.center
				from_center = (cx - px)**2 + (cy - py)**2 + (cz - pz)**2
				if inner_sq <= from_center <= outer_sq:
					# Calculate the distance from the plane to the start point
					dist = nx*vx + ny*vy + nz*vz
					if dist > 0:
						# start point is on opposite side of normal
						nx = -nx
						ny = -ny
						nz = -nz
					return (px, py, pz), (nx, ny, nz)
		return None, None


