"""Sprite class

Sprites are structs that store the sprites state. 
All sprite presentation is done via renderers
"""


import ctypes

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
	
	## Vector math methods to be moved to C ##

	def length((x, y, z)):
		return sqrt(x**2 + y**2 + z**2)
	
	def normalize(self):
		L = self.length()
		if L != 0:
			x, y, z = self
			return self.__class__(x / L, y / L, z / L)
		else:
			return self.__class__(0, 0, 0)

	def __nonempty__((x, y, z)):
		return x != 0 or y !=0 or z != 0
	
	def __lt__((sx, sy, sz), (ox, oy, oz)):
		return (sx**2 + sy**2 + sz**2) < (ox**2 + oy**2 + oz**2)

	def __gt__((sx, sy, sz), (ox, oy, oz)):
		#sx, sy, sz = self
		#ox, oy, oz = other
		return (sx**2 + sy**2 + sz**2) > (ox**2 + oy**2 + oz**2)
	
	def dot((sx, sy, sz), (ox, oy, oz)):
		return sx*ox + sy*oy + sz*oz
	
	'''
	def __mul__(self, (ox, oy, oz)):
		sx, sy, sz = self
		return self.__class__(sx*ox, sy*oy, sz*oz)
	
	def __div__(self, (ox, oy, oz)):
		sx, sy, sz = self
		return self.__class__(sx/ox, sy/oy, sz/oz)

	'''

	def __add__(self, (ox, oy, oz)):
		sx, sy, sz = self
		return self.__class__(sx+ox, sy+oy, sz+oz)
	
	def __sub__(self, (ox, oy, oz)):
		sx, sy, sz = self
		return self.__class__(sx-ox, sy-oy, sz-oz)
	
	def __mul__(self, scalar):
		sx, sy, sz = self
		return self.__class__(sx*scalar, sy*scalar, sz*scalar)	

	def __neg__(self):
		x, y, z = self
		return self.__class__(-x, -y, -z)

	def reflect(self, normal):
		sx, sy, sz = self
		nx, ny, nz = normal
		d = self.dot(normal)*2
		return self.__class__(sx - d*nx, sy - d*ny, sz - d*nz)

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


class Sprite(ctypes.Structure):
	"""sprite state"""
	_fields_ = [
		('position', Vec3), # Current sprite position
		('velocity', Vec3), # Current sprite velocity
		('last_velocity', Vec3), # Previous sprite velocity
		('last_position', Vec3), # Previous sprite position
		('size', Vec3), # sprite width, height and depth
		('up', Vec3), # sprite orientation vector
		('rotation', Vec3), # Rotation speed of up vector
		('color', Color), # sprite color
		('mass', ctypes.c_float), # sprite mass
		('age', ctypes.c_float), # Current age of sprite
		('name', ctypes.c_char), # name of the current sprite
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
