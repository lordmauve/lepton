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
"""Bouncy Bouncy

Demos how to use domains to position particles and redirect them
using the Bounce controller.
"""

__version__ = '$Id$'

from pyglet import image
from pyglet.gl import *

from lepton import Particle, ParticleGroup, default_system
from lepton.renderer import PointRenderer
from lepton.emitter import StaticEmitter
from lepton.controller import Movement, Bounce, Gravity, Drag
from lepton.domain import AABox, Sphere

class Bumper:
	color = (1, 0, 0)

	def __init__(self, position, radius):
		self.domain = Sphere(position, radius)
		self.controller = Bounce(
			self.domain, bounce=1.5, friction=-0.25, callback=self.set_bumper_color)

	def set_bumper_color(self, particle, group, bumper, collision_point, collision_normal):
		"""Set bumper color to the color of the particle that collided with it"""
		self.color = tuple(particle.color)[:3]


win = pyglet.window.Window(resizable=True, visible=False)
win.clear()

def resize(widthWindow, heightWindow):
	"""Initial settings for the OpenGL state machine, clear color, window size, etc"""
	glEnable(GL_BLEND)
	glEnable(GL_POINT_SMOOTH)
	glShadeModel(GL_SMOOTH)# Enables Smooth Shading
	glBlendFunc(GL_SRC_ALPHA,GL_ONE)#Type Of Blending To Perform
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);#Really Nice Perspective Calculations
	glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);#Really Nice Point Smoothing
	glDisable(GL_DEPTH_TEST)

ball_count = 100
ball_size = 15
bumper_count = 8

# Screen domain is a box the size of the screen
screen_domain = AABox((ball_size/2.0, ball_size/2.0, 0), 
	(win.width-ball_size/2.0,win.height-ball_size/2.0,0))

bumpers = []
for i in range(bumper_count):
	bumper = Bumper(
		(win.width/(bumper_count-1) * i, win.height*2.0/3.0 - (i % 2) * win.height/3, 0), 
		win.height / 15)
	bumpers.append(bumper)

up_fan = AABox((win.width/2 - win.width/12, 0, -1), (win.width/2 + win.width/12, win.height * 0.8, 1))
left_fan = AABox((win.width/2 - win.width/12, win.height * 0.8, -1), (win.width/2, win.height, 1))
right_fan = AABox((win.width/2, win.height * 0.8, -1), (win.width/2 + win.width/12, win.height, 1))

default_system.add_global_controller(
	Gravity((0,-50,0)),
	Movement(max_velocity=250), 
	Drag(0.0, 0.0001, (0, 800, 0), domain=up_fan),
	Drag(0.0, 0.0001, (-200, 400, 0), domain=left_fan),
	Drag(0.0, 0.0001, (200, 400, 0), domain=right_fan),
	*[bumper.controller for bumper in bumpers]
)
# Make the bounce controller for the screen boundary run last 
# to ensure no particles can "escape"
default_system.add_global_controller(
	Bounce(screen_domain, friction=0.01)
)
group = ParticleGroup(renderer=PointRenderer(point_size=ball_size))

ball_emitter = StaticEmitter(
	position=screen_domain,
	deviation=Particle(velocity=(60,60,0), color=(0.3,0.3,0.3,0)),
	color=[(1,0,0,1), (0,1,0,1), (0,0,1,1), (1,1,0,1), (0,1,1,1), (1,1,1,1)],
	mass=[1],
)
ball_emitter.emit(ball_count, group)
group.update(0)
# Kill particles inside the bumpers
for p in group:
	for bumper in bumpers:
		if p.position in bumper.domain:
			group.kill(p)

win.resize = resize
win.set_visible(True)
win.resize(win.width, win.height)
pyglet.clock.schedule_interval(default_system.update, 1.0/30.0)
#pyglet.clock.schedule_interval(lambda x: default_system.update(0.05), (1.0/5.0))

def draw_bumpers():
	glPointSize(bumpers[0].domain.radius * 2 - ball_size/2.0 - 15)
	glColor3f(1.0, 1.0, 0)
	glBegin(GL_POINTS)
	for bumper in bumpers:
		cx, cy, cz = bumper.domain.center
		glVertex3f(cx, cy, cz)
	glEnd()
	glPointSize(bumpers[0].domain.radius * 2 - ball_size/2.0)
	glBegin(GL_POINTS)
	for bumper in bumpers:
		cx, cy, cz = bumper.domain.center
		glColor3f(*bumper.color)
		glVertex3f(cx, cy, cz)
	glEnd()

@win.event
def on_draw():
	win.clear()
	glLoadIdentity()
	draw_bumpers()
	default_system.draw()

if __name__ == '__main__':
	pyglet.app.run()
