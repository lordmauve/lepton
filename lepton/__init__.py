"""Lepton particle engine"""

__version__ = "0.7a"

from system import ParticleSystem
from group import ParticleGroup
from particle_struct import Particle

# Init default particle system for convenience
# groups are added to this system by default
default_system = ParticleSystem()
