import random
import sys

def factorize(n, factors, depth):
	if (depth == 0):
		print("n: %d" % n)

	if ((n & 1) == 0):
		factors.append(2)

		print("[e] factor: %d (rem: %d)" % (2, n >> 1))
		return (factorize(n >> 1, factors, depth + 1) + 1)
	else:
		d = 3

		## NOTE: we only need to test prime divisors here
		## (as opposed to every odd number between 3 and n)
		##
		## cf. Euler Problem 3
		while (d <= n):
			r = n / d

			if ((r * d) == n):
				factors.append(d)

				print("[o] factor: %d (rem: %d)" % (d, r))
				return (factorize(r, factors, depth + 1) + 1)
				break
			else:
				d += 2

		return 0

def main(argc, argv):
	n = 0
	f = []

	if (argc <= 1):
		n = factorize(random.randint(1, 1 << 32), f, 0)
	else:
		n = factorize(int(argv[1]), f, 0)

	print("[main] number of factors: %d (%s)" % (n, f))
	return 0

sys.exit(main(len(sys.argv), sys.argv))

