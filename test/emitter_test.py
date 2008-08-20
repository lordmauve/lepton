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


class EmitterTestBase:

	def assertVector(self, (vx,vy,vz), (x,y,z), tolerance=0.00001):
		self.failUnless(abs(vx - x) <= tolerance, ((vx,vy,vz), (x,y,z)))
		self.failUnless(abs(vy - y) <= tolerance, ((vx,vy,vz), (x,y,z)))
		self.failUnless(abs(vz - z) <= tolerance, ((vx,vy,vz), (x,y,z)))

	def assertVectorBetween(self, vec3, (lo_x,lo_y,lo_z), (hi_x,hi_y,hi_z)):
		self.failUnless(lo_x <= vec3.x <= hi_x, (vec3, (lo_x,lo_y,lo_z), (hi_x,hi_y,hi_z)))
		self.failUnless(lo_y <= vec3.y <= hi_y, (vec3, (lo_x,lo_y,lo_z), (hi_x,hi_y,hi_z)))
		self.failUnless(lo_z <= vec3.z <= hi_z, (vec3, (lo_x,lo_y,lo_z), (hi_x,hi_y,hi_z)))

	def assertColor(self, (cr,cg,cb,ca), (r,g,b,a), tolerance=0.00001):
		self.failUnless(abs(cr - r) <= tolerance, ((cr,cg,cb,ca), (r,g,b,a)))
		self.failUnless(abs(cg - g) <= tolerance, ((cr,cg,cb,ca), (r,g,b,a)))
		self.failUnless(abs(cb - b) <= tolerance, ((cr,cg,cb,ca), (r,g,b,a)))
		self.failUnless(abs(ca - a) <= tolerance, ((cr,cg,cb,ca), (r,g,b,a)))

	def assertColorBetween(self, color, (lo_r,lo_g,lo_b,lo_a), (hi_r,hi_g,hi_b,hi_a)):
		self.failUnless(lo_r <= color.r <= hi_r, 
			(color, (lo_r,lo_g,lo_b,lo_a), (hi_r,hi_g,hi_b,hi_a)))
		self.failUnless(lo_g <= color.g <= hi_g, 
			(color, (lo_r,lo_g,lo_b,lo_a), (hi_r,hi_g,hi_b,hi_a)))
		self.failUnless(lo_b <= color.b <= hi_b, 
			(color, (lo_r,lo_g,lo_b,lo_a), (hi_r,hi_g,hi_b,hi_a)))
		self.failUnless(lo_a <= color.a <= hi_a, 
			(color, (lo_r,lo_g,lo_b,lo_a), (hi_r,hi_g,hi_b,hi_a)))


