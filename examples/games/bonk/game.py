from pyglet.gl import *
from pyglet import font
import sys, os
from pyglet.window import key 
sys.path.append('../..')
sys.path.append('../../..')
from sprite_struct import Sprite, Vec3, Color
from controls import Controls, message

"""
The next thing I need to work on is getting the ball to move around.
On its own. Then collision detection. I should look at the movement 
controller in order to figure this out.

At this point its time to take the gamesystem used in bonk. The game
system will handle how objects within the game interact. Perhaps I even
need to integrate the message_handler into it. Game system is the clock
which schedules updates of all other components.

Controllers will handle things like collision detection. I don't think the particle
movement strategy will work here. Game_system is not using a set of particles.
Should I move the movement code inside each sprite? No. I should find a different
way to update the sprites. It should still be instigated from within the game_system
maybe different rates of movement based on name? So, movement, collision detection,
phsyics, should all be done from within the game_system. I don't need to iterate over loads of 
particles here.

Maybe if I make objects a set I could iterate it? Ok that works.
Next thing I need to do is to take the boundry box example and work out how to do collision detection.

I need to import box and bouncy from the controllers.

So how am I going to integate bounce?
"""

class game_system(object):
	"""game system controls when objects in the system get updated"""
	objects = ()
	controllers = ()
	def __init__(self, window=None, objects=(), controllers=()):
		#Create a font for score
		self.font = font.load('Arial', 18)
		#pyglet.font.Text object to display the score
		self.score = font.Text(self.font, x = 10, y=10, color=(1, 1, 1, 1))
		self.control = Controls(window)
		self.bind_objects(*objects)
		self.bind_controllers(*controllers)
		self.score_right = 0
		self.score_left = 0
		self.draw_score()
		self.height = window.width
		self.width  = window.height
	def draw_score(self):
		self.score.text = ("left: %d right: %d") % (self.score_left, self.score_right)
		self.score.draw()
	def bind_controllers(self, *controllers):
		"""Bind one or more controllers"""
		self.controllers += controllers
	def bind_objects(self, *objects):
		"""Bind one or more objects"""
		self.control.bind_keys(objects)
		self.objects += objects
	def add(self, object):
		"""Add an object to the list of objects that get traveresed when update is called"""
		self.objects += object
	def __iter__(self):
		"""Iterate the particles in the group"""
		# Make a copy of the particle set in case the group is
		# modified during iteration
		return iter(set(self.objects))#used to be particles
	
	def __len__(self):
		"""Return the number of particles in the group"""
		return len(self.objects)
	def send(self, message):
		#send the message to correct object
		for object in self.objects:
			if object.name == message.to:
				object.move(message.content)
		#Check to see if score has occured
		if message.to == "score":
			print "score message"
			self.update_score(message.content)
	def update_score(self, message):
		if message == "right:score":
			self.score_right += 1
		if message == "left:score":
			self.score_left += 1
		#draw the score
		self.draw_score()
		#reset ball because its gone outside of the screen
		for object in self.objects:
			if object.name =="ball":
				object.reset_ball(self.height/2, self.width/2)
	def update(self, time_delta):
		"""Update all sprites in the system. time_delta is the
		time since the last update (in arbitrary time units).
		
		This method can be conveniently scheduled using the Pyglet
		scheduler method: pyglet.clock.schedule_interval
		"""
		self.control.update(self, time_delta)
		for object in self.objects:
			object.sprite.last_position = object.sprite.position
			object.sprite.last_velocity = object.sprite.velocity

		#for group in self:
		for controller in self.controllers:
			controller(time_delta, self)
	def draw(self):
		"""Draw all the sprites in the system using their renderers.
		
		This method is convenient to call from you Pyglet window's
		on_draw handler to redraw particles when needed.
		"""
		self.draw_score()
		for sprite in self:
			sprite.draw()

