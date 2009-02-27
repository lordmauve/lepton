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
""" Magnet.py

    Demos the magnet controller. Electrons orbit protons.


"""

__version__ = '$Id: magnet.py 104 2008-11-08 06:49:41Z andrew.charles $'

from pyglet import image
from pyglet.gl import *

import os, math

from lepton import Particle, ParticleGroup, default_system
from lepton.renderer import PointRenderer, BillboardRenderer
from lepton.emitter import StaticEmitter, PerParticleEmitter
from lepton.controller import Movement, Bounce, Magnet, Lifetime,Fader, ColorBlender
from lepton.domain import AABox, Sphere
from random import expovariate, uniform, gauss

if __name__ == '__main__':
    win = pyglet.window.Window(resizable=True, visible=False)
    win.clear()
    glEnable(GL_BLEND)
    glEnable(GL_POINT_SMOOTH)
    glShadeModel(GL_SMOOTH)# Enables Smooth Shading
    glBlendFunc(GL_SRC_ALPHA,GL_ONE)#Type Of Blending To Perform
    glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);#Really Nice Point Smoothing
    glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
    glDisable(GL_DEPTH_TEST)

    def resize(widthWindow, heightWindow):
        """Initial settings for the OpenGL state machine, clear color, window size, etc"""
        glViewport(0, 0, widthWindow, heightWindow)
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluPerspective(70, 1.0*widthWindow/heightWindow, 0.001, 10000.0)
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()

    win.on_resize = resize

    electron_count = 1 
    electron_size = 25
    proton_size =0.1 
    n_protons = 2

    electron_lifetime = 22 
    trail_lifetime = 4.5 

    screen_domain = AABox( (-100.0,-100.0,-100.0), 
    (100.0,100.0,100.0))

    protons=[]
    spheres=[]
    
    # Attractive attractor    
    sphere = Sphere((0.0,0.0,0.0),proton_size)
    proton = Magnet(sphere, charge=2500.0, inner_cutoff=20.0)
    proton.color = (0.1,0.7,0.1)
    protons.append(proton)
    spheres.append(sphere)

    default_system.add_global_controller(
        Movement(max_velocity=100), 
        Bounce(screen_domain,bounce=0.5,friction=0.1),
        *protons
    )

    electrons = ParticleGroup(renderer=BillboardRenderer(),
            controllers=[Lifetime(electron_lifetime * 1.5)]
		)
       
    # Trails for electrons 
    electron_emitter = StaticEmitter(
        template=Particle(
        #position=screen_domain,
            position=(-15.0,-25.0,-0.0),
            size=(electron_size,electron_size,electron_size),
            rate=1,
			age=electron_lifetime*0.75,
            velocity=(30.0,0.0,0.0)
        ),
        deviation=Particle(
        position=(1,1,1), 
        velocity=(3,1,1), 
        color=(0.0,0.0,0.0,0.0), 
        ),
        color=[(0.1,1.0,0.1,1.0)]

    )

    trail_emitter = PerParticleEmitter(electrons,rate=uniform(5,15),
            template=Particle(
                color=(1.0,0.0,0.0 , 1),
                size=(5,5,5)),
                deviation=Particle(
                up=(0,0,math.pi),
                rotation=(0,0,math.pi),
                velocity=(1,1,1),
                size=(1.5,1.5,1.5),
                color=(0.00,0.0,1.0,0.01),
                age=trail_lifetime))

    trails = ParticleGroup(
            controllers=[
                Lifetime(trail_lifetime * 1.5),
                Movement(damping=0.7),
                Fader(max_alpha=0.9,fade_out_start=0, fade_out_end=gauss(trail_lifetime, trail_lifetime*0.5)),
                trail_emitter],
             renderer=BillboardRenderer()
             )

    texture = image.load(os.path.join(os.path.dirname(__file__),'flare3.png')).texture
    glEnable(texture.target)
    glTexParameteri(texture.target, GL_TEXTURE_WRAP_S, GL_CLAMP)
    glTexParameteri(texture.target, GL_TEXTURE_WRAP_T, GL_CLAMP)
    glTexParameteri(texture.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glTexParameteri(texture.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
    glBindTexture(texture.target,texture.id)
            
    win.set_visible(True)
    pyglet.clock.schedule_interval(default_system.update, (1.0/30.0))

    def draw_protons():
        glPointSize(protons[0].domain.radius)
        glBegin(GL_POINTS)
        for proton in protons:
            cx, cy, cz = proton.domain.center
            glColor3f(*proton.color)
            glVertex3f(cx, cy, cz)
        glEnd()

    yrot = 0.0
    xrot = 0.0

    @win.event
    def on_mouse_motion(x, y, dx, dy):
        global yrot, xrot
        yrot += dx * 0.3
        xrot -= dy * 0.3
	
    def emit(dt):
        electron_emitter.emit(electron_count,electrons)
        electrons.update(0)

    def summon(dt=None):
        emit(dt)
        pyglet.clock.schedule_once(summon,2.0)

    summon()

    @win.event
    def on_draw():
        global i
        global yrot,xrot
        win.clear()
        glLoadIdentity()
        glTranslatef(0, 0, -75)
        glRotatef(yrot, 0.0, 1.0, 0.0)
        glRotatef(xrot, 1.0, 0.0, 0.0)
        glEnable(texture.target)
        glTexParameteri(texture.target, GL_TEXTURE_WRAP_S, GL_CLAMP)
        glTexParameteri(texture.target, GL_TEXTURE_WRAP_T, GL_CLAMP)
        glTexParameteri(texture.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        glTexParameteri(texture.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
        glBindTexture(texture.target,texture.id)
        trails.draw()
        electrons.draw()
        glDisable(texture.target)

    pyglet.app.run()