class StaticEmitterTest(EmitterTestBase, unittest.TestCase):

	def test_StaticEmitter_template(self):
		from lepton import Particle, ParticleGroup
		from lepton.emitter import StaticEmitter

		emitter = StaticEmitter(rate=1, template=Particle(
			position=(1.0, 1.0, 1.0), velocity=(0, 5, 2), color=(0.5, 0.5, 0.5, 1.0)))
		group = ParticleGroup()
		count = emitter(1, group)
		group.update(0)
		self.assertEqual(count, 1)
		self.assertEqual(len(group), 1)
		particle = list(group)[0]
		self.assertVector(particle.position, (1,1,1))
		self.assertVector(particle.velocity, (0,5,2))
		self.assertColor(particle.color, (0.5, 0.5, 0.5, 1.0))

	def test_StaticEmitter_invalid_rate(self):
		from lepton import Particle, ParticleGroup
		from lepton.emitter import StaticEmitter

		self.assertRaises(ValueError, StaticEmitter, rate=-1)
	
	def test_StaticEmitter_deviation(self):
		from lepton import Particle, ParticleGroup
		from lepton.emitter import StaticEmitter

		template=Particle(
			position=(1.0, 1.0, 1.0), 
			velocity=(0, 0, 0), 
			size=(2,2,2),
			color=(0.5, 0.5, 0.5, 1.0))
		deviation=Particle(
			position=(0, 0, 0), 
			velocity=(5, 10, 5), 
			color=(1.0, 1.0, 1.0, 0))

		particle_count = 12345
		emitter = StaticEmitter(
			rate=particle_count, template=template, deviation=deviation)
		group = ParticleGroup()
		emitter(1, group)
		group.update(0)
		self.assertEqual(len(group), particle_count)

		position_mean =[0,0,0]
		position_var = [0,0,0]
		velocity_mean = [0,0,0]
		velocity_var = [0,0,0]
		size_mean = [0,0,0]
		size_var = [0,0,0]
		color_mean = [0,0,0,0]
		color_var = [0,0,0,0]

		def accum(mean_list, var_list, val, template):
			for i in range(len(mean_list)):
				mean_list[i] += val[i]
				d = val[i] - template[i]
				var_list[i] += d**2

		def calc_mean_var(mean_list, var_list):
			for i in range(len(mean_list)):
				mean_list[i] /= float(particle_count)
				var_list[i] = math.sqrt(var_list[i] / float(particle_count))

		for particle in group:
			accum(position_mean, position_var, particle.position, template.position)
			accum(velocity_mean, velocity_var, particle.velocity, template.velocity)
			accum(size_mean, size_var, particle.size, template.size)
			accum(color_mean, color_var, particle.color, template.color)

		calc_mean_var(position_mean, position_var) 
		calc_mean_var(velocity_mean, velocity_var) 
		calc_mean_var(size_mean, size_var) 
		calc_mean_var(color_mean, color_var) 
		
		self.assertVector(position_mean, template.position, tolerance=0.2)
		self.assertVector(position_var, deviation.position, tolerance=0.2)
		self.assertVector(velocity_mean, template.velocity, tolerance=0.2)
		self.assertVector(velocity_var, deviation.velocity, tolerance=0.2)
		self.assertVector(size_mean, template.size, tolerance=0.2)
		self.assertVector(size_var, deviation.size, tolerance=0.2)
		self.assertColor(color_mean, template.color, tolerance=0.2)
		self.assertColor(color_var, deviation.color, tolerance=0.2)

	def test_StaticEmitter_time_to_live(self):
		from lepton import Particle, ParticleGroup
		from lepton.emitter import StaticEmitter

		emitter = StaticEmitter(rate=1, time_to_live=3.0)
		group = ParticleGroup(controllers=[emitter])
		count = emitter(2, group)
		self.assertEqual(count, 2)
		self.assertEqual(emitter.time_to_live, 1)
		self.failUnless(emitter in group.controllers)
		count = emitter(2, group)
		# Since only one second remained before expiring
		# only one particle should be emitted
		self.assertEqual(count, 1)
		self.assertEqual(emitter.time_to_live, 0)
		self.failUnless(emitter not in group.controllers)
	
	def test_StaticEmitter_partial(self):
		from lepton import Particle, ParticleGroup
		from lepton.emitter import StaticEmitter

		emitter = StaticEmitter(rate=1)
		group = ParticleGroup()
		# It should take four quarter second updates to emit one
		self.assertEqual(emitter(0.25, group), 0)
		self.assertEqual(emitter(0.25, group), 0)
		self.assertEqual(emitter(0.25, group), 0)
		self.assertEqual(emitter(0.25, group), 1)
		group.update(0)
		self.assertEqual(len(group), 1)
	
	def test_StaticEmitter_emit(self):
		from lepton import ParticleGroup
		from lepton.emitter import StaticEmitter

		emitter = StaticEmitter()
		group = ParticleGroup()
		self.assertEqual(len(group), 0)
		emitter.emit(10, group)
		group.update(0)
		self.assertEqual(len(group), 10)

		# Negative emit value is equivilant to zero
		emitter.emit(-10, group)
		group.update(0)
		self.assertEqual(len(group), 10)

	
	def test_StaticEmitter_discrete(self):
		from lepton import Particle, ParticleGroup
		from lepton.emitter import StaticEmitter
		
		masses = (0.5, 2.0, 8.0)
		positions = ((1.0,1.0,1.0), (10.0,20.0,30.0), (-100.0,0.0,0.0))
		velocities = ((1,1,0),)
		colors = ((1.0, 1.0, 1.0, 0.5), (1.0, 0, 0, 1.0), (0, 1.0, 0, 1.0))
		emitter = StaticEmitter(rate=1, mass=masses, position=positions, 
			velocity=velocities, color=colors)
		group = ParticleGroup()
		emitter(3, group)
		group.update(0)
		self.assertEqual(len(group), 3)
		for particle in group:
			self.failUnless(particle.mass in masses, (particle.mass, masses))
			self.failUnless(tuple(particle.position) in positions, 
				(particle.position, positions))
			self.failUnless(tuple(particle.color) in colors, (particle.color, colors))
			self.assertVector(particle.velocity, (1,1,0))
	
	def test_StaticEmitter_domain(self):
		from lepton import Particle, ParticleGroup
		from lepton.emitter import StaticEmitter

		expected = (-42, 0, 9)
		
		class TestDomain:
			generate_calls = 0
			def generate(self):
				self.generate_calls += 1
				return expected
		
		domain = TestDomain()
		emitter = StaticEmitter(rate=1, position=domain)
		self.assertEqual(domain.generate_calls, 0)
		group = ParticleGroup()
		count = emitter(2, group)
		group.update(0)
		self.assertEqual(count, 2)
		self.assertEqual(domain.generate_calls, 2)
		self.assertEqual(len(group), 2)
		for particle in group:
			self.assertVector(particle.position, expected)


