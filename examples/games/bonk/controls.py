from pyglet.window import key 
class message(object):
	"""message object"""
	def __init__(self, to, sender, content):
		self.to 	=	to
		self.sender	=	sender 
		self.content	=	content

#Taken from caseys helicopter game. Integrating it into this
"""so he is keeping track of what keys are active and which are not and continually
sending messages when a key is active"""
class Controls(object):
	# Control states (-1.0 to 1.0)
	collective = 0.7 # Main rotor thrust
	cyclic_pitch = 0.0 # Main rotor angle
	cyclic_roll = 0.0
	pedal = 0.0 # Tail-rotor thrust

	cyclic_mouse_sensitivity = 0.0015
	cyclic_keyboard_speed = 0.01
	collective_wheel_sensitivity = 0.02
	collective_keyboard_speed = 0.25
	pedal_keyboard_speed = 0.05
	pedal_centering = 0.75
	"""What is the best way to configure the key map?
	key.A will always be A. Assume that a keystroke will
	always pass a message, I need a way to set what the message is.
	a Message map. func set message
	"""
	message_map = {
			'key_a': "left",
			'key_d': "right",
			'key_w': "up",
			'key_s': "down",
			'left': "left",
			'right': "right",
			'up': "up",
			'down': "down",
			'pause': "pause",			
			}
	key_map = {
			'key_a': key.A,
			'key_d': key.D,
			'key_w': key.W,
			'key_s': key.S,
			'left': key.LEFT,
			'right': key.RIGHT,
			'up': key.UP,
			'down': key.DOWN,
			'pause': key.P,
			}
	"""
	Each object has a list of keys assigned to it. When an object is added 
	to the game system its keys are passed in to control which binds messages 
	to that object. The message logic is handled by the object"""
	#Function, name of object mapped to. Initially all set to none
	name_map ={
			"left": 	None,
			"right":	None, 
			"up":		None,
			"down":		None, 
			"a": 	None,
			"d":	None, 
			"w": 	None,
			"s":	None
				}

	def __init__(self, window):
		self.configure_keys()
		window.push_handlers(self)
	def bind_key_name(self, function, object_name):
		"""Bind a key to an object name"""
		for funcname, name in self.name_map.items():
			if funcname == function:
				self.name_map[funcname] = object_name#getattr(self, funcname)
	def bind_keys(self, objects):
		"""Configure name map: My goal here is to associate a named object 
		with a specific function"""
		for object in objects:
			if object.keys != None:
				for key in object.keys:
					if key != None:
						self.bind_key_name(key, object.name)
				#for funcname, key in self.key_map.items():
		
	def configure_keys(self):
		"""Configure key map"""
		self.active_functions = set()
		self.key2func = {}
		for funcname, key in self.key_map.items():
			self.key2func[key] = getattr(self, funcname)

	@staticmethod
	def clamp(value, minval=-1.0, maxval=1.0):
		return min(max(value, minval), maxval)

	def on_mouse_motion(self, x, y, dx, dy):
		# Mouse controls cyclic
		self.cyclic_pitch = self.clamp(self.cyclic_pitch - dy * self.cyclic_mouse_sensitivity)
		self.cyclic_roll = self.clamp(self.cyclic_roll - dx * self.cyclic_mouse_sensitivity)

	def on_mouse_scroll(self, x, y, scroll_x, scroll_y):
		self.collective = self.clamp(
			self.collective - scroll_y * self.collective_wheel_sensitivity, minval=0)
       
	def on_key_press(self, symbol, modifier):
		if symbol in self.key2func:
			self.active_functions.add(self.key2func[symbol])
			return True
       
	def on_key_release(self, symbol, modifier):
		try:
			self.active_functions.remove(self.key2func[symbol])
			return True
		except KeyError:
			pass
       
	def update(self, gamesystem, dt):
		if abs(self.pedal) < self.pedal_keyboard_speed / 2:
			self.pedal = 0
			'''
			if abs(self.cyclic_pitch) < self.cyclic_keyboard_speed / 2:
			self.cyclic_pitch = 0
			if abs(self.cyclic_roll) < self.cyclic_keyboard_speed / 2:
			self.cyclic_roll = 0
			'''
		for func in self.active_functions:
			func(gamesystem, dt)
		self.pedal *= self.pedal_centering
		#self.cyclic_pitch *= self.pedal_centering
		#self.cyclic_roll *= self.pedal_centering
	# Control functions #
	"""so The problem now is how do I map keys to certain functions. 
	   I am currently mapping object names to keys. For now ignore and
	   verify that multiple objects can take input."""
	def key_a(self, gamesystem, dt):
		gamesystem.send(message(self.name_map['a'], "game", self.message_map['key_a']))
	def key_d(self, gamesystem, dt):
		gamesystem.send(message(self.name_map['d'], "game", self.message_map['key_d']))
	def key_w(self, gamesystem, dt):
		gamesystem.send(message(self.name_map['w'], "game", self.message_map['key_w']))
	def key_s(self, gamesystem, dt):
		gamesystem.send(message(self.name_map['s'], "game", self.message_map['key_s']))
	def down(self, gamesystem, dt): 
		gamesystem.send(message(self.name_map['down'], "game", self.message_map['down']))
	def up(self, gamesystem, dt):
		gamesystem.send(message(self.name_map['up'], "game", self.message_map['up']))
	def left(self, gamesystem, dt):
		gamesystem.send(message(self.name_map['left'], "game", self.message_map['left']))
	def right(self, gamesystem, dt):
		gamesystem.send(message(self.name_map['right'], "game", self.message_map['right']))
	def pause(self, dt):
		global PAUSED
		PAUSED = not PAUSED