class Paddle(object):
	"""Draw a rectangle paddle"""
	keys = ()
	def __init__(self, position, color, size, name, screen_height, keys=None):
		if keys != None:
			self.bind_keys(*keys)
		self.name =name
		self.velocity=(0,0,0)
		self.sprite = Sprite(position=position, color=color, size=size)
		self.screen_height = screen_height - self.sprite.size.y
		self.domain = Box((self.sprite.position.x, self.sprite.position.y, self.sprite.position.z), 
		(self.sprite.position.x+self.sprite.size.x,self.sprite.position.y+self.sprite.size.y,
		self.sprite.position.z + self.sprite.size.z))
	def install_domain(self, domain):
		"""Manually change the domain that handles collision 
		detection to something else. remove for debugging purposes only"""
		self.domain = domain
	def bind_keys(self, *keys):
		"""Bind one or more controllers"""
		if keys != None:
			self.keys += keys
	def return_keys(self):
		return keys
	def move(self, message):
		if message == "right":
			self.sprite.position.x += 10
			self.domain.update_position((self.sprite.position.x, self.sprite.position.y, 
				self.sprite.position.z), (self.sprite.position.x+self.sprite.size.x,
					self.sprite.position.y+self.sprite.size.y,self.sprite.position.z
					+ self.sprite.size.z))
		if message == "left":
			self.sprite.position.x -= 10
			self.domain.update_position((self.sprite.position.x, self.sprite.position.y, 
				self.sprite.position.z), (self.sprite.position.x+self.sprite.size.x,
					self.sprite.position.y+self.sprite.size.y,self.sprite.position.z
					+ self.sprite.size.z))
		if message == "up":
			y = self.sprite.position.y + 10
			if y < self.screen_height:
				self.sprite.position.y += 10
				self.domain.update_position((self.sprite.position.x, self.sprite.position.y, 
					self.sprite.position.z), (self.sprite.position.x+self.sprite.size.x,
						self.sprite.position.y+self.sprite.size.y,self.sprite.position.z
						+ self.sprite.size.z))
		if message == "down":
			y = self.sprite.position.y - 10
			if y > 0:
				self.sprite.position.y -= 10
				self.domain.update_position((self.sprite.position.x, self.sprite.position.y, 
					self.sprite.position.z), (self.sprite.position.x+self.sprite.size.x,
						self.sprite.position.y+self.sprite.size.y,self.sprite.position.z
						+ self.sprite.size.z))
	def draw(self):
		z1 = 0.0
		x1 = self.sprite.position.x
		y1 = self.sprite.position.y
		x2 = x1+self.sprite.size.x
		y2 = y1+self.sprite.size.y
		z2 = 0.0
		glBegin(GL_QUADS)
		glColor4fv(self.sprite.color)
		glVertex3f(x1, y1, z1)#Top left
		glVertex3f(x2, y1, z2) #Top right	
		glVertex3f(x2, y2, z2) #Bottom right
		glVertex3f(x1, y2, z1)#Bottom left
		glEnd()
	def update(self):
		"""Update state of paddle"""

class ball(object):
	"""draw a ball using glpoints"""
	def __init__(self, position=(210,100,0), velocity=(60,60,0),color=(0.0,0.0,1.0,1.0),size=(10.0,0.0,0.0)):
		self.sprite = Sprite(position=position, velocity=velocity, color=color, size=size)
		self.name ="ball"
		self.keys=None
		#We don't care if anything collides with the ball
		self.domain = None
	def move(self, message):
		print message
		if message == "a":
			self.sprite.position.x += 10
	def draw(self):
		"""Render the particles as points"""
		x =  self.sprite.position.x
		y =  self.sprite.position.y
		glPointSize( self.sprite.size.x)
		glBegin(GL_POINTS)
		glColor4fv(self.sprite.color)
		glVertex3f(x, y, 0)
		glEnd()
	def reset_ball(self, x, y):
		"""reset ball to set location on the screen"""
		self.sprite.position.x = x
		self.sprite.position.y = y
def update(self):
		"""Update state of ball"""


