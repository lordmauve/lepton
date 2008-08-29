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


from random import random, uniform
from math import sqrt
from particle_struct import Vec3
from _domain import Line, Plane, AABox


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


class OldLine(Domain):
	"""1-Dimensional line segment domain"""

	def __init__(self, start_point, end_point):
		"""Define the line segment domain between the specified start and
		end points (as 3-tuples)
		"""
		self.start_x, self.start_y, self.start_z = [float(c) for c in start_point]
		end_x, end_y, end_z = [float(c) for c in end_point]
		self.vec_x = end_x - self.start_x
		self.vec_y = end_y - self.start_y
		self.vec_z = end_z - self.start_z
	
	def generate(self):
		"""Return a random point along the line segment domain"""
		r = random()
		return (self.start_x + self.vec_x * r,
			self.start_y + self.vec_y * r,
			self.start_z + self.vec_z * r)

	def __contains__(self, point):
		return False
	
	def intersect(self, start_point, end_point):
		"""You cannot intersect the line segment"""
		return (None, None)


class OldPlane(Domain):
	"""Infinite 2D plane domain"""

	def __init__(self, point, normal):
		"""
		point -- Any point in the plane
		normal -- Normal vector perpendicular to the plane. This need not
		be a unit vector.
		"""
		self.point = Vec3(*point)
		self.normal = Vec3(*normal).normalize()
		assert self.normal, ('%s: normal vector must have non-zero length' 
			% self.__class__.__name__)
		self.d = self.point.dot(self.normal)
	
	def generate(self):
		"""Aways return the provided point on the plane"""
		return tuple(self.point)

	def __contains__(self, point):
		return False
	
	def intersect(self, (sx, sy, sz), (ex, ey, ez)):
		"""Intersect the line segment with the plane return the intersection
		point and normal vector pointing into space on the same side of the
		plane as the start point.
		
		If the line does not intersect, or lies completely in the plane
		return (None, None)
		"""
		vx = ex - sx
		vy = ey - sy
		vz = ez - sz
		nx, ny, nz = self.normal
		num = self.d - nx*sx - ny*sy - nz*sz
		denom = nx*vx + ny*vy + nz*vz
		if denom:
			t = num / denom
			if 0.0 <= t <= 1.0:
				vx *= t
				vy *= t
				vz *= t
				# Calculate the intersection point
				px = sx + vx
				py = sy + vy
				pz = sz + vz
				# Calculate the distance from the plane to the start point
				dist = nx*vx + ny*vy + nz*vz
				if dist > 0:
					# start point is on opposite side of normal
					nx = -nx
					ny = -ny
					nz = -nz
				return (px, py, pz), (nx, ny, nz)
		return None, None
				

class OldBox(Domain):
	"""Axis aligned rectangular prism"""

	def __init__(self, point1, point2):
		"""point1 and point2 define any two opposite corners of the box"""
		p1 = Vec3(*point1)
		p2 = Vec3(*point2)
		self.point1 = Vec3()
		self.point2 = Vec3()
		self.point1.x = min(p1.x, p2.x)
		self.point1.y = min(p1.y, p2.y)
		self.point1.z = min(p1.z, p2.z)
		self.point2.x = max(p1.x, p2.x)
		self.point2.y = max(p1.y, p2.y)
		self.point2.z = max(p1.z, p2.z)
		self.size_x = self.point2.x - self.point1.x
		self.size_y = self.point2.y - self.point1.y
		self.size_z = self.point2.z - self.point1.z
	
	def generate(self):
		"""Return a random point inside the box"""
		x, y, z = self.point1
		return (x + self.size_x * random(),
			y + self.size_y * random(),
			z + self.size_z * random())

	def __contains__(self, (px, py, pz)):
		"""Return true if the point is within the box"""
		x1, y1, z1 = self.point1
		x2, y2, z2 = self.point2
		return (x1 <= px <= x2 and y1 <= py <= y2 and z1 <= pz <= z2)

	def intersect(self, start_point, end_point):
		"""Intersect the line segment with the box return the first 
		intersection point and normal vector pointing into space from
		the box side intersected.
		
		If the line does not intersect, or lies completely in one side
		of the box return (None, None)
		"""
		sx, sy, sz = start_point
		ex, ey, ez = end_point
		p1x, p1y, p1z = self.point1
		p2x, p2y, p2z = self.point2
		start_inside = start_point in self
		end_inside = end_point in self
		if start_inside != end_inside:
			if (end_inside and sy > p2y) or (start_inside and ey >= p2y) and (ey != sy):
				# Test for itersection with bottom face
				t = (sy - p2y) / (ey - sy)
				ix = (ex - sx) * t + sx
				iy = p2y
				iz = (ez - sz) * t + sz
				if p1x <= ix <= p2x and p1z <= iz <= p2z:
					return (ix, iy, iz), (0.0, (sy > p2y) * 2.0 - 1.0, 0.0)
			if (end_inside and sx < p1x) or (start_inside and ex <= p1x) and (ex != sx):
				# Test for itersection with left face
				t = (sx - p1x) / (ex - sx)
				ix = p1x
				iy = (ey - sy) * t + sy
				iz = (ez - sz) * t + sz
				if p1y <= iy <= p2y and p1z <= iz <= p2z:
					return (ix, iy, iz), ((sx > p1x) * 2.0 - 1.0, 0.0, 0.0)
			if (end_inside and sy < p1y) or (start_inside and ey <= p1y) and (ey != sy):
				# Test for itersection with top face
				t = (sy - p1y) / (ey - sy)
				ix = (ex - sx) * t + sx
				iy = p1y
				iz = (ez - sz) * t + sz
				if p1x <= ix <= p2x and p1z <= iz <= p2z:
					return (ix, iy, iz), (0.0, (sy > p1y) * 2.0 - 1.0, 0.0)
			if (end_inside and sx > p2x) or (start_inside and ex >= p2x) and (ex != sx):
				# Test for itersection with right face
				t = (sx - p2x) / (ex - sx)
				ix = p2x
				iy = (ey - sy) * t + sy
				iz = (ez - sz) * t + sz
				if p1y <= iy <= p2y and p1z <= iz <= p2z:
					return (ix, iy, iz), ((sx > p2x) * 2.0 - 1.0, 0.0, 0.0)
			if (end_inside and sz > p2z) or (start_inside and ez >= p2z) and (ez != sz):
				# Test for itersection with far face
				t = (sz - p2z) / (ez - sz)
				ix = (ex - sx) * t + sx
				iy = (ey - sy) * t + sy
				iz = p2z
				if p1y <= iy <= p2y and p1x <= ix <= p2x:
					return (ix, iy, iz), (0.0, 0.0, (sz > p2z) * 2.0 - 1.0)
			if (end_inside and sz < p1z) or (start_inside and ez <= p1z) and (ez != sz):
				# Test for itersection with near face
				t = (sz - p1z) / (ez - sz)
				ix = (ex - sx) * t + sx
				iy = (ey - sy) * t + sy
				iz = p1z
				if p1y <= iy <= p2y and p1x <= ix <= p2x:
					return (ix, iy, iz), (0.0, 0.0, (sz > p1z) * 2.0 - 1.0)
		return None, None


