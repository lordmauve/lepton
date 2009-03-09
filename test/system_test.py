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


class TestGroup:

	drawn = False

	def __init__(self):
		self.particles = set()
		self.updated = 0
	
	def __iter__(self):
		# Iterate a copy to tolerate mutation during iteration
		return iter(set(self.particles))
	
	def update(self, td):
		self.updated += 1
		self.time_delta = td
	
	def draw(self):
		self.drawn = True


class TestController:

	def __init__(self):
		self.groups = set()
	
	def __call__(self, td, group):
		self.groups.add(group)
		self.time_delta = td


class SystemTest(unittest.TestCase):

	def test_add_remove_group(self):
		""""""
		from lepton import ParticleSystem
		system = ParticleSystem()
		self.failIf(system.groups)
		test_group = object()
		system.add_group(test_group)
		self.assertEqual(len(system.groups), 1)

		system.remove_group(test_group)
		self.assertEqual(len(system.groups), 0)
	
		self.assertRaises(ValueError, system.remove_group, test_group)

	def test_len_contains_iter(self):
		"""Test built in __len__, __contains__, __iter__"""
		from lepton import ParticleSystem
		system = ParticleSystem()
		test_group = object()
		self.assertFalse(test_group in system)
		self.assertEqual(list(iter(system)), [])
		system.add_group(test_group)
		self.assertEqual(len(system), 1)
		self.assertEqual(len(system.groups), len(system))
		self.assertTrue(test_group in system)
		self.assertEqual(list(iter(system)), [test_group])
		another_group = object()
		self.assertFalse(another_group in system)
		system.add_group(another_group)
		self.assertTrue(another_group in system)
		self.assertEqual(list(iter(system)), [test_group, another_group])

	def test_add_global_controllers(self):
		from lepton import ParticleSystem
		system = ParticleSystem()
		test_controller = TestController()
		self.assertEqual(len(system.controllers), 0)
		system.add_global_controller(test_controller)
		self.assertEqual(len(system.controllers), 1)
		self.assertEqual(list(system.controllers), [test_controller])

	def test_pkg_default_system(self):
		from lepton import ParticleSystem, default_system
		self.failUnless(isinstance(default_system, ParticleSystem))
	
	def test_update(self):
		from lepton import ParticleSystem
		system = ParticleSystem()
		group1 = TestGroup()
		group2 = TestGroup()
		system.add_group(group1)
		system.add_group(group2)
		self.failIf(group1.updated)
		self.failIf(group2.updated)
		system.update(0.05)
		self.failUnless(group1.updated)
		self.failUnless(group2.updated)
	
	def test_run_ahead(self):
		from lepton import ParticleSystem
		system = ParticleSystem()
		group1 = TestGroup()
		group2 = TestGroup()
		system.add_group(group1)
		system.add_group(group2)
		self.failIf(group1.updated)
		self.failIf(group2.updated)
		system.run_ahead(2, 30)
		self.assertEqual(group1.updated, 60)
		self.assertEqual(group1.time_delta, 1.0 / 30.0)
		self.assertEqual(group2.updated, 60)
		self.assertEqual(group2.time_delta, 1.0 / 30.0)
		self.failIf(group1.drawn)
		self.failIf(group2.drawn)

	def test_draw(self):
		from lepton import ParticleSystem
		system = ParticleSystem()
		group1 = TestGroup()
		group2 = TestGroup()
		system.add_group(group1)
		system.add_group(group2)
		self.failIf(group1.drawn)
		self.failIf(group2.drawn)
		system.draw()
		self.failUnless(group1.drawn)
		self.failUnless(group2.drawn)


if __name__=='__main__':
	unittest.main()
