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

class DummyDomain(object):
	
	def __init__(self, size=0):
		self.size = 0
	
	def __contains__(self, point):
		x, y, z = point
		return x <= self.size and y <= self.size and z <= self.size


class ControllerTest(unittest.TestCase):

	def assertVector(self, vec3, (x,y,z)):
		tolerance = 0.00001
		self.failUnless(abs(vec3.x - x) <= tolerance, (vec3, (x,y,z)))
		self.failUnless(abs(vec3.y - y) <= tolerance, (vec3, (x,y,z)))
		self.failUnless(abs(vec3.z - z) <= tolerance, (vec3, (x,y,z)))
	
	def assertFloatEqiv(self, f1, f2):
		tolerance = 0.00001
		self.failUnless(abs(f2 - f1) <= tolerance, (f1, f2))
	
	def assertMag(self, vec, mag):
		tolerance = 0.00001
		self.failUnless(vec.x**2 + vec.y**2 + vec.z**2 <= mag**2 + tolerance, (vec, mag))
	
	def _make_group(self):
		from lepton import Particle, ParticleGroup
		g = ParticleGroup()
		g.new(Particle((0,0,0), (0,0,0)))
		g.new(Particle((0,0,0), (1,1,1), size=(2,2,2)))
		g.new(Particle((1,1,1), (-2,-2,-2), size=(3,2,0)))
		g.update(0)
		return g

	def test_Gravity_controller(self):
		from lepton import controller
		group = self._make_group()
		gravity = controller.Gravity((0.5, 1.0, 2.0))
		gravity(0.1, group)
		p = list(group)
		self.assertVector(p[0].velocity, (0.05, 0.1, 0.2))
		self.assertVector(p[1].velocity, (1.05, 1.1, 1.2))
		self.assertVector(p[2].velocity, (-1.95, -1.9, -1.8))
	
	def test_Lifetime_controller(self):
		from lepton import controller, Particle, ParticleGroup
		g = ParticleGroup()
		g.new(Particle(age=0))
		g.new(Particle(age=0.8))
		g.new(Particle(age=1.0))
		g.new(Particle(age=0.75))
		lifetime = controller.Lifetime(max_age=0.75)
		g.update(0)
		lifetime(0, g)
		p = list(g)
		self.assertEqual(len(g), 2)
		self.assertFloatEqiv(p[0].age, 0.0)
		self.assertFloatEqiv(p[1].age, 0.75)
	
	def test_Movement_controller_simple(self):
		from lepton import controller
		group = self._make_group()
		
		# Simple movement, no acceleration or damping
		movement = controller.Movement()
		movement(0.2, group)
		p = list(group)
		self.assertVector(p[0].position, (0,0,0))
		self.assertVector(p[1].position, (0.2,0.2,0.2))
		self.assertVector(p[2].position, (0.6,0.6,0.6))

	def test_Movement_controller_damping_scalar(self):
		from lepton import controller
		group = self._make_group()

		# Movement w/damping
		movement = controller.Movement(damping=0.9)
		movement(0.1, group)
		p = list(group)
		self.assertVector(p[0].velocity, (0,0,0))
		self.assertVector(p[1].velocity, (0.9,0.9,0.9))
		self.assertVector(p[2].velocity, (-1.8,-1.8,-1.8))

	def test_Movement_controller_damping_vector(self):
		from lepton import controller
		group = self._make_group()

		# Movement w/damping vector
		movement = controller.Movement(damping=(1,0.9,1))
		movement(0.1, group)
		p = list(group)
		self.assertVector(p[0].velocity, (0,0,0))
		self.assertVector(p[1].velocity, (1.0,0.9,1.0))
		self.assertVector(p[2].velocity, (-2.0,-1.8,-2.0))

	def test_Growth_controller_simple(self):
		from lepton import controller
		group = self._make_group()

		# Simple growth, no damping
		growth = controller.Growth(1)
		growth(0.2, group)
		p = list(group)
		self.assertVector(p[0].size, (0.2,0.2,0.2))
		self.assertVector(p[1].size, (2.2,2.2,2.2))
		self.assertVector(p[2].size, (3.2,2.2,0.2))

	def test_Growth_controller_damping_scalar(self):
		from lepton import controller
		group = self._make_group()

		# Growth w/damping
		growth = controller.Growth(growth=1, damping=0.9)
		growth(0.1, group)
		growth(0.1, group)
		p = list(group)
		self.assertVector(p[0].size, (0.19,0.19,0.19))
		self.assertVector(p[1].size, (2.19,2.19,2.19))
		self.assertVector(p[2].size, (3.19,2.19,0.19))

	def test_Growth_controller_damping_vector(self):
		from lepton import controller
		group = self._make_group()

		# Growth w/damping vector
		growth = controller.Growth(growth=(2,1,0), damping=(1,0.9,0))
		growth(0.1, group)
		growth(0.1, group)
		p = list(group)
		self.assertVector(p[0].size, (0.4,0.19,0))
		self.assertVector(p[1].size, (2.4,2.19,2))
		self.assertVector(p[2].size, (3.4,2.19,0))

	def test_Movement_controller_max_velocity(self):
		from lepton import controller
		group = self._make_group()

		# Movement w/max velocity
		movement = controller.Movement(max_velocity=0.5)
		movement(0, group)
		p = list(group)
		self.assertMag(p[0].velocity, 0)
		self.assertMag(p[1].velocity, 0.5)
		self.assertMag(p[2].velocity, 0.5)

	def test_Movement_controller_min_velocity(self):
		from lepton import controller
		group = self._make_group()

		# Movement w/min velocity
		movement = controller.Movement(min_velocity=2.0)
		movement(0, group)
		p = list(group)
		self.assertMag(p[0].velocity, 0)
		self.assertMag(p[1].velocity, 2.0)
		self.assertMag(p[2].velocity, math.sqrt(12))
	
	def test_Collector_controller_collect_inside(self):
		from lepton import controller
		group = self._make_group()

		collector = controller.Collector(DummyDomain(size=0.5))
		self.assertEqual(len(group), 3)
		collector(0, group)
		self.assertEqual(len(group), 1)
		self.assertEqual(collector.collected_count, 2)
		p = list(group)
		self.assertVector(p[0].position, (1, 1, 1))

	def test_Collector_controller_collect_outside(self):
		from lepton import controller
		group = self._make_group()

		collector = controller.Collector(DummyDomain(size=0.5), collect_inside=False)
		self.assertEqual(len(group), 3)
		collector(0, group)
		self.assertEqual(len(group), 2)
		self.assertEqual(collector.collected_count, 1)
		p = list(group)
		self.assertVector(p[0].position, (0, 0, 0))
		self.assertVector(p[1].position, (0, 0, 0))

	def test_Collector_controller_callback(self):
		from lepton import controller
		group = self._make_group()
		collector = None
			
		killed_pos = []
		def callback(particle, pgroup, controller):
			self.assertEqual(controller, collector)
			self.assertEqual(pgroup, group)
			killed_pos.append(tuple(particle.position))

		collector = controller.Collector(DummyDomain(size=0.5), callback=callback)
		collector(0, group)
		self.assertEqual(len(group), 1)
		p = list(group)
		self.assertVector(p[0].position, (1, 1, 1))
		self.assertEqual(killed_pos, [(0,0,0), (0,0,0)])

if __name__=='__main__':
	unittest.main()
