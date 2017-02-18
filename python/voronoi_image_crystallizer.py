from array import array
from random import *

import sys
import time

class image:
	def __init__(self, width, height):
		self.width = width
		self.height = height

		## TGA header
		self.hdr = array('B')
		self.hdr.fromstring("\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00")
		self.hdr.append((self.width  >> 0) & 0xff)
		self.hdr.append((self.width  >> 8) & 0xff)
		self.hdr.append((self.height >> 0) & 0xff)
		self.hdr.append((self.height >> 8) & 0xff)
		self.hdr.append(24)
		self.hdr.append(0x30)
		self.rgb = array('B')

		## start with all channels fully white
		for i in xrange(width * height * 3):
			self.rgb.append(255)

	def write_tga(self, f):
		self.hdr.tofile(f)
		self.rgb.tofile(f)
		f.close()

	def generate_random_noise(self):
		for y in xrange(self.height):
			for x in xrange(self.width):
				pxl_idx = (y * self.width + x) * 3

				self.rgb[pxl_idx + 0] = int(random() * 255)
				self.rgb[pxl_idx + 1] = int(random() * 255)
				self.rgb[pxl_idx + 2] = int(random() * 255)

	def generate_stepped_gradient(self):
		for y in xrange(self.height):
			for x in xrange(self.width):
				pxl_col = int(round((x / (self.width / 10)) * (255.0 / (10 - 1))))
				pxl_idx = (y * self.width + x) * 3

				self.rgb[pxl_idx + 0] = pxl_col
				self.rgb[pxl_idx + 1] = pxl_col
				self.rgb[pxl_idx + 2] = pxl_col

	def voronoi_crystallize_image(self, cell_size):
		rows = self.height / cell_size
		cols = self.width / cell_size

		if ((self.height % cell_size) != 0):
			rows += 1
		if ((self.width % cell_size) != 0):
			cols += 1

		points = [0, 0] * (rows * cols)
		pindex = 0

		## pick random points as the cell centers
		for row in xrange(rows):
			for col in xrange(cols):
				points[pindex    ] = randint(col * cell_size, (col + 1) * cell_size - 1)
				points[pindex + 1] = randint(row * cell_size, (row + 1) * cell_size - 1)
				pindex += 2

		def sq_distance(x0, y0, x1, y1): return ((x1 - x0) ** 2.0 + (y1 - y0) ** 2.0)
		def distance(x0, y0, x1, y1): return (sq_distance(x0, y0, x1, y1) ** 0.5)

		def get_ngb_cell_indices(x, y, r = 5):
			cell_x = x / cell_size
			cell_y = y / cell_size

			cells = [-1] * (r * r)
			index = 0

			## grab all pixels in the vicinity of (x, y)
			for j in xrange(r):
				for i in xrange(r):
					pxl_x = cell_x - 2 + i
					pxl_y = cell_y - 2 + j

					if (pxl_x < 0 or pxl_x >= cols): continue
					if (pxl_y < 0 or pxl_y >= rows): continue

					cells[index] = (pxl_y * cols) + pxl_x
					index += 1

			return cells

		def cell_color(x, y):
			ngbs = get_ngb_cell_indices(x, y)
			indx = ngbs[0] * 2

			min_indx = 0
			min_dist = sq_distance(x, y, points[indx], points[indx + 1])

			## figure out which cell this pixel is closest to
			for cell_idx in ngbs:
				if (cell_idx == -1):
					break

				indx = cell_idx * 2
				dist = sq_distance(x, y, points[indx], points[indx + 1])

				if (dist < min_dist):
					min_dist = dist
					min_indx = indx

			xj = points[min_indx    ]
			yj = points[min_indx + 1]

			## return the color of this cell
			k = (yj * self.width + xj) * 3
			r = self.rgb[k + 0]
			g = self.rgb[k + 1]
			b = self.rgb[k + 2]
			return r, g, b

		for y in xrange(self.height):
			for x in xrange(self.width):
				r, g, b = cell_color(x, y)
				i = (y * self.width + x) * 3

				self.rgb[i + 0] = r
				self.rgb[i + 1] = g
				self.rgb[i + 2] = b


def gen_random_noise_image(xsize, ysize, csize, name):
	img = image(xsize, ysize)

	img.generate_random_noise()
	img.write_tga(open(name + ("_original_%dx%d.tga" % (xsize, ysize)), "wb"))

	img.voronoi_crystallize_image(csize)
	img.write_tga(open(name + ("_crystals_%dx%d.tga" % (xsize, ysize)), "wb"))

def gen_stepped_gradient_image(xsize, ysize, csize, name):
	img = image(xsize, ysize)

	img.generate_stepped_gradient()
	img.write_tga(open(name + ("_original_%dx%d.tga" % (xsize, ysize)), "wb"))

	img.voronoi_crystallize_image(csize)
	img.write_tga(open(name + ("_crystals_%dx%d.tga" % (xsize, ysize)), "wb"))


def main(argc, argv):
	xsize = ((argc >= 2) and int(argv[1])) or 320
	ysize = ((argc >= 3) and int(argv[2])) or 200
	csize = ((argc >= 4) and int(argv[3])) or  20

	t0 = time.time()
	gen_random_noise_image(xsize, ysize, csize, "random_noise")
	gen_stepped_gradient_image(xsize, ysize, csize, "stepped_gradient")
	t1 = time.time()

	print("[main] xs=%d ys=%d cs=%d dt=%f" % (xsize, ysize, csize, t1 - t0))
	return 0

sys.exit(main(len(sys.argv), sys.argv))

