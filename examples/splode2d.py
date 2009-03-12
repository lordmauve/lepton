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
"""2D Explosion using textured billboard quads

This example is designed to illustrate how to create 2D effects
using the default pyglet projection. Compare this code to the
3D splode.py
"""

__version__ = '$Id$'

import os
import math
import time
from pyglet import image
from pyglet.gl import *

from lepton import Particle, ParticleGroup, default_system
from lepton.renderer import BillboardRenderer
from lepton.emitter import StaticEmitter
from lepton.controller import Gravity, Lifetime, Movement, Fader, ColorBlender, Growth

if __name__ == '__main__':
	win = pyglet.window.Window(resizable=True, visible=False)
	win.clear()

	glEnable(GL_BLEND)
	glShadeModel(GL_SMOOTH)
	glBlendFunc(GL_SRC_ALPHA,GL_ONE)
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	glDisable(GL_DEPTH_TEST)

	sparks = ParticleGroup(
		controllers=[
			Lifetime(3),
			Movement(damping=0.93),
			Fader(fade_out_start=0.75, fade_out_end=3.0),
		],
		renderer=BillboardRenderer())
	
	spark_emitter = StaticEmitter(
		template=Particle(
			position=(win.width/2,win.height/2,0), 
			color=(1,1,1)), 
		deviation=Particle(
			position=(1,1,0), 
			velocity=(300,300,0), 
			age=1.5),
		size=[(3,3,0), (4,4,0), (5,5,0), (5,5,0), (6,6,0), (7,7,0)])
	spark_emitter.emit(400, sparks)
	
	spark_tex = image.load(os.path.join(os.path.dirname(__file__), 'flare3.png')).texture
	glEnable(spark_tex.target)
	glTexParameteri(spark_tex.target, GL_TEXTURE_WRAP_S, GL_CLAMP)
	glTexParameteri(spark_tex.target, GL_TEXTURE_WRAP_T, GL_CLAMP)
	glTexParameteri(spark_tex.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
	glTexParameteri(spark_tex.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR)

	fire = ParticleGroup(
		controllers=[
			Lifetime(4),
			Movement(damping=0.95),
			Growth(30),
			Fader(fade_in_start=0, start_alpha=0, fade_in_end=0.5, max_alpha=0.4, 
				fade_out_start=1.0, fade_out_end=4.0)
		],
		renderer=BillboardRenderer())
	fire_emitter = StaticEmitter(
		template=Particle(
			position=(win.width/2,win.height/2,0), 
			size=(20,20,0)),
		deviation=Particle(
			position=(2,2,0), 
			velocity=(70,70,0), 
			size=(10,10,0),
			up=(0,0,math.pi*2), 
			rotation=(0,0,math.pi*0.06),
			age=2,),
		color=[(0.5,0,0), (0.5,0.5,0.5), (0.4,0.1,0.1), (0.85,0.3,0)],
	)
	fire_emitter.emit(400, fire)

	fire_tex = image.load(os.path.join(os.path.dirname(__file__), 'puff.png')).texture
	glEnable(fire_tex.target)
	glTexParameteri(fire_tex.target, GL_TEXTURE_WRAP_S, GL_CLAMP)
	glTexParameteri(fire_tex.target, GL_TEXTURE_WRAP_T, GL_CLAMP)
	glTexParameteri(fire_tex.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
	glTexParameteri(fire_tex.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR)


	win.set_visible(True)
	time.sleep(0.5)
	pyglet.clock.schedule_interval(default_system.update, (1.0/30.0))
	pyglet.clock.set_fps_limit(None)

	@win.event
	def on_draw():
		win.clear()
		glLoadIdentity()
		glBindTexture(spark_tex.target, spark_tex.id)
		sparks.draw()
		glBindTexture(fire_tex.target, fire_tex.id)
		fire.draw()

	pyglet.app.run()