class Movement(object):
	"""Updates the position and velocity of particles"""

	def __init__(self, acceleration=None, damping=None, max_velocity=None):
		"""Initialize the movement controller.
		
		acceleration -- A three-tuple representing the velocity change
		vector per unit time.
		
		damping -- A three-tuple velocity multiplier per unit time
		in each respective dimension. A scalar may also be specified
		if the damping is equal in all dimensions (thus .25 is equivilent
		to (.25, .25. .25))

		max_velocity -- Maximum velocity scalar. Particle velocity
		magnitudes are clamped to this value
		"""
		self.acceleration = acceleration
		self.damping = damping
		self.max_velocity = max_velocity
	
	def __call__(self, td, objects):
		accel = self.acceleration
		if accel is not None:
			accel_x, accel_y, accel_z = accel

		damping = self.damping
		if damping is not None:
			try:
				damp_x, damp_y, damp_z = damping
			except TypeError:
				# Not iterable, assume a scalar value
				damp_x = damp_y = damp_z = damping

		max_vel = self.max_velocity
		if max_vel is not None:
			max_vel_sq = max_vel**2
				
		for p in objects:
			#Change this to a list of names of objects/To/Move not move?
			"""Perhaps each sprite should have a list of controllers(behaviors) that apply to it? This list
			gets iterated by each controllor to see if this behavior applies to the sprite"""
			if p.name is "ball":
				vel = p.sprite.velocity
				if damping is not None:
					# XXX This is highly inaccurate for td > 1.0
					vel.x += (vel.x * damp_x - vel.x) * td
					vel.y += (vel.y * damp_y - vel.y) * td
					vel.z += (vel.z * damp_z - vel.z) * td
				if accel is not None:
					vel.x += accel_x * td
					vel.y += accel_y * td
					vel.z += accel_z * td
				if max_vel is not None:
					vel_sq = vel.x**2 + vel.y**2 + vel.z**2
					if vel_sq > max_vel_sq:
						vel_mag = sqrt(vel_sq)
						adjustment = max_vel / vel_mag
						vel.x *= adjustment
						vel.y *= adjustment
						vel.z *= adjustment
				pos = p.sprite.position
				pos.x += vel.x * td
				pos.y += vel.y * td
				pos.z += vel.z * td

class Bounce(object):
	"""Controller that bounces particles off the surface of a domain
	
	For best results this controller should be bound after other controllers
	that affect the position and velocity of particles.
	"""

	def __init__(self, domain = None, friction=0, callback=None):
		"""
		domain -- Particles that collide with the surface of this domain are
		redirected as if they bounced or reflected off the surface. This
		alters the position and velocity of the particle. The domain must have
		a non-zero area.

		friction -- The fraction of velocity lost after the bounce. If
		friction is negative, the particle will gain velocity. If friction is
		1, the particle will stop at the point of collision. If friction is
		greater than 1, the particle will appear to refract off of the surface
		rather than reflect.

		callback -- An optional function called when a particle collides
		with the domain. Must have the signature:
			callback(particle, group, controller, collision_point, collision_normal)
		collision_point is point on the domain where the collision occurred.
		collision_normal is the normal vector on the domain's surface at the
		point of collision.
		"""
		self.domain = domain
		self.friction = float(friction)
		self.callback = callback

	def __call__(self, td, objects):
		"""Check for global non object specific collisions"""
		bounce = 1.0 - self.friction
		callback = self.callback
		domains = set()
		if self.domain != None:
			for p1 in objects:
				check_score = self.domain.check_score(p1.sprite.last_position, p1.sprite.position, objects)
				if check_score != "score":
					collide_point, normal = self.domain.intersect(p1.sprite.last_position, p1.sprite.position)
					if collide_point is not None:
						reflect_vec = ( p1.sprite.position - collide_point).reflect(normal) * bounce
						p1.sprite.velocity = p1.sprite.velocity.reflect(normal) * bounce
						p1.sprite.position = reflect_vec + collide_point
					if callback is not None:
						callback(p1, objects, self, collide_point, normal)
		"""check for objects colliding with other objects"""
		for p in objects:
			if p.domain != None:
				domains.add(p.domain)
		for d in domains:
			domain = d
			for p1 in objects:
				collide_point, normal = domain.intersect(p1.sprite.last_position, p1.sprite.position)
				if collide_point is not None:
					reflect_vec = ( p1.sprite.position - collide_point).reflect(normal) * bounce
					p1.sprite.velocity = p1.sprite.velocity.reflect(normal) * bounce
					p1.sprite.position = reflect_vec + collide_point
				if callback is not None:
					callback(p1, objects, self, collide_point, normal)


class BoundryCheck(object):
	"""Boundary is a 4 sided box. Get the four points which make up the box.
	x1/y1---x2/1
	|	 |
	x1/y2---x2/y2
	If interesection occures bettween x1/y1-y2, don't bounce increment scores.
	If  interesection occures bettween x2/y1-y2, don't bounce increment scores.
	else bounce. see bouncy example
	"""
	def __init__(self, domain = None, friction=0, callback=None):
		self.domain = domain

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

