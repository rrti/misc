## input:
##   texture (-channel) in which all pixels are either 0 or 255,
##   maximum search-radius for local neighborhood around pixels
##   (should be an integer)
## output:
##   modified texture (-channel) in which all non-255 pixels now
##   contain normalized distance value to the nearest 255-pixel

import sys

def PrintRow(row):
	s = ""
	for v in row:
		s += (" %.2f " % v)
	print(s)

def PrintChannel(rows, xsize, ysize):
	for y in xrange(ysize):
		PrintRow(rows[(y * xsize): ((y + 1) * xsize)])
	print("\n")


def FillChannel(srcChannel, xsize, ysize, radius):
	dstChannel = [srcChannel[i] for i in xrange(xsize * ysize)]

	for y in xrange(ysize):
		for x in xrange(xsize):
			if (srcChannel[y * xsize + x] != 1.0):
				continue

			## diffuse the "on" pixels radially
			for i in xrange(-radius, radius + 1):
				xi = x + i
				xi = max(xi,         0)
				xi = min(xi, xsize - 1)

				for j in xrange(-radius, radius + 1):
					if (i == 0 and j == 0):
						continue

					yj = y + j
					yj = max(yj,         0)
					yj = min(yj, ysize - 1)

					dx = xi - x
					dy = yj - y
					rr = (dx * dx + dy * dy)
					v = srcChannel[yj * xsize + xi]

					## can only happen with more than one 255-pixel
					if (v == 1.0):
						continue

					if (rr > radius * radius):
						continue

					## NOTE: if rr == RR, then r == radius and d will be 0!
					r = rr ** 0.5
					## normalized distance
					d = min(0.99, r / radius)

					dstChannel[yj * xsize + xi] = max(v, d)

	return dstChannel

def main(argc, argv):
	x_size  = (128/8)+1
	y_size  =  (64/8)+1
	radius  =         4

	if (argc == 4):
		x_size = int(argv[1])
		y_size = int(argv[2])
		radius = int(argv[3])

	channel = [0.0] * (x_size * y_size)

	channel[(y_size / 2) * x_size + (x_size / 2)] = 1.0
	channel[(y_size / 2) * x_size +           1 ] = 1.0

	PrintChannel(channel, x_size, y_size)
	channel = FillChannel(channel, x_size, y_size, radius)
	PrintChannel(channel, x_size, y_size)

	return 0

sys.exit(main(len(sys.argv), sys.argv))

