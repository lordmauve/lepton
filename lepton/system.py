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
"""Particle system classes"""

__version__ = '$Id$'

class ParticleSystem(object):

	def __init__(self, global_controllers=()):
		"""Initialize the particle system, adding the specified global
		controllers and renderers, if any
		"""
		# Tuples are used for global controllers and renders to prevent
		# unpleasant side-affects if they are added during update or draw
		self.controllers = tuple(controller for controller in global_controllers)
		self.groups = []

	def add_global_controller(self, *controllers):
		"""Add a global controller applied to all groups on update"""
		self.controllers += controllers

	def add_group(self, group):
		"""Add a particle group to the system"""
		self.groups.append(group)
	
	def remove_group(self, group):
		"""Remove a particle group from the system, raise ValueError
		if the group is not in the system
		"""
		self.groups.remove(group)
	
	def __len__(self):
		"""Return the number of particle groups in the system"""
		return len(self.groups)
	
	def __iter__(self):
		"""Iterate the system's particle groups"""
		# Iterate a copy of the group list to so that the groups
		# can be safely changed during iteration
		return iter(list(self.groups))
	
	def __contains__(self, group):
		"""Return True if the specified group is in the system"""
		return group in self.groups
	
	def update(self, time_delta):
		"""Update all particle groups in the system. time_delta is the
		time since the last update (in arbitrary time units).
		
		When updating, first the global controllers are applied to
		all groups. Then update(time_delta) is called for all groups.

		This method can be conveniently scheduled using the Pyglet
		scheduler method: pyglet.clock.schedule_interval
		"""
		for group in self:
			group.update(time_delta)
	
	def run_ahead(self, time, framerate):
		"""Run the particle system for the specified time frame at the 
		specified framerate to move time forward as quickly as possible.
		Useful for "warming up" the particle system to reach a steady-state
		before anything is drawn or to simply "skip ahead" in time.

		time -- The amount of simulation time to skip over.

		framerate -- The framerate of the simulation in updates per unit 
		time. Higher values will increase simulation accuracy, 
		but will take longer to compute.
		"""
		if time:
			td = 1.0 / framerate
			update = self.update
			for i in xrange(int(time / td)):
				update(td)
	
	def draw(self):
		"""Draw all particle groups in the system using their renderers.
		
		This method is convenient to call from you Pyglet window's
		on_draw handler to redraw particles when needed.
		"""
		for group in self:
			group.draw()
