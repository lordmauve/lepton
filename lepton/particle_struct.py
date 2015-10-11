#
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
#
"""particle class

Particles are basically just structs that store the
particle state. All particle behavior is done via controllers
and all particle presentation is done via renderers
"""

__version__ = '$Id$'

import ctypes
from math import sqrt


class Vec3(ctypes.Array):

    """Simple 3D vector type"""

    _length_ = 3
    _type_ = ctypes.c_float

    def __getattr__(self, name):
        return self['xyz'.index(name)]

    def __setattr__(self, name, value):
        self['xyz'.index(name)] = value

    def __repr__(self):
        return '%s(%s, %s, %s)' % (self.__class__.__name__, self.x, self.y, self.z)

    # Vector math methods to be moved to C ##
    def length(self):
        x, y, z = self
        return sqrt(x * x + y * y + z * z)

    def normalize(self):
        L = self.length()
        if L != 0:
            x, y, z = self
            return self.__class__(x / L, y / L, z / L)
        else:
            return self.__class__(0, 0, 0)

    def __nonempty__(v):
        (x, y, z) = v
        return x != 0 or y != 0 or z != 0

    def __lt__(self, o):
        (sx, sy, sz) = self
        (ox, oy, oz) = o
        return (sx ** 2 + sy ** 2 + sz ** 2) < (ox ** 2 + oy ** 2 + oz ** 2)

    def __gt__(self, other):
        sx, sy, sz = self
        ox, oy, oz = other
        return (sx ** 2 + sy ** 2 + sz ** 2) > (ox ** 2 + oy ** 2 + oz ** 2)

    def dot(self, other):
        sx, sy, sz = self
        ox, oy, oz = other
        return sx * ox + sy * oy + sz * oz

    def cross(self, other):
        sx, sy, sz = self
        ox, oy, oz = other
        return self.__class__(sy * oz - sz * oy, sz * ox - sx * oz, sx * oy - sy * ox)

    def __div__(self, scalar):
        sx, sy, sz = self
        return self.__class__(sx / scalar, sy / scalar, sz / scalar)

    def __add__(self, other):
        sx, sy, sz = self
        ox, oy, oz = other
        return self.__class__(sx + ox, sy + oy, sz + oz)

    def __sub__(self, other):
        sx, sy, sz = self
        ox, oy, oz = other
        return self.__class__(sx - ox, sy - oy, sz - oz)

    def __mul__(self, scalar):
        sx, sy, sz = self
        return self.__class__(sx * scalar, sy * scalar, sz * scalar)

    def __neg__(self):
        x, y, z = self
        return self.__class__(-x, -y, -z)

    def reflect(self, normal):
        sx, sy, sz = self
        nx, ny, nz = normal
        d = self.dot(normal) * 2
        return self.__class__(sx - d * nx, sy - d * ny, sz - d * nz)


class Color(ctypes.Array):
    """Simple rgba color type"""
    _length_ = 4
    _type_ = ctypes.c_float

    def __getattr__(self, name):
        return self['rgba'.index(name)]

    def __setattr__(self, name, value):
        self['rgba'.index(name)] = value

    def __repr__(self):
        return '%s(%s, %s, %s, %s)' % (self.__class__.__name__,
                                       self.r, self.g, self.b, self.a)


class Particle(ctypes.Structure):

    """Particle state"""
    _fields_ = [
        ('position', Vec3),  # Current particle position
        ('velocity', Vec3),  # Current particle velocity
        ('color', Color),  # Particle color
        ('mass', ctypes.c_float),  # Particle mass
        ('age', ctypes.c_float),  # Current age of particle
        ('size', Vec3),  # Particle width, height and depth
        ('up', Vec3),  # Particle orientation vector
        ('rotation', Vec3),  # Rotation speed of up vector
        ('last_position', Vec3),  # Previous particle position
        ('last_velocity', Vec3),  # Previous particle velocity
    ]

    def __repr__(self):
        s = ['<%s %x' % (self.__class__.__name__, id(self))]
        for name, typ in self._fields_:
            value = getattr(self, name)
            if typ is Vec3:
                vstr = '(%.2f, %.2f, %.2f)' % (value.x, value.y, value.z)
            elif typ is Color:
                vstr = '(%.3f, %.3f, %.3f, %.3f)' % (value.r, value.g, value.b, value.a)
            else:
                vstr = str(value)
            s.append(' %s=%s' % (name, vstr))
        s.append('>')
        return ''.join(s)

    def __copy__(self):
        return self.__class__(
            position=self.position,
            velocity=self.velocity,
            last_velocity=self.last_velocity,
            last_position=self.last_position,
            size=self.size,
            up=self.up,
            rotation=self.rotation,
            color=self.color,
            mass=self.mass,
            age=self.age)

    def __hash__(self):
        return id(self)
