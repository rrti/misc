import math
import random

tau = math.pi * 2.0
cos = math.cos
sin = math.sin
rng = random.random

def uniform_sample_carthesian_disk_point(radius):
	x = 1.0 + radius
	y = 1.0 + radius

	## Carthesian-coordinate (rejection) sampling
	while (((x*x) + (y*y)) > (radius*radius)):
		x = ((rng() * 2.0) - 1.0) * radius
		y = ((rng() * 2.0) - 1.0) * radius

	return (x, y)

def uniform_sample_polar_disk_point(radius):
	## polar-coordinate sampling (radius, angle)
	r0 = rng()
	r1 = rng()

	## wrong, will not result in uniform sampling density
	## sr = r0
	## right, see disk point-picking article on mathworld
	sr = r0 ** 0.5

	x = sr * cos(r1 * tau) * radius
	y = sr * sin(r1 * tau) * radius

	return (x, y)

def uniform_sample_disk_points(N, R):
	f = open("points.dat", 'w')

	for n in xrange(N):
		p = uniform_sample_polar_disk_point(R)
		f.write("%f\t%f\n" % (p[0], p[1]))

	f.close()

uniform_sample_disk_points(10000, 5.0)

