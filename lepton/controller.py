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
"""Particle controllers

particle controllers are bound to particle groups and invoked
via the group's update() method. A controller is a callable
object that accepts a timedelta and group as its argument.

A typical controller will iterate all particles in a group,
apply some logic and mutate the particles or remove or add
particles to the group.
"""

__version__ = '$Id$'

from math import sqrt
from particle_struct import Color, Vec3
from _controller import Gravity, Fader, Movement, Lifetime, ColorBlender, Growth, Collector, \
	Bounce, Magnet, Drag
import sys


def NoopController(time_delta, group):
    """Do nothing controller"""


class Clumper(object):
    """EXPERMENTAL: SUBJECT TO CHANGE
    
    Clumps objects in a group together or keeps them apart
    
    The center of the group is calculated by averaging all of the
    particle positions, and all particles are accelerated toward
    (or away) from this center point.
    """

    def __init__(self, magnitude):
        """
        magnitude -- The acceleration magnitude toward the
        group center. If negative, the acceleration is
        away from the center.
        """
        self.magnitude = magnitude
    
    def __call__(self, td, group):
        if group:
            avgx = 0
            avgy = 0
            avgz = 0

            for p in group:
                px, py, pz = p.position
                avgx += px
                avgy += py
                avgz += pz

            avgx /= len(group)
            avgy /= len(group)
            avgz /= len(group)

            mag = self.magnitude * td

            for p in group:
                # calculate the position looking ahead one frame
                px, py, pz = p.position
                px += p.velocity.x
                py += p.velocity.y
                pz += p.velocity.z
                # calculate the acceleration vector
                dx = avgx - px
                dy = avgy - py
                dz = avgz - pz
                dmag = sqrt(dx**2 + dy**2 + dz**2)
                p.velocity.x += (dx / dmag) * mag
                p.velocity.y += (dy / dmag) * mag
                p.velocity.z += (dz / dmag) * mag

