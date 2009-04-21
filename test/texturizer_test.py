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


class TexTestBase:

	def assertVector(self, vec3, (x,y,z), tolerance=0.0001):
		self.failUnless(abs(vec3.x - x) <= tolerance, (vec3, (x,y,z)))
		self.failUnless(abs(vec3.y - y) <= tolerance, (vec3, (x,y,z)))
		self.failUnless(abs(vec3.z - z) <= tolerance, (vec3, (x,y,z)))
	
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


class SpriteTexturizerTest(TexTestBase, unittest.TestCase):
	
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

	def test_aspect_adjust(self):
		from lepton.texturizer import SpriteTexturizer
		coord_set1 = (0,0, 1,0, 1,0.5, 0,0.5)
		coord_set2 = (0,0.5, 0.5,0.5, 0.5,1, 0,1)
		tex = SpriteTexturizer(0, coords=(coord_set1, coord_set2))
		self.failIf(tex.aspect_adjust_width)
		self.failIf(tex.aspect_adjust_height)
		sizes = [
			(1, 1, 0),
			(2, 3, 0),
		]
		group = self._make_group(2)
		for size, p in zip(sizes, group):
			p.size = size
		self.assertEqual([tuple(p.size) for p in group], sizes)
		tex.generate_tex_coords(group)
		self.assertEqual([tuple(p.size) for p in group], sizes)
		tex.aspect_adjust_width = True
		expected = [
			(2, 1, 0),
			(3, 3, 0),
		]
		tex.generate_tex_coords(group)
		for p, b in zip(group, expected):
			self.assertVector(p.size, b)

		for size, p in zip(sizes, group):
			p.size = size
		self.assertEqual([tuple(p.size) for p in group], sizes)
		tex.aspect_adjust_width = False
		tex.aspect_adjust_height = True
		expected = [
			(1, 0.5, 0),
			(2, 2, 0),
		]
		tex.generate_tex_coords(group)
		for p, b in zip(group, expected):
			self.assertVector(p.size, b)

	def test_invalid_args(self):
		from lepton.texturizer import SpriteTexturizer
		self.assertRaises(TypeError, SpriteTexturizer, 0, object())
		self.assertRaises(TypeError, SpriteTexturizer, 0, [(0,0,0,0,0,0,0,0)], object())
		self.assertRaises(ValueError, SpriteTexturizer, 0, [])
		self.assertRaises(ValueError, SpriteTexturizer, 0, [(0,0)])
		self.assertRaises(ValueError, SpriteTexturizer, 0, [(0,0,0,0,0,0,0,0)], [])
		self.assertRaises(ValueError, SpriteTexturizer, 0, [(0,0,0,0,0,0,0,0)], [-1])
		self.assertRaises(ValueError, SpriteTexturizer, 0, [(0,0,0,0,0,0,0,0)], [1,1])
		self.assertRaises(ValueError, 
			SpriteTexturizer, 0, [(0,0,0,0,0,0,0,0), (0,0,0,0,0,0,0,0)], [1,-1])

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