class Box(Domain):
	"""Axis aligned rectangular prism"""

	def __init__(self, point1, point2):
		"""point1 and point2 define any two opposite corners of the box"""
		p1 = Vec3(*point1)
		p2 = Vec3(*point2)
		self.point1 = Vec3()
		self.point2 = Vec3()
		self.point1.x = min(p1.x, p2.x)
		self.point1.y = min(p1.y, p2.y)
		self.point1.z = min(p1.z, p2.z)
		self.point2.x = max(p1.x, p2.x)
		self.point2.y = max(p1.y, p2.y)
		self.point2.z = max(p1.z, p2.z)
		self.size_x = self.point2.x - self.point1.x
		self.size_y = self.point2.y - self.point1.y
		self.size_z = self.point2.z - self.point1.z
	
	def generate(self):
		"""Return a random point inside the box"""
		x, y, z = self.point1
		return (x + self.size_x * random(),
			y + self.size_y * random(),
			z + self.size_z * random())

	def __contains__(self, (px, py, pz)):
		"""Return true if the point is within the box"""
		x1, y1, z1 = self.point1
		x2, y2, z2 = self.point2
		return (x1 <= px <= x2 and y1 <= py <= y2 and z1 <= pz <= z2)
	
	def update_position(self, point1, point2):
		"""update the position of the box"""
		p1 = Vec3(*point1)
		p2 = Vec3(*point2)
		self.point1 = Vec3()
		self.point2 = Vec3()
		self.point1.x = min(p1.x, p2.x)
		self.point1.y = min(p1.y, p2.y)
		self.point1.z = min(p1.z, p2.z)
		self.point2.x = max(p1.x, p2.x)
		self.point2.y = max(p1.y, p2.y)
		self.point2.z = max(p1.z, p2.z)
		self.size_x = self.point2.x - self.point1.x
		self.size_y = self.point2.y - self.point1.y
		self.size_z = self.point2.z - self.point1.z
	def intersect(self, start_point, end_point):
		"""Intersect the line segment with the box return the first 
		intersection point and normal vector pointing into space from
		the box side intersected.
		
		If the line does not intersect, or lies completely in one side
		of the box return (None, None)
		"""
		sx, sy, sz = start_point
		ex, ey, ez = end_point
		p1x, p1y, p1z = self.point1
		p2x, p2y, p2z = self.point2
		start_inside = start_point in self
		end_inside = end_point in self
		if start_inside != end_inside:
			if (end_inside and sy > p2y) or (start_inside and ey >= p2y) and (ey != sy):
				# Test for itersection with bottom face
				t = (sy - p2y) / (ey - sy)
				ix = (ex - sx) * t + sx
				iy = p2y
				iz = (ez - sz) * t + sz
				if p1x <= ix <= p2x and p1z <= iz <= p2z:
					return (ix, iy, iz), (0.0, (sy > p2y) * 2.0 - 1.0, 0.0)
			if (end_inside and sx < p1x) or (start_inside and ex <= p1x) and (ex != sx):
				# Test for itersection with left face
				t = (sx - p1x) / (ex - sx)
				ix = p1x
				iy = (ey - sy) * t + sy
				iz = (ez - sz) * t + sz
				if p1y <= iy <= p2y and p1z <= iz <= p2z:
					return (ix, iy, iz), ((sx > p1x) * 2.0 - 1.0, 0.0, 0.0)
			if (end_inside and sy < p1y) or (start_inside and ey <= p1y) and (ey != sy):
				# Test for itersection with top face
				t = (sy - p1y) / (ey - sy)
				ix = (ex - sx) * t + sx
				iy = p1y
				iz = (ez - sz) * t + sz
				if p1x <= ix <= p2x and p1z <= iz <= p2z:
					return (ix, iy, iz), (0.0, (sy > p1y) * 2.0 - 1.0, 0.0)
			if (end_inside and sx > p2x) or (start_inside and ex >= p2x) and (ex != sx):
				# Test for itersection with right face
				t = (sx - p2x) / (ex - sx)
				ix = p2x
				iy = (ey - sy) * t + sy
				iz = (ez - sz) * t + sz
				if p1y <= iy <= p2y and p1z <= iz <= p2z:
					return (ix, iy, iz), ((sx > p2x) * 2.0 - 1.0, 0.0, 0.0)
			if (end_inside and sz > p2z) or (start_inside and ez >= p2z) and (ez != sz):
				# Test for itersection with far face
				t = (sz - p2z) / (ez - sz)
				ix = (ex - sx) * t + sx
				iy = (ey - sy) * t + sy
				iz = p2z
				if p1y <= iy <= p2y and p1x <= ix <= p2x:
					return (ix, iy, iz), (0.0, 0.0, (sz > p2z) * 2.0 - 1.0)
			if (end_inside and sz < p1z) or (start_inside and ez <= p1z) and (ez != sz):
				# Test for itersection with near face
				t = (sz - p1z) / (ez - sz)
				ix = (ex - sx) * t + sx
				iy = (ey - sy) * t + sy
				iz = p1z
				if p1y <= iy <= p2y and p1x <= ix <= p2x:
					return (ix, iy, iz), (0.0, 0.0, (sz > p1z) * 2.0 - 1.0)
		return None, None

