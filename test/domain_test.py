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

# $Id$

import unittest
import sys
import math


class DomainTest(unittest.TestCase):

	def assertVector(self, (x1, y1, z1), (x2,y2,z2)):
		tolerance = 0.00001
		self.failUnless(abs(x1 - x2) <= tolerance, ((x1, y1, z1), (x2,y2,z2)))
		self.failUnless(abs(y1 - y2) <= tolerance, ((x1, y1, z1), (x2,y2,z2)))
		self.failUnless(abs(z2 - z2) <= tolerance, ((x1, y1, z1), (x2,y2,z2)))


	def test_Line(self):
		from lepton.domain import Line
		line = Line((0, 1.0, 2.0), (1.0, 2.0, 3.0))
		self.assertEqual(tuple(line.start_point), (0, 1.0, 2.0))
		self.assertEqual(tuple(line.end_point), (1.0, 2.0, 3.0))
		for i in range(10):
			x,y,z = line.generate()
			self.failUnless(0 <= x <= 1.0 and 1.0 <= y <= 2.0 and 2.0 <= z <= 3.0, (x, y, z))
			self.assertAlmostEqual(x, y - 1.0, 3)
			self.assertAlmostEqual(x, z - 2.0, 3)
			self.failIf((x, y, x) in line)
		self.assertEqual(line.intersect((1.0, 1.0, 2.0), (0, 2.0, 3.0)), (None, None))
	
	def test_horizontal_Plane(self):
		from lepton.domain import Plane

		for Ny in (1, -1, 50, -50):
			# Simple horizontal plane
			plane = Plane((0,0,0), (0,Ny,0))
			self.assertEqual(plane.generate(), (0,0,0))
			self.failIf((0,0,0) in plane)
			
			# Perpendicular intersection
			p, N = plane.intersect((1, 1, 4), (1, -1, 4))
			self.assertVector(p, (1, 0, 4))
			self.assertVector(N, (0, 1, 0))
			p, N = plane.intersect((-5, -2, 20), (-5, 5, 20))
			self.assertVector(p, (-5, 0, 20))
			self.assertVector(N, (0, -1, 0))

			# Oblique intersection
			plane = Plane((20,5,3000), (0,Ny,0))
			p, N = plane.intersect((0, 7, 0), (4, 3, -4))
			self.assertVector(p, (2, 5, -2))
			self.assertVector(N, (0, 1, -0))

			# No intersection
			self.assertEqual(
				plane.intersect((40, 8, 20), (40, 6, -4)), (None, None))
			self.assertEqual(
				plane.intersect((0, 0, 0), (3, 4, 10)), (None, None))

			# Line in plane
			self.assertEqual(
				plane.intersect((-10, 5, 0), (3, 5, 10)), (None, None))
	
	def test_oblique_Plane(self):
		from lepton.domain import Plane
		from lepton.particle_struct import Vec3
		normal = Vec3(3,4,5).normalize()
		plane = Plane((0,0,0), normal)
		self.assertEqual(tuple(plane.point), (0, 0, 0))
		self.assertEqual(tuple(plane.normal), tuple(normal))
		p, N = plane.intersect((0,1,0), (0,-1,0))
		self.assertVector(p, (0,0,0))
		self.assertVector(N, normal)
		p, N = plane.intersect((0,-1,0), (0,1,0))
		self.assertVector(p, (0,0,0))
		self.assertVector(N, -normal)
	
	def test_plane_zero_length_normal(self):
		from lepton.domain import Plane
		self.assertRaises(ValueError, Plane, (0,0,0), (0,0,0))
	
	def test_Box_generate_contains(self):
		from lepton.domain import Box
		box = Box((-3, -1, 0), (-2, 1, 3))
		for i in range(20):
			x, y, z = box.generate()
			self.failUnless(-3 <= x <= -2, x)
			self.failUnless(-1 <= y <= 1, y)
			self.failUnless(0 <= z <= 3, z)
			self.failUnless((x, y, z) in box, (x, y, z))
		self.failUnless((-3, -1, 0) in box)
		self.failUnless((-3, 1, 0) in box)
		self.failUnless((-2, 1, 3) in box)
		self.failUnless((-2, -1, 0) in box)
		self.failUnless((-2.5, 0, 0) in box)
		self.failIf((-3,-3,-3) in box)
		self.failIf((-3,2,3) in box)
	
	def test_Box_intersect(self):
		from lepton.domain import Box
		from lepton.particle_struct import Vec3
		box = Box((-3, -1, 0), (-2, 1, 3))
		lines = [
			((-4, 0, 1), (-2, 0, 1)),
			((-2.5, -2, 2), (-2.5, -0.5, 2)),
			((-2.8, 0.5, -1), (-2.8, 0.5, 1)),
			((-1, 0, 1), (-2, 0, 1)),
			((-2.5, 2, 2), (-2.5, 1, 2)),
			((-2.8, 0.5, 4), (-2.8, 0.5, 1)),
		]
		expected = [
			((-3, 0, 1), (-1, 0, 0)),
			((-2.5, -1, 2), (0, -1, 0)),
			((-2.8, 0.5, 0), (0, 0, -1)),
			((-2, 0, 1), (1, 0, 0)),
			((-2.5, 1, 2), (0, 1, 0)),
			((-2.8, 0.5, 3), (0, 0, 1)),
		]
		for (start, end), (point, normal) in zip(lines, expected):
			p, N = box.intersect(start, end)
			self.failUnless(start not in box)
			self.failUnless(end in box)
			self.assertVector(p, point)
			self.assertVector(N, normal)

			# Reverse direction should yield same point and inverse normal
			p, N = box.intersect(end, start)
			self.assertVector(p, point)
			self.assertVector(N, -Vec3(*normal))
	
	def test_Box_grazing_intersect(self):
		from lepton.domain import Box
		box = Box((-3, -1, 0), (-2, 1, 3))
		p, N = box.intersect((-4, 0, 1), (0, 0, 1))
		self.assertEqual(p, (-3, 0, 1))
		self.assertEqual(N, (-1, 0, 0))
		p, N = box.intersect((0, 0, 1), (-4, 0, 1))
		self.assertEqual(p, (-2, 0, 1))
		self.assertEqual(N, (1, 0, 0))

	def test_Box_no_intersect(self):
		from lepton.domain import Box
		box = Box((-3, -1, 0), (-2, 1, 3))
	
		# No intersection
		self.assertEqual(
			box.intersect((-4, 2, 1), (-2, 2, 1)), (None, None))
		self.assertEqual(
			box.intersect((-2, 0, 1), (-2.8, 0.5, 1)), (None, None))

	def test_Box_line_in_sides(self):
		from lepton.domain import Box
		box = Box((-3, -1, 0), (-2, 1, 3))

		# Lines completely in sides
		lines = [
			((-3, 0, 1), (-3, 0.5, 2)),
			((-2.5, -1, 2), (-2, -1, 2)),
			((-2.8, 0.5, 0), (-2.5, 0.5, 0)),
			((-2, 0, 1), (-2, 0.5, 2)),
			((-2.5, 1, 2), (-2.5, 1, 1)),
			((-2.8, 0.5, 3), (-2.9, 0.5, 3)),
		]
		for start, end in lines:
			self.assertEqual(
				box.intersect(start, end), (None, None))

	
	def test_Sphere_generate_contains(self):
		from lepton.domain import Sphere
		sphere = Sphere((0, 1, 2), 2)
		for i in range(20):
			x, y, z = sphere.generate()
			mag = x**2 + (y - 1)**2 + (z - 2)**2
			self.failUnless(mag <= 4, mag)
			self.failUnless((x, y, z) in sphere, (x, y ,z))

		self.failUnless((0, 1, 2) in sphere)
		self.failUnless((1, 2, 3) in sphere)
		self.failUnless((2, 1, 2) in sphere)
		self.failUnless((-2, 1, 2) in sphere)
		self.failIf((2.1, 1, 2) in sphere)
		self.failIf((-2.1, 1, 2) in sphere)
		
	def test_Sphere_intersect(self):
		from lepton.domain import Sphere
		from lepton.particle_struct import Vec3
		sphere = Sphere((0, 1, 2), 2)
		lines = [
			((0, -2, 2), (0, 0, 2)),
			((3, 4, 2), (1, 2, 2)),
			((-1, -1, 0), (0, -1, 2)), # tangential
		]
		expected = [
			((0, -1, 2), (0, -1, 0)),
			((math.sin(math.pi/4)*2, math.sin(math.pi/4)*2+1, math.sin(math.pi/4)*2+2), 
				Vec3(1, 1, 0).normalize()),
			((0, -1, 2), (0, -1, 0)),
		]

		for (start, end), (point, normal) in zip(lines, expected):
			p, N = sphere.intersect(start, end)
			self.assertVector(p, point)
			self.assertVector(N, normal)

			# Reverse direction should yield same point and inverse normal
			p, N = sphere.intersect(end, start)
			self.assertVector(p, point)
			self.assertVector(N, -Vec3(*normal))

if __name__=='__main__':
	unittest.main()
