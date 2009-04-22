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


class TestSystem(list):

	def add_group(self, group):
		self.append(group)


class TestController:
	group = None
	
	def __call__(self, td, group):
		self.group = group
		self.time_delta = td

class TestUnbindingController:
	"""Controller that unbinds itself when invoked"""
	group = None
	
	def __call__(self, td, group):
		self.group = group
		group.unbind_controller(self)

class TestBindingController:
	"""Controller that binds another controller when invoked"""
	group = None

	def __init__(self, ctrl):
		self.ctrl = ctrl
	
	def __call__(self, td, group):
		self.group = group
		group.bind_controller(self.ctrl)


class TestRenderer:
	group = None
	drawn = False

	def draw(self, group):
		self.drawn = True
		self.group = group


class TestParticle:
	velocity = (0,0,0)


class GroupTest(unittest.TestCase):

	def test_group_system(self):
		import lepton
		from lepton import ParticleGroup
		# By default the group should be added to the global system
		group = ParticleGroup()
		self.failUnless(group in lepton.default_system)
		lepton.default_system.add_group(group)

		# We should be able to override the system by argument
		test_system = TestSystem()
		group = ParticleGroup(system=test_system)
		self.failUnless(group in test_system)
		self.failIf(group in lepton.default_system)

		# If the global system is overridden, the group should respect that
		original_system = lepton.default_system
		try:
			lepton.default_system = test_system
			group = ParticleGroup()
			self.failUnless(group in test_system)
		finally:
			lepton.default_system = original_system

		# If None is specified for system, group is not added
		group = ParticleGroup(system=None)
		self.failIf(group in lepton.default_system)
		self.failIf(group in test_system)
	
	def test_bind_controllers(self):
		from lepton import ParticleGroup
		ctrl1 = TestController()
		ctrl2 = TestController()
		ctrl3 = TestController()
		ctrl4 = TestController()
		group = ParticleGroup()
		self.failIf(group.controllers)

		# Can bind controllers in constructor and after
		group = ParticleGroup(controllers=(ctrl1, ctrl2))
		self.assertEqual(tuple(group.controllers), (ctrl1, ctrl2))
		group.bind_controller(ctrl3, ctrl4)
		self.assertEqual(tuple(group.controllers), (ctrl1, ctrl2, ctrl3, ctrl4))

		'''
		# Can bind controllers at the class level and after
		class MyGroup(ParticleGroup):
			controllers = (ctrl1, ctrl2)
		group = MyGroup(controllers=[ctrl3])
		self.assertEqual(tuple(group.controllers), (ctrl1, ctrl2, ctrl3))
		group.bind_controller(ctrl4)
		self.assertEqual(tuple(group.controllers), (ctrl1, ctrl2, ctrl3, ctrl4))
		'''
	
	def test_unbind_controllers(self):
		from lepton import ParticleGroup
		ctrl1 = TestController()
		ctrl2 = TestController()
		group = ParticleGroup(controllers=(ctrl1, ctrl2))
		self.failUnless(ctrl1 in group.controllers)
		self.failUnless(ctrl2 in group.controllers)
		group.unbind_controller(ctrl1)
		self.failUnless(ctrl1 not in group.controllers)
		self.failUnless(ctrl2 in group.controllers)
		group.unbind_controller(ctrl2)
		self.failUnless(ctrl1 not in group.controllers)
		self.failUnless(ctrl2 not in group.controllers)
		self.assertRaises(ValueError, group.unbind_controller, ctrl1)
	
	def test_modify_controllers_during_update(self):
		from lepton import ParticleGroup
		ctrl1 = TestController()
		ctrl2 = TestUnbindingController()
		ctrl4 = TestController()
		ctrl3 = TestBindingController(ctrl4)
		group = ParticleGroup(controllers=(ctrl1, ctrl2, ctrl3))
		self.assertEqual(len(group.controllers), 3)
		self.failUnless(ctrl1 in group.controllers)
		self.failUnless(ctrl2 in group.controllers)
		self.failUnless(ctrl3 in group.controllers)
		self.failUnless(ctrl4 not in group.controllers)
		group.update(1)
		self.failUnless(ctrl1.group is group)
		self.failUnless(ctrl2.group is group)
		self.failUnless(ctrl3.group is group)
		self.failUnless(ctrl4.group is None)
		self.assertEqual(len(group.controllers), 3)
		self.failUnless(ctrl1 in group.controllers)
		self.failUnless(ctrl2 not in group.controllers)
		self.failUnless(ctrl3 in group.controllers)
		self.failUnless(ctrl4 in group.controllers)

	def test_set_renderer(self):
		from lepton import ParticleGroup
		renderer = TestRenderer()
		group = ParticleGroup()
		self.assertEqual(group.renderer, None)

		# Can set renderer after init
		group.renderer = renderer
		self.assertEqual(group.renderer, renderer)

		# Can set renderer at init
		group2 = ParticleGroup(renderer=renderer)
		self.assertEqual(group2.renderer, renderer)

		# Can set renderer back to None
		group.renderer = None
		self.assertEqual(renderer.group, None)
	
	def test_new_particle(self):
		from lepton import ParticleGroup, Particle
		group = ParticleGroup()
		self.assertEqual(len(group), 0)
		self.assertEqual(group.new_count(), 0)
		p1 = Particle(age=1)
		p2 = Particle(age=2)
		group.new(p1)
		self.assertEqual(len(group), 0)
		self.assertEqual(group.new_count(), 1)
		self.failIf(list(group))
		group.new(p2)
		self.assertEqual(len(group), 0)
		self.assertEqual(group.new_count(), 2)
		self.failIf(list(group))
		group.update(0) # incorporate new particles
		self.assertEqual(len(group), 2)
		self.assertEqual(group.new_count(), 0)
		particles = list(group)
		self.assertEqual(len(particles), 2)
		self.assertEqual(particles[0].age, 1)
		self.assertEqual(particles[1].age, 2)
		return group, particles
	
	def test_new_particle_kwargs(self):
		from lepton import ParticleGroup, Particle
		group = ParticleGroup()
		self.assertEqual(len(group), 0)
		self.assertEqual(group.new_count(), 0)
		p = group.new(position=(1,-1,2), age=2)
		self.assertEqual(tuple(p.position), (1, -1, 2))
		self.assertEqual(p.age, 2)
		tmpl_p = Particle(age=3, velocity=(-1,2,3))
		p = group.new(tmpl_p, age=5)
		self.assertEqual(tuple(p.velocity), (-1,2,3))
		self.assertEqual(p.age, 5)
		self.assertEqual(len(group), 0)
		self.assertEqual(group.new_count(), 2)
		self.failIf(list(group))
		group.update(0) # incorporate new particles
		self.assertEqual(len(group), 2)
		self.assertEqual(group.new_count(), 0)
	
	def test_particle_attrs(self):
		from lepton import ParticleGroup
		p = TestParticle()
		p.position=(1,2,3)
		p.velocity=(4,5,6)
		p.color=(7,8,9,10)
		p.size=(11,12,13)
		p.up=(-1,-2,-3)
		p.rotation=(-4,-5,-6)
		p.age=111
		p.mass=2

		group = ParticleGroup()
		newp = group.new(p)
		self.assertEqual(tuple(newp.position), p.position)
		self.assertEqual(tuple(newp.velocity), p.velocity)
		self.assertEqual(tuple(newp.color), p.color)
		self.assertEqual(tuple(newp.size), p.size)
		self.assertEqual(tuple(newp.up), p.up)
		self.assertEqual(tuple(newp.rotation), p.rotation)
		self.assertEqual(newp.age, p.age)
		self.assertEqual(newp.mass, p.mass)
	
	def test_particle_vector_swizzle(self):
		from lepton import ParticleGroup
		group = ParticleGroup()
		newp = group.new(TestParticle())
		self.assertEqual(tuple(newp.velocity), (0,0,0))
		self.assertEqual(newp.velocity.x, 0)
		self.assertEqual(newp.velocity.y, 0)
		self.assertEqual(newp.velocity.z, 0)
		newp.velocity.x = 2
		newp.velocity.y = -2
		newp.velocity.z = 1
		self.assertEqual(tuple(newp.velocity), (2,-2,1))
		self.assertEqual(newp.velocity.x, 2)
		self.assertEqual(newp.velocity.y, -2)
		self.assertEqual(newp.velocity.z, 1)
		newp.velocity = (3,4,5)
		self.assertEqual(tuple(newp.velocity), (3,4,5))
		self.assertEqual(newp.velocity.x, 3)
		self.assertEqual(newp.velocity.y, 4)
		self.assertEqual(newp.velocity.z, 5)

		self.assertEqual(tuple(newp.color), (0,0,0,0))
		self.assertEqual(newp.color.r, 0)
		self.assertEqual(newp.color.g, 0)
		self.assertEqual(newp.color.b, 0)
		self.assertEqual(newp.color.a, 0)
		newp.color.r = 1
		newp.color.g = -2
		newp.color.b = 3
		newp.color.a = -1
		self.assertEqual(tuple(newp.color), (1,-2,3,-1))
		self.assertEqual(newp.color.r, 1)
		self.assertEqual(newp.color.g, -2)
		self.assertEqual(newp.color.b, 3)
		self.assertEqual(newp.color.a, -1)
		newp.color = (5,4,3,2)
		self.assertEqual(tuple(newp.color), (5,4,3,2))
		self.assertEqual(newp.color.r, 5)
		self.assertEqual(newp.color.g, 4)
		self.assertEqual(newp.color.b, 3)
		self.assertEqual(newp.color.a, 2)
		newp.color = (6,5,4) # alpha defaults to 1
		self.assertEqual(tuple(newp.color), (6,5,4,1))
		self.assertEqual(newp.color.r, 6)
		self.assertEqual(newp.color.g, 5)
		self.assertEqual(newp.color.b, 4)
		self.assertEqual(newp.color.a, 1)
	
	def test_particle_vector_clamp(self):
		from lepton import ParticleGroup
		group = ParticleGroup()
		p = TestParticle()
		p.color = (2,0.5,-1,5)
		p.size = (0, 2, 0)
		newp = group.new(p)
		self.assertEqual(tuple(newp.color), p.color)
		self.assertEqual(tuple(newp.size), p.size)
		self.assertEqual(tuple(newp.color.clamp(0, 1)), (1, 0.5, 0, 1))
		self.assertEqual(tuple(newp.size.clamp(1, 1.5)), (1, 1.5, 1))
	
	def test_mass_new_particles(self):
		from lepton import ParticleGroup
		count = 12345
		group = ParticleGroup()
		p = TestParticle()
		for i in xrange(count):
			p.mass = i
			newp = group.new(p)
			self.assertEqual(newp.mass, p.mass)

		group.update(0)
		piter = iter(group)
		for i in xrange(count):
			newp = piter.next()
			self.assertEqual(newp.mass, i)

		self.assertEqual(len(group), count)
		self.assertEqual(group.killed_count(), 0)
		self.assertRaises(StopIteration, piter.next)
	
	def test_particle_ref_invalidation(self):
		from lepton.group import InvalidParticleRefError
		group, particles = self.test_new_particle()
		piter = iter(group)
		self.assertEqual(piter.next().age, particles[0].age)
		group.update(0) # Invalidates particle references and iter
		self.assertRaises(InvalidParticleRefError, getattr, particles[0], 'age')
		self.assertRaises(InvalidParticleRefError, getattr, particles[1], 'age')
		self.assertRaises(InvalidParticleRefError, piter.next)
	
	def test_kill_particle(self):
		group, particles = self.test_new_particle()
		group.kill(particles[0])
		self.assertEqual(len(group), 1)
		group.kill(particles[1])
		self.assertEqual(len(group), 0)
		self.assertEqual(list(group), [])
		# group.kill() only accepts particle ref objects
		p3 = TestParticle()
		self.assertRaises(TypeError, group.kill, p3)
		# update should reclaim killed particles
		self.assertEqual(group.killed_count(), 2)
		group.update(0)
		self.assertEqual(group.killed_count(), 0)
		self.assertEqual(len(group), 0)

	def test_len(self):
		from lepton import ParticleGroup
		group = ParticleGroup()
		self.assertEqual(len(group), 0)
		group.new(TestParticle())
		self.assertEqual(len(group), 0)
		group.new(TestParticle())
		self.assertEqual(len(group), 0)
		# New particles become visible on update
		group.update(0)
		self.assertEqual(len(group), 2)
	
	def test_update(self):
		group, particles = self.test_new_particle()
		ctrl1 = TestController()
		ctrl2 = TestController()
		group.bind_controller(ctrl1, ctrl2)
		group.update(0.33)
		self.failUnless(ctrl1.group is group)
		self.assertAlmostEqual(ctrl1.time_delta, 0.33)
		self.failUnless(ctrl2.group is group)
		self.assertAlmostEqual(ctrl2.time_delta, 0.33)
			
	def test_draw(self):
		group, particles = self.test_new_particle()
		renderer = TestRenderer()
		self.assertFalse(renderer.drawn)
		self.failUnless(renderer.group is None)
		group.renderer = renderer
		group.draw()
		self.assertTrue(renderer.drawn)
		self.failUnless(renderer.group is group)


if __name__=='__main__':
	unittest.main()