class FlipBookTexturizerTest(TexTestBase, unittest.TestCase):

	def test_2D_single_duration_loop(self):
		from lepton.texturizer import FlipBookTexturizer
		coord_sets = [
			(0,0, 0.5,0, 0.5,0.5, 0,0.5),
			(0.5,0, 1,0, 1,0.5, 0.5,0.5),
			(0,0.5, 0.5,0.5, 0.5,1, 0,1),
			(0.5,0.5, 1,0.5, 1,1, 0.5,1),
			]
		fbtex = FlipBookTexturizer(0,
			coords=coord_sets,
			duration=0.1,
			)
		self.failUnless(fbtex.loop)
		self.assertAlmostEqual(fbtex.duration, 0.1)
		self.assertEqual(fbtex.tex_dimension, 2)
		group = self._make_group(10)
		age = 0.0
		for p in group:
			p.age = age
			age += 0.06
		coords = tuple(fbtex.generate_tex_coords(group))
		self.failUnless(len(coords) >= len(group) * 8, (len(coords), len(group)))
		self.assertEqual(coords[:8], coord_sets[0])
		self.assertEqual(coords[8:16], coord_sets[0])
		self.assertEqual(coords[16:24], coord_sets[1])
		self.assertEqual(coords[24:32], coord_sets[1])
		self.assertEqual(coords[32:40], coord_sets[2])
		self.assertEqual(coords[40:48], coord_sets[3])
		self.assertEqual(coords[48:56], coord_sets[3])
		self.assertEqual(coords[56:64], coord_sets[0])
		self.assertEqual(coords[64:72], coord_sets[0])
		self.assertEqual(coords[72:80], coord_sets[1])
		# Next frame
		group.update(0.05)
		coords = tuple(fbtex.generate_tex_coords(group))
		self.assertEqual(coords[:8], coord_sets[0])
		self.assertEqual(coords[8:16], coord_sets[1])
		self.assertEqual(coords[16:24], coord_sets[1])
		self.assertEqual(coords[24:32], coord_sets[2])
		self.assertEqual(coords[32:40], coord_sets[2])
		self.assertEqual(coords[40:48], coord_sets[3])
		self.assertEqual(coords[48:56], coord_sets[0])
		self.assertEqual(coords[56:64], coord_sets[0])
		self.assertEqual(coords[64:72], coord_sets[1])
		self.assertEqual(coords[72:80], coord_sets[1])
	
	def test_2D_single_duration_no_loop(self):
		from lepton.texturizer import FlipBookTexturizer
		coord_sets = [
			(0,0, 0.5,0, 0.5,0.5, 0,0.5),
			(0.5,0, 1,0, 1,0.5, 0.5,0.5),
			(0,0.5, 0.5,0.5, 0.5,1, 0,1),
			(0.5,0.5, 1,0.5, 1,1, 0.5,1),
			]
		fbtex = FlipBookTexturizer(0,
			coords=coord_sets,
			duration=0.03,
			loop=False,
			)
		self.failIf(fbtex.loop)
		self.assertAlmostEqual(fbtex.duration, 0.03)
		group = self._make_group(10)
		age = 0.0
		for p in group:
			p.age = age
			age += 0.016
		coords = tuple(fbtex.generate_tex_coords(group))
		self.failUnless(len(coords) >= len(group) * 8, (len(coords), len(group)))
		self.assertEqual(coords[:8], coord_sets[0])
		self.assertEqual(coords[8:16], coord_sets[0])
		self.assertEqual(coords[16:24], coord_sets[1])
		self.assertEqual(coords[24:32], coord_sets[1])
		self.assertEqual(coords[32:40], coord_sets[2])
		self.assertEqual(coords[40:48], coord_sets[2])
		self.assertEqual(coords[48:56], coord_sets[3])
		self.assertEqual(coords[56:64], coord_sets[3])
		self.assertEqual(coords[64:72], coord_sets[3])
		self.assertEqual(coords[72:80], coord_sets[3])
		# Next frame
		group.update(0.02)
		coords = tuple(fbtex.generate_tex_coords(group))
		self.assertEqual(coords[:8], coord_sets[0])
		self.assertEqual(coords[8:16], coord_sets[1])
		self.assertEqual(coords[16:24], coord_sets[1])
		self.assertEqual(coords[24:32], coord_sets[2])
		self.assertEqual(coords[32:40], coord_sets[2])
		self.assertEqual(coords[40:48], coord_sets[3])
		self.assertEqual(coords[48:56], coord_sets[3])
		self.assertEqual(coords[56:64], coord_sets[3])
		self.assertEqual(coords[64:72], coord_sets[3])
		self.assertEqual(coords[72:80], coord_sets[3])
	
	def test_2D_duration_list_loop(self):
		from lepton.texturizer import FlipBookTexturizer
		coord_sets = [
			(0,0, 0.5,0, 0.5,0.5, 0,0.5),
			(0.5,0, 1,0, 1,0.5, 0.5,0.5),
			(0,0.5, 0.5,0.5, 0.5,1, 0,1),
			(0.5,0.5, 1,0.5, 1,1, 0.5,1),
			]
		durations = (0.12, 0.3, 0.2, 0.15)
		times = []
		t = 0
		for d in durations:
			t += d
			times.append(t)
		fbtex = FlipBookTexturizer(0,
			coords=coord_sets,
			duration=durations,
			)
		self.failUnless(fbtex.loop)
		for d, expected in zip(fbtex.duration, durations):
			self.assertAlmostEqual(d, expected)
		group = self._make_group(10)
		age = 0.0
		for p in group:
			p.age = age % 2.0
			age += 0.7
		for f in range(5):
			coords = tuple(fbtex.generate_tex_coords(group))
			self.failUnless(len(coords) >= len(group) * 8, (len(coords), len(group)))
			i = 0
			for p, t in zip(group, times):
				age = p.age % times[-1]
				c = 0
				while c < 3 and age > times[c]:
					c += 1
				self.assertEqual(coords[i:i+8], coord_sets[c], "f=%s i=%s c=%s age=%s: %s != %s" % 
					(f, i, c, p.age, coords[i:i+8], coord_sets[c]))
				i += 8
			group.update(0.2)
		
	def test_2D_duration_list_no_loop(self):
		from lepton.texturizer import FlipBookTexturizer
		coord_sets = [
			(0,0, 0.5,0, 0.5,0.5, 0,0.5),
			(0.5,0, 1,0, 1,0.5, 0.5,0.5),
			(0,0.5, 0.5,0.5, 0.5,1, 0,1),
			(0.5,0.5, 1,0.5, 1,1, 0.5,1),
			]
		durations = (0.5, 0.25, 0.3, 0.4)
		times = []
		t = 0
		for d in durations:
			t += d
			times.append(t)
		fbtex = FlipBookTexturizer(0,
			coords=coord_sets,
			duration=durations,
			loop=False,
			)
		self.failIf(fbtex.loop)
		for d, expected in zip(fbtex.duration, durations):
			self.assertAlmostEqual(d, expected, 6)
		group = self._make_group(10)
		age = 0.0
		for p in group:
			p.age = age % 2.0
			age += 0.7
		for f in range(5):
			coords = tuple(fbtex.generate_tex_coords(group))
			self.failUnless(len(coords) >= len(group) * 8, (len(coords), len(group)))
			i = 0
			for p, t in zip(group, times):
				c = 0
				while c < 3 and p.age > times[c]:
					c += 1
				self.assertEqual(coords[i:i+8], coord_sets[c], "f=%s i=%s c=%s age=%s: %s != %s" % 
					(f, i, c, p.age, coords[i:i+8], coord_sets[c]))
				i += 8
			group.update(0.2)
	
	def test_default_r_coords(self):
		from lepton.texturizer import FlipBookTexturizer
		fbtex = FlipBookTexturizer(0,
			coords=[(0,0, 0.5,0, 0.5,0.5, 0,0.5)],
			duration=1,
			dimension=3)
		self.assertEqual(fbtex.tex_dimension, 3)
		coords = fbtex.tex_coords
		self.assertEqual(coords, ((0,0,0, 0.5,0,0, 0.5,0.5,0, 0,0.5,0),))
		fbtex = FlipBookTexturizer(0,
			coords=[((0.5,0), (1,0), (1,0.5), (0.5,0.5))],
			duration=1,
			dimension=3)
		self.assertEqual(fbtex.tex_dimension, 3)
		coords = fbtex.tex_coords
		self.assertEqual(coords, ((0.5,0,0, 1,0,0, 1,0.5,0, 0.5,0.5,0),))
	
	def test_3D_single_duration_loop(self):
		from lepton.texturizer import FlipBookTexturizer
		coord_sets = [
			(0,0,0, 1,0,0, 1,1,0, 0,1,0),
			(0,0,0.5, 1,0,0.5, 1,1,0.5, 0,1,0.5),
			(0,0,1, 1,0,1, 1,1,1, 0,1,1),
			]
		fbtex = FlipBookTexturizer(0,
			coords=coord_sets,
			duration=0.1,
			dimension=3,
			)
		self.assertEqual(fbtex.tex_dimension, 3)
		self.assertAlmostEqual(fbtex.duration, 0.1)
		self.failUnless(fbtex.loop)
		group = self._make_group(10)
		age = 0.0
		for p in group:
			p.age = age % 0.4
			age += 0.07
		times = [0.1, 0.2, 0.3]
		for f in range(5):
			coords = tuple(fbtex.generate_tex_coords(group))
			self.failUnless(len(coords) >= len(group) * 12, (len(coords), len(group)))
			i = 0
			for p, t in zip(group, times):
				age = p.age % times[-1]
				c = 0
				while c < 2 and age > times[c]:
					c += 1
				self.assertEqual(coords[i:i+12], coord_sets[c], "f=%s i=%s c=%s age=%s: %s != %s" % 
					(f, i, c, age, coords[i:i+12], coord_sets[c]))
				i += 12
			group.update(0.04)

	def test_3D_single_duration_no_loop(self):
		from lepton.texturizer import FlipBookTexturizer
		coord_sets = [
			(0,0,0, 1,0,0, 1,1,0, 0,1,0),
			(0,0,0.5, 1,0,0.5, 1,1,0.5, 0,1,0.5),
			(0,0,1, 1,0,1, 1,1,1, 0,1,1),
			]
		fbtex = FlipBookTexturizer(0,
			coords=coord_sets,
			duration=0.12,
			dimension=3,
			loop=False,
			)
		self.assertEqual(fbtex.tex_dimension, 3)
		self.assertAlmostEqual(fbtex.duration, 0.12)
		self.failIf(fbtex.loop)
		group = self._make_group(10)
		age = 0.0
		for p in group:
			p.age = age % 0.4
			age += 0.07
		times = [0.12, 0.24, 0.36]
		for f in range(5):
			coords = tuple(fbtex.generate_tex_coords(group))
			self.failUnless(len(coords) >= len(group) * 12, (len(coords), len(group)))
			i = 0
			for p, t in zip(group, times):
				c = 0
				while c < 2 and p.age > times[c]:
					c += 1
				self.assertEqual(coords[i:i+12], coord_sets[c], "f=%s i=%s c=%s age=%s: %s != %s" % 
					(f, i, c, p.age, coords[i:i+12], coord_sets[c]))
				i += 12
			group.update(0.055)

	def test_3D_duration_list_loop(self):
		from lepton.texturizer import FlipBookTexturizer
		coord_sets = [
			(0,0,0, 1,0,0, 1,1,0, 0,1,0),
			(0,0,0.5, 1,0,0.5, 1,1,0.5, 0,1,0.5),
			(0,0,1, 1,0,1, 1,1,1, 0,1,1),
			]
		durations = [0.7, 0.3, 0.5]
		times = []
		t = 0
		for d in durations:
			t += d
			times.append(t)
		fbtex = FlipBookTexturizer(0,
			coords=coord_sets,
			duration=durations,
			dimension=3,
			)
		self.assertEqual(fbtex.tex_dimension, 3)
		self.failUnless(fbtex.loop)
		for d, expected in zip(fbtex.duration, durations):
			self.assertAlmostEqual(d, expected, 6)
		group = self._make_group(10)
		age = 0.0
		for p in group:
			p.age = age % 0.4
			age += 0.07
		for f in range(5):
			coords = tuple(fbtex.generate_tex_coords(group))
			self.failUnless(len(coords) >= len(group) * 12, (len(coords), len(group)))
			i = 0
			for p, t in zip(group, times):
				age = p.age % times[-1]
				c = 0
				while c < 2 and age > times[c]:
					c += 1
				self.assertEqual(coords[i:i+12], coord_sets[c], "f=%s i=%s c=%s age=%s: %s != %s" % 
					(f, i, c, age, coords[i:i+12], coord_sets[c]))
				i += 12
			group.update(0.11)

	def test_3D_duration_list_no_loop(self):
		from lepton.texturizer import FlipBookTexturizer
		coord_sets = [
			(0,0,0, 1,0,0, 1,1,0, 0,1,0),
			(0,0,0.5, 1,0,0.5, 1,1,0.5, 0,1,0.5),
			(0,0,1, 1,0,1, 1,1,1, 0,1,1),
			]
		durations = [0.4, 0.4, 0.5]
		times = []
		t = 0
		for d in durations:
			t += d
			times.append(t)
		fbtex = FlipBookTexturizer(0,
			coords=coord_sets,
			duration=durations,
			dimension=3,
			loop=False,
			)
		self.assertEqual(fbtex.tex_dimension, 3)
		self.failIf(fbtex.loop)
		for d, expected in zip(fbtex.duration, durations):
			self.assertAlmostEqual(d, expected, 6)
		group = self._make_group(10)
		age = 0.0
		for p in group:
			p.age = age % 0.5
			age += 0.07
		for f in range(5):
			coords = tuple(fbtex.generate_tex_coords(group))
			self.failUnless(len(coords) >= len(group) * 12, (len(coords), len(group)))
			i = 0
			for p, t in zip(group, times):
				c = 0
				while c < 2 and p.age > times[c]:
					c += 1
				self.assertEqual(coords[i:i+12], coord_sets[c], "f=%s i=%s c=%s age=%s: %s != %s" % 
					(f, i, c, p.age, coords[i:i+12], coord_sets[c]))
				i += 12
			group.update(0.17)

	def test_invalid_args(self):
		from lepton.texturizer import FlipBookTexturizer
		self.assertRaises(TypeError, FlipBookTexturizer, 0, object(), 1)
		self.assertRaises(TypeError, FlipBookTexturizer, 0, [(0,0,0,0,0,0,0,0)], object())
		self.assertRaises(ValueError, FlipBookTexturizer, 0, [], 1)
		self.assertRaises(ValueError, FlipBookTexturizer, 0, [(0,0)], 1)
		self.assertRaises(ValueError, FlipBookTexturizer, 0, [(0,0,0,0,0,0,0,0)], 0)
		self.assertRaises(ValueError, FlipBookTexturizer, 0, [(0,0,0,0,0,0,0,0)], -1)
		self.assertRaises(ValueError, FlipBookTexturizer, 0, [(0,0,0,0,0,0,0,0)], [])
		self.assertRaises(ValueError, 
			FlipBookTexturizer, 0, [(0,0,0,0,0,0,0,0), (0,0,0,0,0,0,0,0)], [1,-1])
		self.assertRaises(ValueError, 
			FlipBookTexturizer, 0, [(0,0,0,0,0,0,0,0), (0,0,0,0,0,0,0,0)], [1,1], dimension=0)
		self.assertRaises(ValueError, 
			FlipBookTexturizer, 0, [(0,0,0,0,0,0,0,0), (0,0,0,0,0,0,0,0)], [1,1], dimension=4)

	if pyglet is not None:
		def _glGet(self, what):
			result = (ctypes.c_int * 1)()
			glGetIntegerv(what, result)
			return result[0]

		def test_2D_set_state_restore_state(self):
			from lepton.texturizer import FlipBookTexturizer
			texture = (ctypes.c_ulong * 1)()
			glGenTextures(1, texture)
			glDisable(GL_TEXTURE_2D)
			glDisable(GL_TEXTURE_3D)
			glBindTexture(GL_TEXTURE_2D, 0)
			sprite_tex = FlipBookTexturizer(texture[0], [(0,0,0,0,0,0,0,0)], 1)
			self.assertEqual(sprite_tex.tex_dimension, 2)
			self.failIf(self._glGet(GL_TEXTURE_2D))
			self.failIf(self._glGet(GL_TEXTURE_3D))
			self.assertEqual(self._glGet(GL_TEXTURE_BINDING_2D), 0)
			sprite_tex.set_state()
			self.failUnless(self._glGet(GL_TEXTURE_2D))
			self.failIf(self._glGet(GL_TEXTURE_3D))
			self.assertEqual(self._glGet(GL_TEXTURE_BINDING_2D), texture[0])
			sprite_tex.restore_state()
			self.failIf(self._glGet(GL_TEXTURE_2D))
			self.failIf(self._glGet(GL_TEXTURE_3D))

		def test_3D_set_state_restore_state(self):
			from lepton.texturizer import FlipBookTexturizer
			texture = (ctypes.c_ulong * 1)()
			glGenTextures(1, texture)
			glDisable(GL_TEXTURE_2D)
			glDisable(GL_TEXTURE_3D)
			glBindTexture(GL_TEXTURE_3D, 0)
			sprite_tex = FlipBookTexturizer(texture[0], [(0,0,0,0,0,0,0,0)], 1, dimension=3)
			self.assertEqual(sprite_tex.tex_dimension, 3)
			self.failIf(self._glGet(GL_TEXTURE_2D))
			self.failIf(self._glGet(GL_TEXTURE_3D))
			self.assertEqual(self._glGet(GL_TEXTURE_BINDING_3D), 0)
			sprite_tex.set_state()
			self.failUnless(self._glGet(GL_TEXTURE_3D))
			self.failIf(self._glGet(GL_TEXTURE_2D))
			self.assertEqual(self._glGet(GL_TEXTURE_BINDING_3D), texture[0])
			sprite_tex.restore_state()
			self.failIf(self._glGet(GL_TEXTURE_2D))
			self.failIf(self._glGet(GL_TEXTURE_3D))



if __name__ == '__main__':
	unittest.main()
