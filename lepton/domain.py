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
"""Domains represent regions of space and are used for generating vectors
(positions, velocities, colors). Domains are also used by controllers to test
for collision.  Colliding with domains can then influence particle
behavior
"""

__version__ = '$Id$'

from .particle_struct import Vec3
from ._domain import Line, Plane, AABox, Sphere, Disc, Cylinder, Cone


class Domain(object):
    """Domain abstract base class"""

    def generate(self):
        """Return a point within the domain as a 3-tuple. For domains with a
        non-zero volume, 'point in domain' is guaranteed to return true.
        """
        raise NotImplementedError

    def __contains__(self, point):
        """Return true if point is inside the domain, false if not."""
        raise NotImplementedError

    def closest_point_to(self, point):
        """Return the closest point in the domain to the given point
        and the surface normal vector at that point. If the given
        point is in the domain, return the point and a null normal
        vector.

        Note the closest point may not in the domain, if there are
        multiple points in the domain that are closest. In that case
        return the average of the closest points and a null normal
        vector.
        """
        raise NotImplementedError

    def intersect(self, start_point, end_point):
        """For the line segment defined by the start and end point specified
        (coordinate 3-tuples), return the point closest to the start point
        where the line segment intersects surface of the domain, and the
        surface normal unit vector at that point as a 2-tuple.  If the line
        segment does not intersect the domain, return the 2-tuple (None,
        None).

        Only 2 or 3 dimensional domains may be intersected.

        Note performance is more important than absolute accuracy with this
        method, so approximations are acceptable.
        """
        raise NotImplementedError


EPSILON = 0.00001


class Point(object):
    """Simple single point domain"""

    def __init__(self, point):
        self.point = Vec3(*point)

    def __contains__(self, point):
        x, y, z = point
        cx, cy, cz = self.point
        return (x - cx) ** 2 + (y - cy) ** 2 + (z - cz) ** 2 < EPSILON

    def generate(self):
        """Generate always returns the domain point"""
        return tuple(self.point)

    def closest_point_to(self, point):
        """Return the closest point in the domain to the given point
        and a normal vector at that point.

        The point returned is always the domain point.

        The normal returned for a point domain is a unit vector parallel to
        the line formed between the supplied point and the domain point,
        facing outward from the domain. This effectively treats the point
        domain like a zero-radius sphere.
        """
        return self.point, -(self.point - point).normalize()

    def intersect(self, start_point, end_point):
        """You cannot intersect a point domain"""
        return None, None


def Box(*args, **kw):
    """Axis-aligned box domain (same as AABox for now)

    WARNING: Deprecated, use AABox instead. This domain will mean something
    different in future versions of lepton
    """
    import warnings
    warnings.warn("lepton.domain.Box is deprecated, use AABox instead. "
                  "This domain class will mean something different in future versions of lepton",
                  stacklevel=2)
    return AABox(*args, **kw)
