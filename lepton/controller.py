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
from _controller import Gravity, Fader, Movement, Lifetime, ColorBlender, Growth, Collector, Bounce
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
        
 
class Magnet(object):
    """ Controller that attracts/repels particles with an inverse distance force.

        Acceleration of affected particles is computed as 

                dv/dt = charge * 1/distance^exponent

        and directed towards the nearest point of the magnetic domain.

    """

    def __init__(self, domain, charge=5000.0, exponent=2.0, inner_cutoff=0.0, outer_cutoff=1000000.0):
        """
        domain -- the shape of the domain does not matter as attraction
        is computed with respect to the domain centre.

        charge -- the strength of the force. 5,000 is a nice value.
        
        exponent -- exponent.

        inner_cutoff -- no force is exerted on particles closer to the domain
        than this. Good for avoiding unstable large forces at close distance.

        outer_cutoff -- no force is extered on particles further from the 
        domain than this.

        """
        self.domain = domain
        self.charge = float(charge)
        self.exponent = float(exponent) 
        self.inner_cutoff = float(inner_cutoff)
        self.outer_cutoff = float(outer_cutoff)

    def __call__(self, td, group):
        domain = self.domain
        k = self.charge
        a = self.exponent
        for p in group:
            px, py, pz = p.position
            attract_point = domain.closest_point_to(p.position)
            if None in attract_point:
                # Required?
                print "No closest point"
                return
            else:
                cx, cy, cz = attract_point[0]
          
            # Compute distance
            dx, dy, dz = cx - px, cy - py, cz - pz
            rsq = dx**2 + dy**2 + dz**2
            r = sqrt(rsq)

            if (r <= self.inner_cutoff) or (r > self.outer_cutoff):
                # no force applied
                # .le. needed to deal with case r==0
                pass
            else:
                # Compute magnitude of acceleration 
                # and resolve to components.
                mag = k/(r**a)
                dv = Vec3(mag*dx/r,mag*dy/r,mag*dz/r)
                p.velocity = Vec3(*p.velocity)+dv