class Boundary(Box):
	"""The purpose of this domain, is to check when a ball goes 
	through either the left or right side of the screen and to send a message
	accordingly."""
	def __init__(self, point1, point2):
		"""point1 and point2 define any two opposite corners of the box"""
		p1 = Vec3(*point1)
		p2 = Vec3(*point2)
		self.point1 = Vec3()
		self.point2 = Vec3()
		self.point1.x = min(p1.x, p2.x)
		self.point1.y = min(p1.y, p2.y)
		self.point1.z = min(p1.z, p2.z)
		self.point2.x = max(p1.x, p2.x)
		self.point2.y = max(p1.y, p2.y)
		self.point2.z = max(p1.z, p2.z)
		self.size_x = self.point2.x - self.point1.x
		self.size_y = self.point2.y - self.point1.y
		self.size_z = self.point2.z - self.point1.z
	def check_score(self, start_point, end_point, gamesystem):
		sx, sy, sz = start_point
		ex, ey, ez = end_point
		p1x, p1y, p1z = self.point1
		p2x, p2y, p2z = self.point2
		start_inside = start_point in self
		end_inside = end_point in self
		if start_inside != end_inside:
			if (end_inside and sx < p1x) or (start_inside and ex <= p1x) and (ex != sx):
				# Test for itersection with left face
				t = (sx - p1x) / (ex - sx)
				ix = p1x
				iy = (ey - sy) * t + sy
				iz = (ez - sz) * t + sz
				if p1y <= iy <= p2y and p1z <= iz <= p2z:
					gamesystem.send(message("score", "game", "right:score"))
					return "score"
			if (end_inside and sx > p2x) or (start_inside and ex >= p2x) and (ex != sx):
				# Test for itersection with right face
				t = (sx - p2x) / (ex - sx)
				ix = p2x
				iy = (ey - sy) * t + sy
				iz = (ez - sz) * t + sz
				if p1y <= iy <= p2y and p1z <= iz <= p2z:
					gamesystem.send(message("score", "game", "left:score"))
					return "score"
		return "no score"

if __name__ == '__main__':
	"""Paddle game."""
	win = pyglet.window.Window(resizable=True, visible=False)
	paddle = Paddle((10,200,0), (1.0,0.0,0.0,1.0), (10.0,100.0,0.0), "paddle", win.height, keys=("up", "down"))
	ball = ball(position=(210,100,0), velocity=(60,60,0),color=(0.0,0.0,1.0,1.0),size=(10.0,0.0,0.0))
	paddle2 = Paddle((620,200,0), (1.0,0.0,0.0,1.0), (10.0,100.0,0.0), "paddle2", win.height, keys=("w", "s"))

	def resize(widthWindow, heightWindow):
		"""Initial settings for the OpenGL state machine, clear color, window size, etc"""
		glEnable(GL_BLEND)
		glShadeModel(GL_SMOOTH)# Enables Smooth Shading
		glBlendFunc(GL_SRC_ALPHA,GL_ONE)#Type Of Blending To Perform
		glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);#Really Nice Perspective Calculations
		glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);#Really Nice Point Smoothing
		glDisable(GL_DEPTH_TEST)

	win.resize = resize
	win.set_visible(True)
	win.resize(win.width, win.height)
	ball_size = 15
	screen_box = Boundary((ball_size/2.0, ball_size/2.0, 0), 
		(win.width-ball_size/2.0,win.height-ball_size/2.0,0))

	messageHandler = game_system(window=win, objects=[ball, paddle, paddle2], 
			controllers=[Movement(max_velocity=200), Bounce(friction=0.0), Bounce(screen_box, friction=0.01)])

	pyglet.clock.schedule_interval(messageHandler.update, (1.0/30.0))


	@win.event
	def on_draw():
		win.clear()
		messageHandler.draw()
		glLoadIdentity()

	pyglet.app.run()


