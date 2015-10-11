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
"""Particle renderer texturizers

Texturizers generate texture coordinates for particles and perform
the necessary OpenGL state changes to setup texturing for rendering
"""

import math
import ctypes
from . import _texturizer

def _nearest_pow2(v):
    # From http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    # Credit: Sean Anderson
    v -= 1
    v |= v >> 1
    v |= v >> 2
    v |= v >> 4
    v |= v >> 8
    v |= v >> 16
    return v + 1

def _atlas_from_images(images):
	"""Create a pyglet texture atlas from a sequence of images.
	Return a tuple of (atlas, textures)
	"""
	import pyglet
	widest = max(img.width for img in images)
	height = sum(img.height for img in images)

	atlas = pyglet.image.atlas.TextureAtlas(
		width=_nearest_pow2(widest), height=_nearest_pow2(height))
	textures = [atlas.add(image) for image in images]
	return atlas, textures


class SpriteTexturizer(_texturizer.SpriteTexturizer):
	__doc__ = _texturizer.SpriteTexturizer.__doc__

	@classmethod
	def from_images(cls, images, weights=None, filter=None, wrap=None,
		aspect_adjust_width=False, aspect_adjust_height=False):
		"""Create a SpriteTexturizer from a sequence of Pyglet images.

		Note all the images must be able to fit into a single OpenGL texture, so
		their combined size should typically be less than 1024x1024
		"""
		import pyglet
		atlas, textures = _atlas_from_images(images)
		texturizer = cls(
			atlas.texture.id, [tex.tex_coords for tex in textures],
			weights, filter or pyglet.gl.GL_LINEAR, wrap or pyglet.gl.GL_CLAMP,
			aspect_adjust_width, aspect_adjust_height)
		texturizer.atlas = atlas
		texturizer.textures = textures
		return texturizer


class FlipBookTexturizer(_texturizer.FlipBookTexturizer):
	__doc__ = _texturizer.FlipBookTexturizer.__doc__

	@classmethod
	def from_images(cls, images, duration, loop=True, dimension=2, filter=None, wrap=None,
		aspect_adjust_width=False, aspect_adjust_height=False):
		"""Create a FlipBookTexturizer from a sequence of Pyglet images

		Note all the images must be able to fit into a single OpenGL texture, so
		their combined size should typically be less than 1024x1024
		"""
		import pyglet
		atlas, textures = _atlas_from_images(images)
		texturizer = cls(
			atlas.texture.id, [tex.tex_coords for tex in textures],
			duration, loop, dimension,
			filter or pyglet.gl.GL_LINEAR, wrap or pyglet.gl.GL_CLAMP,
			aspect_adjust_width, aspect_adjust_height)
		texturizer.atlas = atlas
		texturizer.textures = textures
		return texturizer


def create_point_texture(size, feather=0):
	"""Create and load a circular grayscale image centered in a square texture
	with a width and height of size. The radius of the circle is size / 2.
	Since size is used as the texture width and height, it should typically
	be a power of two.

	Feather determines the softness of the edge of the circle. The default,
	zero, creates a hard edged circle. Larger feather values create softer
	edges for blending. The point at the center of the texture is always
	white.

	Return the OpenGL texture name (id) for the resulting texture. This
	value can be passed directy to a texturizer or glBindTexture
	"""
	from pyglet import gl

	assert feather >= 0, 'Expected feather value >= 0'
	coords = range(size)
	texel = (gl.GLfloat * size**2)()
	r = size / 2.0
	c = feather + 1.0

	for y in coords:
		col = y * size
		for x in coords:
			d = math.sqrt((x - r)**2 + (y - r)**2)
			if d < r and (1.0 - 1.0 / (d / r - 1.0)) < 100:
				texel[x + col] = c**2 / c**(1.0 - 1.0 / (d / r - 1.0))
			else:
				texel[x + col] = 0
	id = gl.GLuint()
	gl.glGenTextures(1, ctypes.byref(id))
	gl.glBindTexture(gl.GL_TEXTURE_2D, id.value)
	gl.glPixelStorei(gl.GL_UNPACK_ALIGNMENT, 4)
	gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, gl.GL_LUMINANCE, size, size, 0,
		gl.GL_LUMINANCE, gl.GL_FLOAT, ctypes.byref(texel))
	gl.glFlush()
	return id.value
