import sys
import random

## fast exponentiation-by-squaring via the
## square-and-multiply method; the modulus
## is optional
def fast_exp(x, n, m = 0):
	r = 1

	if (m == 0):
		## r = (x ^ n)
		while (n > 0):
			if ((n & 1) == 1):
				r *= x
				## not needed because of the bitshift
				## (n >> 1) == ((n-1) >> 1) for odd n
				## n -= 1

			x *= x
			n >>= 1
	else:
		## r = (x ^ n) % m
		while (n > 0):
			if ((n & 1) == 1):
				r = (r * x) % m

			x = (x * x) % m
			n >>= 1

	return r



## Wilson's theorem states that every prime p divides (p - 1)! + 1
## therefore, a number that does not divide this term is not prime
def wilson_primality_test(p):
	n = 1

	for i in xrange(1, p):
		## (n * i) mod p === r
		n = (n * i) % p

	return (((n + 1) % p) == 0)

def miller_rabin_primality_test(n, k):
	if (n == 1): return False
	if (n == 2): return True
	if ((n & 1) == 0): return False

	k = max(k, 20)
	d = n - 1

	while ((d & 1) == 0):
		d >>= 1

	for i in xrange(k):
		r = int(random.random() * (n - 2))
		a = r + 1
		t = d
		y = fast_exp(a, t, n)

		while ((t != (n - 1)) and (y != 1) and (y != (n - 1))):
			y = (y * y) % n
			t <<= 1

		if ((y != (n - 1)) and ((t & 1) == 0)):
			return False

	return True

## N := number of candidates to generate
## K := maximum value of a candidate
## L := number of M-R iterations to try
##
def run_primality_tests(N, K, L):
	print("[run_primality_tests][N=%d,K=%d,L=%d]" % (N, K, L))
	for i in xrange(N):
		r = int(random.random() * K)
		pi = miller_rabin_primality_test(i, L)
		pr = miller_rabin_primality_test(r, L)
		print("\t[i=%d (prime=%d)] r=%d (prime=%d)" % (i, pi, r, pr))

def main(argc, argv):
	N = 1000
	K = 1000 ** 3
	L = 20

	if (argc > 1): N = int(argv[1])
	if (argc > 2): K = int(argv[2])
	if (argc > 3): L = int(argv[3])

	run_primality_tests(N, K, L)
	return 0

sys.exit(main(len(sys.argv), sys.argv))