class Sphere(Domain):
	"""Spherical domain"""

	def __init__(self, center_point, radius):
		"""
		center_point -- Center point of sphere
		radius -- radius of sphere
		"""
		cp = self.center_point = Vec3(*center_point)
		self.radius = float(radius)
		self.radius_sq = self.radius**2
		self.cpm = cp.x**2 + cp.y**2 + cp.z**2
	
	def generate(self):
		"""Return a random point within the spherical domain"""
		# To avoid trig overhead, we generate a random point in the sphere's
		# bounding box and return it if it's inside the sphere. If it isn't
		# inside we move it closer to the center point until it is
		cx, cy, cz = self.center_point
		radius = self.radius
		px = uniform(-radius, radius)
		py = uniform(-radius, radius)
		pz = uniform(-radius, radius)
		while 1:
			if px**2 + py**2 + pz**2 <= self.radius_sq:
				return (px + cx, py + cy, pz + cz)
			else:
				px /= 2.0
				py /= 2.0
				pz /= 2.0
	
	def __contains__(self, (px, py, pz)):
		"""Return true if the point is inside the spherical domain"""
		cx, cy, cz = self.center_point
		return (px - cx)**2 + (py - cy)**2 + (pz - cz)**2 <= self.radius_sq
	
	def intersect(self, (sx, sy, sz), (ex, ey, ez), epsilon=0.0001):
		"""Intersect the line segment with the sphere and return the first 
		intersection point and normal vector pointing into space from
		the sphere intersection point.
		
		If the line does not intersect, return (None, None)
		"""
		# Sphere-line intersection algorithm described here
		# http://local.wasp.uwa.edu.au/~pbourke/geometry/sphereline/
		cx, cy, cz = self.center_point
		spm = (sx - cx)**2 + (sy - cy)**2 + (sz - cz)**2
		epm = (ex - cx)**2 + (ey - cy)**2 + (ez - cz)**2
		start_inside = spm <= self.radius_sq
		end_inside = epm <= self.radius_sq
		# XXX This will fail for high velocities/small spheres
		# We need to consider the closest point along the line seg
		# not just the end points
		if start_inside != end_inside: 
			a = (ex - sx)**2 + (ey - sy)**2 + (ez - sz)**2
			b = 2*((ex - sx)*(sx - cx) + (ey - sy)*(sy - cy) + (ez - sz)*(sz - cz))
			c = self.cpm + (sx**2 + sy**2 + sz**2) - 2*(cx*sx + cy*sy + cz*sz) - self.radius_sq
			bb4ac = b**2 - 4 * a * c
			if bb4ac < -epsilon:
				# No intersection
				return None, None
			elif abs(bb4ac) <= epsilon:
				# intersects at single point
				t = -b / (2*a)
			else:
				bb4ac = sqrt(bb4ac)
				t1 = (-b - bb4ac) / (2*a)
				t2 = (-b + bb4ac) / (2*a)
				if t1 < 0.0 or t1 > 1.0:
					# t1 not in segment
					t = t2
				elif t2 < 0.0 or t2 > 1.0:
					# t2 not in segment
					t = t1
				else:
					# both in segment, choose point closest to start
					t = min(t1, t2)
			px = sx + (ex - sx) * t
			py = sy + (ey - sy) * t
			pz = sz + (ez - sz) * t
			if start_inside:
				# Normal points inward
				N = Vec3(cx - px, cy - py, cz - pz).normalize()
			else:
				# Normal points outward
				N = Vec3(px - cx, py - cy, pz - cz).normalize()
			return (px, py, pz), (N.x, N.y, N.z)
		return None, None

