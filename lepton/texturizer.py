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
from _texturizer import SpriteTexturizer

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

def SpriteTexturizerFromImages(images, weights=None, filter=None, wrap=None):
	"""Create a SpriteTexturizer from a sequence of Pyglet images. 
	
	Note all the images must be able to fit into a single OpenGL texture, so
	their combined size should typically be less than 1024x1024
	"""
	import pyglet
	images = sorted(images, key=lambda img: img.height, reverse=True)
	widest = max(img.width for img in images)
	height = sum(img.height for img in images)

	atlas = pyglet.image.atlas.TextureAtlas(
		width=_nearest_pow2(widest), height=_nearest_pow2(height))
	textures = [atlas.add(image) for image in images]
	texturizer = SpriteTexturizer(
		atlas.texture.id, [tex.tex_coords for tex in textures],
		weights, filter or pyglet.gl.GL_LINEAR, wrap or pyglet.gl.GL_CLAMP)
	texturizer.atlas = atlas
	texturizer.textures = textures
	return texturizer

