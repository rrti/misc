import random
import sys

def generate_sierpinski_fractal(argv):
	## define an equilateral triangle
	## and set the initial position
	vertices = [(-1, 0), (1, 0), (0, 3.0 ** 0.5)]
	positions = open("%s.dat" % argv[0], 'w')

	n = int(argv[1])
	p = [
		(vertices[0][0] + vertices[1][0]) * 0.5,
		(vertices[0][1] + vertices[2][1]) * 0.5
	]

	for i in xrange(n):
		## pick one of the three vertices
		r = random.random()
		j = int(r * 3.0)

		assert(j >= 0)
		assert(j <  3)

		v = vertices[j]

		## iterate the position
		p[0] = (v[0] + p[0]) * 0.5
		p[1] = (v[1] + p[1]) * 0.5

		assert(p[0] >= -1.0 and p[0] <= 1.0)
		assert(p[1] >= 0.0)

		## save it
		positions.write("%f\t%f\n" % (p[0], p[1]))

	positions.close()

def main(argc, argv):
	if (argc == 1):
		print("usage: %s <iterations>" % argv[0])
		return -1

	generate_sierpinski_fractal(argv)
	return 0

sys.exit(main(len(sys.argv), sys.argv))