class PerParticleEmitterTest(EmitterTestBase, unittest.TestCase):

	def test_PerParticleEmitter_source_group(self):
		from lepton.emitter import PerParticleEmitter
		from lepton import ParticleGroup
		source_group = ParticleGroup()
		emitter = PerParticleEmitter(source_group)
		self.failUnless(emitter.source_group is source_group)

	def test_PerParticleEmitter_invalid_rate(self):
		from lepton import Particle, ParticleGroup
		from lepton.emitter import PerParticleEmitter

		self.assertRaises(ValueError, PerParticleEmitter, ParticleGroup(), rate=-1)

	def test_PerParticleEmitter_template(self):
		from lepton import Particle, ParticleGroup
		from lepton.emitter import PerParticleEmitter

		source_group = ParticleGroup()
		source_group.new(Particle(position=(1,1,1)))
		source_group.new(Particle(position=(2,2,2)))
		source_group.update(0)

		emitter = PerParticleEmitter(source_group, rate=1, template=Particle(
			position=(1.0, 1.0, 1.0), velocity=(0, 5, 2), color=(0.5, 0.5, 0.5, 1.0)))
		group = ParticleGroup()
		count = emitter(1, group)
		group.update(0)
		self.assertEqual(count, len(source_group))
		self.assertEqual(len(group), len(source_group))
		particle = list(group)[0]
		self.assertVector(particle.position, (1,1,1))
		self.assertVector(particle.velocity, (0,5,2))
		self.assertColor(particle.color, (0.5, 0.5, 0.5, 1.0))
		particle = list(group)[1]
		self.assertVector(particle.position, (2,2,2))
		self.assertVector(particle.velocity, (0,5,2))
		self.assertColor(particle.color, (0.5, 0.5, 0.5, 1.0))

	def test_PerParticleEmitter_emit(self):
		from lepton import ParticleGroup
		from lepton.emitter import PerParticleEmitter

		source_group = ParticleGroup()
		source_group.new(object())
		source_group.new(object())
		source_group.new(object())
		source_group.update(0)

		emitter = PerParticleEmitter(source_group)
		group = ParticleGroup()
		self.assertEqual(len(group), 0)
		emitter.emit(10, group)
		group.update(0)
		expected = 10 * len(source_group)
		self.assertEqual(len(group), expected)

		# Negative emit value is equivilant to zero
		emitter.emit(-10, group)
		group.update(0)
		self.assertEqual(len(group), expected)


	def test_PerParticleEmitter_emit_empty_source(self):
		from lepton import ParticleGroup
		from lepton.emitter import PerParticleEmitter

		emitter = PerParticleEmitter(ParticleGroup())
		group = ParticleGroup()
		self.assertEqual(len(group), 0)
		emitter.emit(10, group)
		group.update(0)
		self.assertEqual(len(group), 0)

	def test_PerParticleEmitter_time_to_live(self):
		from lepton import Particle, ParticleGroup
		from lepton.emitter import PerParticleEmitter

		source_group = ParticleGroup()
		source_group.new(object())
		source_group.new(object())
		source_group.new(object())
		source_group.new(object())
		source_group.update(0)

		emitter = PerParticleEmitter(source_group, rate=1, time_to_live=3.0)
		group = ParticleGroup(controllers=[emitter])
		count = emitter(2, group)
		self.assertEqual(count, 2 * len(source_group))
		self.assertEqual(emitter.time_to_live, 1)
		self.failUnless(emitter in group.controllers)
		count = emitter(2, group)
		# Since only one second remained before expiring
		# only one particle per should be emitted
		self.assertEqual(count, len(source_group))
		self.assertEqual(emitter.time_to_live, 0)
		self.failUnless(emitter not in group.controllers)
	
	def test_PerParticleEmitter_partial(self):
		from lepton import Particle, ParticleGroup
		from lepton.emitter import PerParticleEmitter

		source_group = ParticleGroup()
		source_group.new(object())
		source_group.new(object())
		source_group.new(object())
		source_group.update(0)

		emitter = PerParticleEmitter(source_group, rate=1)
		group = ParticleGroup()
		# It should take four quarter second updates to emit any
		self.assertEqual(emitter(0.25, group), 0)
		self.assertEqual(emitter(0.25, group), 0)
		self.assertEqual(emitter(0.25, group), 0)
		self.assertEqual(emitter(0.25, group), len(source_group))
		group.update(0)
		self.assertEqual(len(group), len(source_group))

if __name__ == '__main__':
	unittest.main()
