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

	def assertVector(self, (x1, y1, z1), (x2,y2,z2), tolerance=0.0001):
		self.failUnless(abs(x1 - x2) <= tolerance, ((x1, y1, z1), (x2,y2,z2)))
		self.failUnless(abs(y1 - y2) <= tolerance, ((x1, y1, z1), (x2,y2,z2)))
		self.failUnless(abs(z1 - z2) <= tolerance, ((x1, y1, z1), (x2,y2,z2)))

	def test_point(self):
		from lepton.domain import Point
		point = Point((1,2,3))
		self.assertEqual(tuple(point.point), (1,2,3))
		self.assertEqual(tuple(point.generate()), (1,2,3))
		self.failUnless((1,2,3) in point)
		self.failIf((1.1,2,3) in point)
		self.failIf((0,0,0) in point)
		self.assertEqual(point.intersect((2,2,2), (3,3,3)), (None, None))
	
	def test_point_closest_point_to(self):
		from lepton.domain import Point
		from lepton.particle_struct import Vec3
		point = Point((4,5,6))
		p, N = point.closest_point_to((4,6,6))
		self.assertVector(p, (4,5,6))
		self.assertVector(N, (0,1,0))
		p, N = point.closest_point_to((0,5,6))
		self.assertVector(p, (4,5,6))
		self.assertVector(N, (-1,0,0))
		p, N = point.closest_point_to((3,4,5))
		self.assertVector(p, (4,5,6))
		self.assertVector(N, Vec3(-1,-1,-1).normalize())
		p, N = point.closest_point_to((4,5,6))
		self.assertVector(p, (4,5,6))
		self.assertVector(N, (0,0,0))

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
	
	def test_line_closest_pt_to(self):
		from lepton.domain import Line
		from lepton.particle_struct import Vec3
		line = Line((0, -1, 0), (0, 2, 0))
		for point, closest, normal in [
			((-1, 0, 0), (0, 0, 0), (-1, 0, 0)),
			((1, 1, 1), (0, 1, 0), Vec3(1, 0, 1).normalize()),
			((0, -2, 0), (0, -1, 0), (0, 0, 0)),
			]:
			p, N = line.closest_point_to(point)
			self.assertVector(p, point)
			self.assertVector(N, normal)
	
	def test_horizontal_Plane(self):
		from lepton.domain import Plane

		for Ny in (1, -1, 50, -50):
			# Simple horizontal plane
			plane = Plane((0,0,0), (0,Ny,0))
			self.assertEqual(plane.generate(), (0,0,0))
			self.failUnless((0,0,0) in plane)
			self.failIf((0, Ny, 0) in plane)
			self.failUnless((0, -Ny, 0) in plane)
			
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
		self.failIf(normal in plane)
		self.failUnless(plane.point in plane)
		self.failUnless(-normal in plane)
		p, N = plane.intersect((0,1,0), (0,-1,0))
		self.assertVector(p, (0,0,0))
		self.assertVector(N, normal)
		p, N = plane.intersect((0,-1,0), (0,1,0))
		self.assertVector(p, (0,0,0))
		self.assertVector(N, -normal)
	
	def test_plane_zero_length_normal(self):
		from lepton.domain import Plane
		self.assertRaises(ValueError, Plane, (0,0,0), (0,0,0))
	
	def test_AABox_generate_contains(self):
		from lepton.domain import AABox
		box = AABox((-3, -1, 0), (-2, 1, 3))
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
	
	def test_AABox_intersect(self):
		from lepton.domain import AABox
		from lepton.particle_struct import Vec3
		box = AABox((-3, -1, 0), (-2, 1, 3))
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
	
	def test_AABox_grazing_intersect(self):
		from lepton.domain import AABox
		box = AABox((-3, -1, 0), (-2, 1, 3))
		p, N = box.intersect((-4, 0, 1), (0, 0, 1))
		self.assertEqual(p, (-3, 0, 1))
		self.assertEqual(N, (-1, 0, 0))
		p, N = box.intersect((0, 0, 1), (-4, 0, 1))
		self.assertEqual(p, (-2, 0, 1))
		self.assertEqual(N, (1, 0, 0))

	def test_AABox_no_intersect(self):
		from lepton.domain import AABox
		box = AABox((-3, -1, 0), (-2, 1, 3))
	
		# No intersection
		self.assertEqual(
			box.intersect((-4, 2, 1), (-2, 2, 1)), (None, None))
		self.assertEqual(
			box.intersect((-2, 0, 1), (-2.8, 0.5, 1)), (None, None))

	def test_AABox_line_in_sides(self):
		from lepton.domain import AABox
		box = AABox((-3, -1, 0), (-2, 1, 3))

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

	def test_solid_Sphere_generate_contains(self):
		from lepton.domain import Sphere
		sphere = Sphere((0, 1, 2), 2)
		for i in range(20):
			x, y, z = sphere.generate()
			mag = x**2 + (y - 1)**2 + (z - 2)**2
			self.failUnless(mag <= 4, ((x, y, z), mag))
			self.failUnless((x, y, z) in sphere, (x, y ,z))

		self.failUnless((0, 1, 2) in sphere)
		self.failUnless((1, 2, 3) in sphere)
		self.failUnless((2, 1, 2) in sphere)
		self.failUnless((-2, 1, 2) in sphere)
		self.failIf((2.1, 1, 2) in sphere)
		self.failIf((-2.1, 1, 2) in sphere)

	def test_shell_Sphere_generate_contains(self):
		from lepton.domain import Sphere
		sphere = Sphere((1, -2, 4), 3, 2)
		for i in range(20):
			x, y, z = sphere.generate()
			mag = (x - 1)**2 + (y + 2)**2 + (z - 4)**2
			self.failUnless(4 <= mag <= 9, ((x, y, z), mag))
			self.failUnless((x, y, z) in sphere, (x, y ,z))

		self.failUnless((3, -2, 4) in sphere)
		self.failUnless((1, 1, 4) in sphere)
		self.failUnless((1, -2, 1.5) in sphere)
		self.failIf((1, -2, 4) in sphere)
		self.failIf((5, 1, 7) in sphere)
		
	def test_solid_Sphere_intersect(self):
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
			((math.sin(math.pi/4)*2, math.sin(math.pi/4)*2+1, 2), 
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
	
	def test_Sphere_stationary_particles(self):
		from lepton.domain import Sphere
		from lepton.particle_struct import Vec3
		sphere = Sphere((0, 1, 2), 2)
		positions = [
			((0, 1.1, 2.2), (0, 1.1, 2.2)),
			((3.1, 4, 2), (3.1, 4, 2)),
			((-0.9, -1, 0), (-0.9, -1, 0)),
		]
		for start, end in positions:
			self.assertEqual(sphere.intersect(start, end), (None, None))
	
	def test_solid_Sphere_grazing_intersect(self):
		from lepton.domain import Sphere
		sphere = Sphere((0, 0, 0), 4)
		p, N = sphere.intersect((-5, 0, 0), (5, 0, 0))
		self.assertVector(p, (-4, 0, 0))
		self.assertVector(N, (-1, 0, 0))
		p, N = sphere.intersect((5, 0, 0), (-5, 0, 0))
		self.assertVector(p, (4, 0, 0))
		self.assertVector(N, (1, 0, 0))
	
	def test_shell_Sphere_intersect(self):
		from lepton.domain import Sphere
		from lepton.particle_struct import Vec3
		sphere = Sphere((2, 1, -1), 3, 1)
		lines = [
			((2, 1, -1), (2, 1, 1)),
			((2.5, 1, -1), (4, 1, -1)),
			((5, 4, -1), (3.5, 2.5, -1)),
			((2, 2.5, -1), (2, 1, -1)),
		]
		expected = [
			((2, 1, 0), (0, 0, -1)),
			((3, 1, -1), (-1, 0, 0)),
			((math.sin(math.pi/4)*3+2, math.sin(math.pi/4)*3+1, -1),
				Vec3(1, 1, 0).normalize()),
			((2, 2, -1), (0, 1, 0)),
		]

		for (start, end), (point, normal) in zip(lines, expected):
			p, N = sphere.intersect(start, end)
			self.assertVector(p, point)
			self.assertVector(N, normal)

			# Reverse direction should yield same point and inverse normal
			p, N = sphere.intersect(end, start)
			self.assertVector(p, point)
			self.assertVector(N, -Vec3(*normal))
		
	def test_shell_Sphere_grazing_intersect(self):
		from lepton.domain import Sphere
		sphere = Sphere((0, 0, 0), 4, 3)
		p, N = sphere.intersect((-5, 0, 0), (5, 0, 0))
		self.assertVector(p, (-4, 0, 0))
		self.assertVector(N, (-1, 0, 0))
		p, N = sphere.intersect((5, 0, 0), (-5, 0, 0))
		self.assertVector(p, (4, 0, 0))
		self.assertVector(N, (1, 0, 0))
		p, N = sphere.intersect((-5, 0, 0), (0, 0, 0))
		self.assertVector(p, (-4, 0, 0))
		self.assertVector(N, (-1, 0, 0))
		p, N = sphere.intersect((0, 0, 0), (-5, 0, 0))
		self.assertVector(p, (-3, 0, 0))
		self.assertVector(N, (1, 0, 0))
	
	def test_Sphere_closest_point_to(self):
		from lepton.domain import Sphere
		from lepton.particle_struct import Vec3
		sphere = Sphere((0,5,-1), 4.0, 2.0)
		for point, closest, normal in [
			((0,5,-1), (0,5,-1), (0,0,0)),
			((5,5,-1), (4,5,-1), (1,0,0)),
			((0,15,-1), (0,9,-1), (0,1,0)),
			((6,11,5), Vec3(0,5,-1) + Vec3(1,1,1).normalize()*4, Vec3(1,1,1).normalize()),
			((1,5,-1), (2,5,-1), (-1,0,0)),
			((0,8,-1), (0,8,-1), (0,0,0)),
			]:
			p, N = sphere.closest_point_to(point)
			self.assertVector(p, closest)
			self.assertVector(N, normal)
	
	def test_solid_disc_generate_contains(self):
		from lepton.domain import Disc
		disc = Disc((0, 1, 2), (0, 0, 1), 2)
		for i in range(20):
			x, y, z = disc.generate()
			mag = x**2 + (y - 1)**2 + (z - 2)**2
			self.failUnless(mag <= 4, ((x, y, z), mag))
			self.failUnless((x, y, z) in disc, (x, y ,z))

		self.failUnless((0, 1, 2) in disc)
		self.failUnless((1, 2, 2) in disc)
		self.failUnless((2, 1, 2) in disc)
		self.failUnless((-2, 1, 2) in disc)
		self.failIf((2.1, 1, 2) in disc)
		self.failIf((-2.1, 1, 2) in disc)
		self.failIf((0, 1, 2.1) in disc)

	def test_hollow_disc_generate_contains(self):
		from lepton.domain import Disc
		disc = Disc((0, 1, 2), (0, 1, 0), 2, 1)
		self.assertEqual(tuple(disc.center), (0, 1, 2))
		self.assertEqual(tuple(disc.normal), (0, 1, 0))
		self.assertEqual(disc.inner_radius, 1)
		self.assertEqual(disc.outer_radius, 2)
		for i in range(20):
			x, y, z = disc.generate()
			mag = x**2 + (y - 1)**2 + (z - 2)**2
			self.failUnless(1 <= mag <= 4, ((x, y, z), mag))
			self.failUnless((x, y, z) in disc, (x, y ,z))

		self.failIf((0, 1, 2) in disc)
		self.failIf((0, 0, 2) in disc)
		self.failUnless((1, 1, 3) in disc)
		self.failUnless((2, 1, 2) in disc)
		self.failUnless((-2, 1, 2) in disc)
		self.failIf((2.1, 1, 2) in disc)
		self.failIf((-2.1, 1, 2) in disc)
		self.failIf((0, 1.2, 2) in disc)
	
	def test_shell_disc_generate_contains(self):
		from lepton.domain import Disc
		disc = Disc((-2, 0, 1), (1, 1, 0), 2, 2)
		for i in range(20):
			x, y, z = disc.generate()
			mag = (x + 2)**2 + y**2 + (z - 1)**2
			self.assertAlmostEqual(mag, 4.0, 4)
			self.failUnless((x, y, z) in disc, (x, y ,z))
	
	def test_solid_disc_intersect(self):
		from lepton.domain import Disc
		from lepton.particle_struct import Vec3
		normal = Vec3(1, 1, 0).normalize()
		disc = Disc((-2, 0, 1), normal, 1)
		lines = [
			((-5, 0, 0), (5, 0, 0)),
			((-2, 1, 1), (-2, -1, 1)),
			((3, normal.y, 1), (-3, normal.y, 1)),
		]
		expected = [
			((-2, 0, 0), -normal),
			((-2, 0, 1), normal),
			((-2 - normal.x, normal.y, 1), normal),
		]

		for (start, end), (point, normal) in zip(lines, expected):
			p, N = disc.intersect(start, end)
			self.assertVector(p, point)
			self.assertVector(N, normal)

			# Reverse direction should yield same point and inverse normal
			p, N = disc.intersect(end, start)
			self.assertVector(p, point)
			self.assertVector(N, -Vec3(*normal))

	def test_solid_disc_no_intersect(self):
		from lepton.domain import Disc
		disc = Disc((0, 0, 0), (1, 0, 0), 1)
		for start, end in [
			((1, 0, 0), (2, 0, 0)),
			((0, 0, 0), (0, 1, 0)),
			((-2, 10, 20), (-5, 15, 19))]:
			self.assertEqual(
				disc.intersect(start, end), (None, None))
			self.assertEqual(
				disc.intersect(end, start), (None, None))

	def test_hollow_disc_intersect(self):
		from lepton.domain import Disc
		from lepton.particle_struct import Vec3
		disc = Disc((2,2,2), (0,1,0), 3, 1)
		lines = [
			((-1, 3, 2), (-1, 0, 2)),
			((1, 3, 0), (1, 0, 0)),
			((2, 5, 4), (2, -5, 4)),
			((3, 1, 2), (5, 3, 2)),
		]
		expected = [
			((-1, 2, 2), (0,1,0)),
			((1, 2, 0), (0,1,0)),
			((2, 2, 4), (0,1,0)),
			((4, 2, 2), (0,-1,0)),
		]

		for (start, end), (point, normal) in zip(lines, expected):
			p, N = disc.intersect(start, end)
			self.assertVector(p, point)
			self.assertVector(N, normal)

			# Reverse direction should yield same point and inverse normal
			p, N = disc.intersect(end, start)
			self.assertVector(p, point)
			self.assertVector(N, -Vec3(*normal))

	def test_hollow_disc_no_intersect(self):
		from lepton.domain import Disc
		disc = Disc((2,2,2), (0,1,0), 3, 1)
		for start, end in [
			((2, 3, 2.5), (2, 0, 1.5)),
			((2, 2, 2), (5, 2, 2)),
			((-2, 10, 20), (-5, 15, 19))]:
			self.assertEqual(
				disc.intersect(start, end), (None, None))
			self.assertEqual(
				disc.intersect(end, start), (None, None))
	
	def test_cylinder_length(self):
		from lepton.domain import Cylinder
		cyl = Cylinder((0,0,0), (2,0,0), 1)
		self.assertEqual(cyl.length, 2)
		self.assertVector(cyl.end_point0, (0,0,0))
		self.assertVector(cyl.end_point1, (2,0,0))
		cyl = Cylinder((1,0,-1), (3,2,1), 1)
		self.assertAlmostEqual(cyl.length, math.sqrt(12), 5)
		self.assertVector(cyl.end_point0, (1,0,-1))
		self.assertVector(cyl.end_point1, (3,2,1))

		# Changing end points shoud adjust length
		cyl.end_point0 = (3, 0, -1)
		self.assertAlmostEqual(cyl.length, math.sqrt(8), 5)
		cyl.end_point1 = (5, 0, -1)
		self.assertAlmostEqual(cyl.length, 2, 5)

	def test_solid_cyl_generate_contains(self):
		from lepton.domain import Cylinder
		cyl = Cylinder((0, 1, 2), (0, 2, 2), 2)
		for i in range(20):
			x, y, z = cyl.generate()
			mag = x**2 + (z - 2)**2
			self.failUnless(mag <= 4, ((x, y), mag))
			self.failUnless(1 <= y <= 2, y)
			self.failUnless((x, y, z) in cyl, (x, y ,z))

		self.failUnless((0, 1, 2) in cyl)
		self.failUnless((0, 2, 2) in cyl)
		self.failUnless((2, 1.5, 2) in cyl)
		self.failUnless((-2, 1.5, 2) in cyl)
		self.failIf((2.1, 1, 2) in cyl)
		self.failIf((-2.1, 1, 2) in cyl)
		self.failIf((0, 0, 2) in cyl)
		self.failIf((0, 2.1, 2) in cyl)

	def test_hollow_cyl_generate_contains(self):
		from lepton.domain import Cylinder
		cyl = Cylinder((-1, -1, 0), (-3, -1, 0), 2, 1)
		for i in range(20):
			x, y, z = cyl.generate()
			mag = (y + 1)**2 + z**2
			self.failUnless(1 <= mag <= 4, ((y, z), mag))
			self.failUnless(-3 <= x <= -1, y)
			self.failUnless((x, y, z) in cyl, (x, y ,z))

		self.failUnless((-1.5, -2, 0) in cyl)
		self.failUnless((-2, -1, -1) in cyl)
		self.failUnless((-3, 0, 0) in cyl)
		self.failUnless((-1, 0, 0) in cyl)
		self.failIf((-2, 2, 0) in cyl)
		self.failIf((-2, -1, 2.1) in cyl)
		self.failIf((0, 0, 2) in cyl)
		self.failIf((0, 2.1, 2) in cyl)
		self.failIf((-1, -1, 0) in cyl)
		self.failIf((-2, -1, 0) in cyl)
		self.failIf((-3, -1, 0) in cyl)

	def test_shell_cyl_generate_contains(self):
		from lepton.domain import Cylinder
		cyl = Cylinder((2, 3, 4), (2, 3, 6), 2, 2)
		for i in range(20):
			x, y, z = cyl.generate()
			mag = (x - 2)**2 + (y - 3)**2
			self.assertAlmostEqual(mag, 4.0, 4)
			self.failUnless(4 <= z <= 6, z)
			self.failUnless((x, y, z) in cyl, (x, y ,z))

	def test_solid_cyl_intersect(self):
		from lepton.domain import Cylinder
		from lepton.particle_struct import Vec3
		cyl = Cylinder((0,-1,0), (0,1,0), 1)
		lines = [
			((-2, 0, 0), (0, 0, 0)),
			((2, 1, 0), (0, 1, 0)),
			((0, 0, -2), (0, 0, 0.8)),
			((0, 2, 0.5), (0, 0, 0.5)),
			((0.5, -3, 0), (0.5, -0.5, 0)),
		]
		expected = [
			((-1, 0, 0), (-1, 0, 0)),
			((1, 1, 0), (1, 0, 0)),
			((0, 0, -1), (0, 0, -1)),
			((0, 1, 0.5), (0, 1, 0)),
			((0.5, -1, 0), (0, -1, 0)),
		]

		for (start, end), (point, normal) in zip(lines, expected):
			p, N = cyl.intersect(start, end)
			self.assertVector(p, point)
			self.assertVector(N, normal)

			# Reverse direction should yield same point and inverse normal
			p, N = cyl.intersect(end, start)
			self.assertVector(p, point)
			self.assertVector(N, -Vec3(*normal))

	def test_solid_cyl_no_intersect(self):
		from lepton.domain import Cylinder
		cyl = Cylinder((-1,-1,0), (1,1,0), 4, 0)
		for start, end in [
			((-1, -1, 0), (-2, -1, 0)),
			((1, 1, 0), (1, 1.1, 0)),
			((-5, -5, 0), (-3, -3, 0)),
			((-2, -2, 4.1), (2, 2, 4.1)),
			((-2, -2, -2), (-2, -2, -2)),
			((-2, 10, 20), (-5, 15, 19))]:
			self.assertEqual(
				cyl.intersect(start, end), (None, None))
			self.assertEqual(
				cyl.intersect(end, start), (None, None))

	def test_hollow_cyl_intersect(self):
		from lepton.domain import Cylinder
		cyl = Cylinder((3,1,0), (5,1,0), 2, 1)
		lines = [
			((4, 4, 0), (4, 2, 0)),
			((3, 4, 0), (6, 1, 0)),
			((4, 1, 0), (4, 4, 0)),
			((2, 2.5, 0), (4, 2.5, 0)),
			((2, 1 , 0.5), (4, 1, 2.5)),
		]
		expected = [
			((4, 3, 0), (0, 1, 0)),
			((4, 3, 0), (0, 1, 0)),
			((4, 2, 0), (0, -1, 0)),
			((3, 2.5, 0), (-1, 0, 0)),
			((3, 1, 1.5), (-1, 0, 0)),
		]

		for (start, end), (point, normal) in zip(lines, expected):
			p, N = cyl.intersect(start, end)
			self.assertVector(p, point)
			self.assertVector(N, normal)

	def test_hollow_cyl_no_intersect(self):
		from lepton.domain import Cylinder
		cyl = Cylinder((0,1,-5), (0,1,0), 5, 1)
		for start, end in [
			((0, 1, -6), (0, 1, 1)),
			((0, 1.5, -2), (0, 0.5, -1)),
			((-10, 1, -5.1), (10, 1, -5.1)),
			((0, -5, 0.1), (0, 1, 0.1)),
			((0, 1, -3), (0, 1, -3)),
			((5, 1, -4), (5, 1, -1)),
			((-2, 10, 20), (-5, 15, 19))]:
			self.assertEqual(
				cyl.intersect(start, end), (None, None))
			self.assertEqual(
				cyl.intersect(end, start), (None, None))

	def test_solid_cone_generate_contains(self):
		from lepton.domain import Cone
		cone = Cone((-2, 0, 4), (-2, 2, 4), 2)
		for i in range(20):
			x, y, z = cone.generate()
			dist = math.sqrt(abs(x + 2)**2 + abs(z - 4)**2)
			self.failUnless(dist <= y * 2, (x, y, z))
			self.failUnless(0 <= y <= 2, y)
			self.failUnless((x, y, z) in cone, (x, y ,z))

		self.failUnless((-2, 0, 4) in cone)
		self.failUnless((-2, 1, 4) in cone)
		self.failUnless((-2.5, 0.5, 4) in cone)
		self.failUnless((-2, 1, 5) in cone)
		self.failIf((-2.1, 0, 4) in cone)
		self.failIf((-2, 2.1, 4) in cone)
		self.failIf((-100, 0, 1000) in cone)
		self.failIf((0, 0, 0) in cone)
	
	def test_nonaa_cone_generate_contains(self):
		# Test arbitrarily aligned cone
		from lepton.domain import Cone
		cone = Cone((-20, 12.5, -2.1), (-30, -10.22, 5.602), 5.77)
		for i in range(20):
			x, y, z = cone.generate()
			self.failUnless((x, y, z) in cone, (x, y ,z))

	def test_hollow_cone_generate_contains(self):
		from lepton.domain import Cone
		cone = Cone((1, -2, -1), (1, -2, 3), 3, 1)
		for i in range(20):
			x, y, z = cone.generate()
			r = (z + 1) / 4.0
			dist = math.sqrt(abs(x - 1)**2 + abs(y + 2)**2)
			self.failUnless(r <= dist <= r * 3 , (x, y, z, r))
			self.failUnless(-1 <= z <= 3, z)
			self.failUnless((x, y, z) in cone, (x, y ,z))

		self.failUnless((1, -2, -1) in cone)
		self.failUnless((1, -3.5, 3) in cone)
		self.failUnless((2, -2, 1) in cone)
		self.failIf((-1, -2, -0.9) in cone)
		self.failIf((1, -2, 3) in cone)
		self.failIf((-100, 0, 1000) in cone)
		self.failIf((0, 0, 0) in cone)

	def test_shell_cone_generate_contains(self):
		from lepton.domain import Cone
		cone = Cone((0, 0, 0), (-10, 0, 0), 3, 3)
		for i in range(20):
			x, y, z = cone.generate()
			r = -x * 3.0 / 10.0
			dist = math.sqrt(abs(y)**2 + abs(z)**2)
			self.assertAlmostEqual(dist, r, 4)
			self.failUnless(-10 <= x <= 0, z)
			self.failUnless((x, y, z) in cone, (x, y ,z))

		self.failUnless((0, 0, 0) in cone)
		self.failUnless((-10, 3, 0) in cone)
		self.failUnless((-10, 0, 3) in cone)
		self.failUnless((-5, 0, 1.5) in cone)
		self.failIf((-10, 0, 0) in cone)
		self.failIf((0.1, 0, 0) in cone)
		self.failIf((-0.1, 0, 0) in cone)
		self.failIf((-100, 0, 1000) in cone)
	
	def test_cone_apex_base_length(self):
		from lepton.domain import Cone
		from lepton.particle_struct import Vec3
		cone = Cone((2,2,2), (4,2,2), 2)
		self.assertVector(cone.apex, (2,2,2))
		self.assertVector(cone.base, (4,2,2))
		self.assertEqual(cone.length, 2)
		cone.apex = (2.5, 2, 2)
		self.assertVector(cone.apex, (2.5,2,2))
		self.assertVector(cone.base, (4,2,2))
		self.assertEqual(cone.length, 1.5)
		cone.base = (3, 3, 3)
		self.assertVector(cone.apex, (2.5,2,2))
		self.assertVector(cone.base, (3,3,3))
		self.assertEqual(cone.length, (Vec3(2.5,2,2) - Vec3(3,3,3)).length())
	
	def test_cone_radii(self):
		from lepton.domain import Cone
		cone = Cone((3,6,1), (9,1,3), 5, 4)
		self.assertEqual(cone.inner_radius, 4)
		self.assertEqual(cone.outer_radius, 5)
		cone.inner_radius = 3
		cone.outer_radius = 6
		self.assertEqual(cone.inner_radius, 3)
		self.assertEqual(cone.outer_radius, 6)
		self.assertRaises(ValueError, setattr, cone, "inner_radius", 7)
		self.assertEqual(cone.inner_radius, 3)
		self.assertRaises(ValueError, setattr, cone, "outer_radius", 2)
		self.assertEqual(cone.outer_radius, 6)

	def test_solid_cone_intersect(self):
		from lepton.domain import Cone
		from lepton.particle_struct import Vec3
		cone = Cone((0,1,0), (0,-1,0), 2)
		lines = [
			((-2, 0, 0), (0, 0, 0)),
			((3, 1, 0), (-1, 1, 0)),
			((0, 0, -2), (0, 0, 5)),
			((0.5, -3, 0), (0.5, -0.5, 0)),
			((-1, 2, 0), (1, 0, 0)),
			((-5, -2, 0), (5, 0, 0)),
			((0, 10, 1), (0, -10, 1)),
			((1, 1, 0), (-1, -1, 0)),
			((0, 0, 0), (3, 0, 0)),
		]
		expected = [
			((-1, 0, 0), Vec3(-1, 1, 0).normalize()),
			((0, 1, 0), (0, 1, 0)),
			((0, 0, -1), Vec3(0, 1, -1).normalize()),
			((0.5, -1, 0), (0, -1, 0)),
			((0, 1, 0), (0, 1, 0)),
			((0, -1, 0), (0, -1, 0)),
			((0, 0, 1), Vec3(0, 1, 1).normalize()),
			((0.5, 0.5, 0), Vec3(1, 1, 0).normalize()),
			((1, 0, 0), Vec3(-1, -1, 0).normalize()),
		]

		for (start, end), (point, normal) in zip(lines, expected):
			p, N = cone.intersect(start, end)
			self.assertVector(p, point)
			self.assertVector(N, normal)

	def test_solid_cone_no_intersect(self):
		from lepton.domain import Cone
		cone = Cone((-1,-1,0), (1,1,0), 4, 0)
		for start, end in [
			((-1, -1, 0), (-2, -1, 0)),
			((1, 1, 0), (1, 1.1, 0)),
			((-3, 7, 0), (7, -3, 0)),
			((-5, -5, 0), (-3, -3, 0)),
			((-2, -2, 4.1), (2, 2, 4.1)),
			((-2, -2, -2), (-2, -2, -2)),
			((-2, 10, 20), (-5, 15, 19))]:
			self.assertEqual(
				cone.intersect(start, end), (None, None))
	
	def test_hollow_cone_intersect(self):
		from lepton.domain import Cone
		from lepton.particle_struct import Vec3
		cone = Cone((0,0,2), (0,0,-1), 3, 1)
		lines = [
			((0, 0, -3), (0, 0, 3)),
			((0, -5, 0), (0, 5, 0)),
			((0, 0, 0), (0, 5, 0)),
			((2, 0, -2), (2, 0, 0)),
			((0.5, 0, -3), (0.5, 0, 3)),
		]
		expected = [
			((0, 0, 2), (0, 0, -1)),
			((0, -2, 0), Vec3(0, -1, 1).normalize()),
			((0, 2.0/3.0, 0), Vec3(0, -3, -1).normalize()),
			((2, 0, -1), (0, 0, -1)),
			((0.5, 0, 0.5), Vec3(-3, 0, -1).normalize()),
		]

		for (start, end), (point, normal) in zip(lines, expected):
			p, N = cone.intersect(start, end)
			self.assertVector(p, point)
			self.assertVector(N, normal)

	def test_hollow_cone_no_intersect(self):
		from lepton.domain import Cone
		cone = Cone((-1,-1,0), (1,1,0), 4, 3)
		for start, end in [
			((-1, -1, 0), (-2, -1, 0)),
			((1, 1, 0), (1, 1.1, 0)),
			((-3, 7, 0), (7, -3, 0)),
			((-5, -5, 0), (-3, -3, 0)),
			((-2, -2, 4.1), (2, 2, 4.1)),
			((-2, -2, -2), (-2, -2, -2)),
			((1, 1 ,0), (0, 0, 0)),
			((-2, 10, 20), (-5, 15, 19))]:
			self.assertEqual(
				cone.intersect(start, end), (None, None))

if __name__=='__main__':
	unittest.main()
