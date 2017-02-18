from random import seed as rseed
from random import randint as rint
from time import time
import sys



## same as int(log(n) / log(b)) + 1, but for e.g. n=1000 and b=10
## that will evaluate to int(2.9999999999999996)+1=3 which is not
## useful
def num_base_digits(n, b = 2):
	i = 0
	while (n > 0):
		n /= b
		i += 1
	return i



def karatsuba_multiply_rec(x, y, nbase = 2, depth = 0):
	nx = num_base_digits(x, nbase)
	ny = num_base_digits(y, nbase)

	## do not assume that x and y are always equally-sized
	## (b=B^m might end up being larger than either of the
	## operands otherwise)
	navg = (nx + ny) >> 1
	nmin = min(nx, ny) >> 1

	a = (navg > nx or navg > ny)
	n = navg * (1 - a) + (nmin * a)
	m = n >> 1

	##
	## print("%sx=%d y=%d n=%d m=%d a=%d" % ('\t' * depth, x, y, n, m, a))
	##

	## terminate recursion when numbers become unsplittable
	if (m <= 1):
		return (x * y)

	if (nbase == 2):
		## special case; muls and divs are bit-shifts here
		b = 1 << (m    )
		c = 1 << (m + m)

		x1 = x >> m
		y1 = y >> m
		x0 = x - (x1 << m)
		y0 = y - (y1 << m)
	else:
		b = nbase ** m
		c = b * b

		x1 = x / b
		y1 = y / b
		x0 = x - (x1 * b)
		y0 = y - (y1 * b)

	## assert(x > 0)
	## assert(y > 0)
	## assert(x1 > 0)
	## assert(y1 > 0)
	## assert(x0 >= 0)
	## assert(y0 >= 0)
	## assert(x0 < b)
	## assert(y0 < b)
	## assert(b < x)
	## assert(b < y)
	## assert(x == ((x1 * b) + x0))
	## assert(y == ((y1 * b) + y0))

	## check if x0 or y0 became 0 in the current recursive step
	## note that numbers which are close to a power of <nbase>
	## (or an exact multiple of b) will not result in balanced
	## splits
	z2 = (                      karatsuba_multiply_rec(x1,      y1,      nbase, depth + 1))
	z0 = (x0 > 0 and y0 > 0 and karatsuba_multiply_rec(     x0,      y0, nbase, depth + 1)) or 0
	z1 = (                      karatsuba_multiply_rec(x1 + x0, y1 + y0, nbase, depth + 1) - z2 - z0)

	return ((z2 * c) + (z1 * b) + z0)

def karatsuba_multiply(x, y, nbase = 2):
	nx = num_base_digits(x, nbase)
	ny = num_base_digits(y, nbase)
	dn = abs(nx - ny)

	## make sure x and y have an equal number of digits
	## extending both to a common power of two would be
	## even more efficient
	if (nbase == 2):
		s = dn

		y <<= (((nx > ny) and s) or 0)
		x <<= (((ny > nx) and s) or 0)

		r = karatsuba_multiply_rec(x, y, nbase) >> s
	else:
		s = nbase ** dn

		y *= (((nx > ny) and s) or 1)
		x *= (((ny > nx) and s) or 1)

		r = karatsuba_multiply_rec(x, y, nbase) / s

	return r



def main(argc, argv):
	t0 = time()

	seed = (argc > 1 and int(argv[1])) or 123456789
	xmin = 1000000000000
	ymin = 1000000000000
	xmax = 5000000000000
	ymax = 5000000000000

	rseed(seed)

	for b in xrange(2, 64 + 1):
		for i in xrange(5000):
			x = rint(xmin, xmax)
			y = rint(ymin, ymax)

			assert(karatsuba_multiply(x, y, b) == (x * y))

	t1 = time()
	dt = t1 - t0

	print("[main] dt=%fs" % dt)
	return 0

sys.exit(main(len(sys.argv), sys.argv))

