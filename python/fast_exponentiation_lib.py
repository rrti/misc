from sys import exit as exit
from time import time as tick


def exp_naive(x, n):
	if (n == 0): return 1
	if (n == 1): return x

	r = 1

	for i in xrange(n):
		r *= x

	return r

def exp_sqamu(x, n, a):
	if (n == 0): return 1
	if (n == 1): return x

	r = 0

	## recursive version; x^n = {
	##   (x^(n/2))^2 if n is even
	##   (x^(n-1))*x if n is odd
	#  }
	if ((n & 1) == 1):
		r = exp_sqamu(x, n - 1, a)
		r *= x

		## count multiplications
		a[0] += 1
	else:
		r = exp_sqamu(x, n >> 1, a)
		r *= r

		## count squares
		a[1] += 1

	return r


## see also the fast{_mod}_exp implementations in
##   miller_rabin_primality_test
##   simple_diffie_helman
##   simple_bignum_lib
def mod_exp_naive(a, b, k): return ((a ** b) % k)
def mod_exp_nosam(a, b, k):
	r = 1
	for i in xrange(b):
		## also works (if returning r % k), but allows r to grow large
		## note that neither version here is "fast" (no square-and-mult)
		## r *= (a % k)
		r *= a
		r %= k
	return r


def test_exp_sqamu(bases, exps):
	print("[fast_exp_test] bases=%s exps=%s" % (bases, exps))
	for base in bases:
		for exp in exps:
			me = [0, 0]
			t0 = tick()
			xn = exp_sqamu(base, exp, me)
			t1 = tick()
			print("\t%d^%d=%d a=%s dt=%f" % (base, exp, xn, me, t1 - t0))

def test_exp_naive(bases, exps):
	print("[slow_exp_test] bases=%s exps=%s" % (bases, exps))
	for base in bases:
		for exp in exps:
			t0 = tick()
			xn = exp_naive(base, exp)
			t1 = tick()
			print("\t%d^%d=%d dt=%f" % (base, exp, xn, t1 - t0))

def test_mod_exp(mins, maxs):
	for a in xrange(mins[0], maxs[0]):
		for b in xrange(mins[1], maxs[1]):
			for k in xrange(mins[2], maxs[2]):
				assert(mod_exp_naive(a, b, k) == mod_exp_nosam(a, b, k))


def main():
	if (False):
		test_exp_sqamu([2], [3, 4, 5, 6, 7, 8, 9, 10, 1000, 1000000, 10000000])
		test_exp_naive([2], [3, 4, 5, 6, 7, 8, 9, 10, 1000, 1000000, 10000000])

	if (True):
		test_mod_exp((2, 2, 2), (23, 23, 23))

	return 0

if (__name__ == "__main__"):
	exit(main())

