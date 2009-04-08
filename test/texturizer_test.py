#############################################################################
#
# Copyright (c) 2008, 2009 by Casey Duncan and contributors
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
import ctypes

try:
	import pyglet
	from pyglet.gl import *
except ImportError:
	import warnings
	warnings.warn("Pyglet not installed, some texturizer tests disabled")
	pyglet = None


class SpriteTexturizerTest(unittest.TestCase):
	
	def _make_group(self, pcount):
		from lepton import ParticleGroup
		group = ParticleGroup()
		self._add_particles(group, pcount)
		self.assertEqual(len(group), pcount)
		return group
	
	def _add_particles(self, group, pcount):
		from lepton import Particle
		for i in range(pcount):
			group.new(Particle())
		group.update(0)
	
	def test_default_coords(self):
		from lepton.texturizer import SpriteTexturizer
		tex = SpriteTexturizer(0)
		self.assertEqual(tex.tex_dimension, 2)
		expected = (0,0, 1,0, 1,1, 0,1)
		self.assertEqual(tex.tex_coords, None)
		self.assertEqual(tex.weights, None)
		group = self._make_group(4)
		coords = tex.generate_tex_coords(group)
		self.failUnless(len(coords) >= len(group) * 8, (len(coords), len(group)))
		self.assertEqual(tuple(coords), expected * (len(coords) // 8))
		return tex, group
	
	def test_default_coords_growing_group(self):
		tex, group = self.test_default_coords()
		self._add_particles(group, 200)
		expected = (0,0, 1,0, 1,1, 0,1)
		coords = tex.generate_tex_coords(group)
		self.failUnless(len(coords) >= len(group) * 8, (len(coords), len(group)))
		self.assertEqual(tuple(coords), expected * (len(coords) // 8))
	
	def test_single_coord_set(self):
		from lepton.texturizer import SpriteTexturizer
		coord_set = (0,0, 0.5,0, 0.5,0.5, 0,0.5)
		tex = SpriteTexturizer(0, coords=[coord_set])
		self.assertEqual(tex.tex_dimension, 2)
		self.assertEqual(tex.tex_coords, (coord_set,))
		self.assertEqual(tex.weights, None)
		group = self._make_group(4)
		coords = tex.generate_tex_coords(group)
		self.failUnless(len(coords) >= len(group) * 8, (len(coords), len(group)))
		self.assertEqual(tuple(coords), coord_set * (len(coords) // 8))
		return coord_set, tex, group
	
	def test_single_coord_set_growing_group(self):
		coord_set, tex, group = self.test_single_coord_set()
		self._add_particles(group, 200)
		expected = (0,0, 1,0, 1,1, 0,1)
		coords = tex.generate_tex_coords(group)
		self.failUnless(len(coords) >= len(group) * 8, (len(coords), len(group)))
		self.assertEqual(tuple(coords), coord_set * (len(coords) // 8))
	
	def test_mutiple_coord_sets(self):
		from lepton.texturizer import SpriteTexturizer
		coord_set1 = (0.5,0.5, 1,0.5, 1,1, 0.5,1)
		coord_set2 = ((0,0.5), (0.5,0.5), (0.5,1), (0,1))
		coord_set3 = (0.5,0,0, 1,0,0, 1,0.5,0, 0.5,0.5,0)
		tex = SpriteTexturizer(0, coords=[coord_set1, coord_set2, coord_set3])
		coord_sets = tex.tex_coords
		self.assertEqual(coord_sets, (
			(0.5,0.5, 1,0.5, 1,1, 0.5,1),
			(0,0.5, 0.5,0.5, 0.5,1, 0,1),
			(0.5,0, 1,0, 1,0.5, 0.5,0.5))
			)
		self.assertEqual(tex.weights, None)
		group = self._make_group(6)
		coords = tuple(tex.generate_tex_coords(group))
		self.failUnless(len(coords) >= len(group) * 8, (len(coords), len(group)))
		self.assertEqual(coords[:8], coord_sets[0])
		self.assertEqual(coords[8:16], coord_sets[1])
		self.assertEqual(coords[16:24], coord_sets[2])
		self.assertEqual(coords[24:32], coord_sets[0])
		self.assertEqual(coords[32:40], coord_sets[1])
		self.assertEqual(coords[40:48], coord_sets[2])
	
	def test_coord_set_weights(self):
		from lepton.texturizer import SpriteTexturizer
		coord_set1 = ((0.5,0.5), (1,0.5), (1,1), (0.5,1))
		coord_set2 = (0,0.5, 0.5,0.5, 0.5,1, 0,1)
		coord_set3 = (0.5,0, 1,0, 1,0.5, 0.5,0.5)
		tex = SpriteTexturizer(0, 
			coords=(coord_set1, coord_set2, coord_set3), weights=(20, 30, 50))
		coord_sets = tex.tex_coords
		self.assertEqual(coord_sets, (
			(0.5,0.5, 1,0.5, 1,1, 0.5,1),
			(0,0.5, 0.5,0.5, 0.5,1, 0,1),
			(0.5,0, 1,0, 1,0.5, 0.5,0.5))
			)
		self.assertEqual(len(tex.weights), 3)
		self.assertAlmostEqual(tex.weights[0], 0.20)
		self.assertAlmostEqual(tex.weights[1], 0.30)
		self.assertAlmostEqual(tex.weights[2], 0.50)
		group = self._make_group(1000)
		coords = tuple(tex.generate_tex_coords(group))
		self.failUnless(len(coords) >= 8000, (len(coords), len(group)))
		counts = {coord_sets[0]: 0, coord_sets[1]: 0, coord_sets[2]: 0}
		for i in range(1000):
			cset = coords[i*8:i*8+8]
			self.failUnless(cset in counts, cset)
			counts[cset] += 1
		self.assertEqual(sum(counts.values()), 1000)
		self.failUnless(250 > counts[coord_sets[0]] > 150, counts[coord_sets[0]])
		self.failUnless(375 > counts[coord_sets[1]] > 225, counts[coord_sets[1]])
		self.failUnless(600 > counts[coord_sets[2]] > 400, counts[coord_sets[2]])
	
	def test_coord_set_weights_deterministic(self):
		from lepton.texturizer import SpriteTexturizer
		coord_set1 = ((0.5,0.5), (1,0.5), (1,1), (0.5,1))
		coord_set2 = (0,0.5, 0.5,0.5, 0.5,1, 0,1)
		coord_set3 = (0.5,0, 1,0, 1,0.5, 0.5,0.5)
		tex = SpriteTexturizer(0, 
			coords=(coord_set1, coord_set2, coord_set3), weights=(20, 70, 10))
		coord_sets = tex.tex_coords
		group = self._make_group(20)
		coords = [tuple(tex.generate_tex_coords(group)) for i in range(20)]
		for cs in coords:
			self.assertEqual(cs, coords[0])

	if pyglet is not None:
		def _glGet(self, what):
			result = (ctypes.c_int * 1)()
			glGetIntegerv(what, result)
			return result[0]

		def test_set_state_restore_state(self):
			from lepton.texturizer import SpriteTexturizer
			texture = (ctypes.c_ulong * 1)()
			glGenTextures(1, texture)
			glDisable(GL_TEXTURE_2D)
			glBindTexture(GL_TEXTURE_2D, 0)
			sprite_tex = SpriteTexturizer(texture[0])
			self.failIf(self._glGet(GL_TEXTURE_2D))
			self.assertEqual(self._glGet(GL_TEXTURE_BINDING_2D), 0)
			sprite_tex.set_state()
			self.failUnless(self._glGet(GL_TEXTURE_2D))
			self.assertEqual(self._glGet(GL_TEXTURE_BINDING_2D), texture[0])
			sprite_tex.restore_state()
			self.failIf(self._glGet(GL_TEXTURE_2D))


if __name__ == '__main__':
	unittest.main()
