## NOTE:
##   this code assumes that 1) A and B are always of the same size (limb-wise)
##   and 2) that operations are allowed to overflow the result C (which is of
##   equal size as A and B) *without* wrap-around
## TODO:
##   generalize operators such that A and B may be of arbitrary sizes and
##   number of limbs in C will be automatically adjusted to fit the result
##
import sys
import time
import random

BIGNUM_BASE = 256
LIMB_MAXVAL = BIGNUM_BASE - 1

ZERO_BIGNUM = None
UNIT_BIGNUM = None

def init_globals(size):
	global ZERO_BIGNUM
	global UNIT_BIGNUM

	if (ZERO_BIGNUM == None or len(ZERO_BIGNUM) != size):
		ZERO_BIGNUM = bignum(size)
	if (UNIT_BIGNUM == None or len(UNIT_BIGNUM) != size):
		UNIT_BIGNUM = bignum(size)

	UNIT_BIGNUM[size - 1] = 1

class bignum:
	def __init__(self, size):
		self.limbs = [0] * size

	def __add__(a, b): return (a.add(b))
	def __sub__(a, b): return (a.sub(b))
	def __mul__(a, b): return (a.mul(b))
	def __div__(a, b): return (a.div(b)[0])
	def __mod__(a, b): return (a.div(b)[1])
	## def __mod__(a, b): return (a.mod(b))
	def __pow__(a, b): return (a.exp(b))

	def __eq__(a, b): return (    a.is_equal(b))
	def __ne__(a, b): return (not a.is_equal(b))
	def __lt__(a, b): return (a.is_smaller(b))
	def __gt__(a, b): return (a.is_greater(b))

	def __ge__(a, b): return ((a > b) or (a == b))
	def __le__(a, b): return ((a < b) or (a == b))

	def __setitem__(self, idx, val): self.limbs[idx] = val
	def __getitem__(self, idx): return self.limbs[idx]

	def __len__(self): return (len(self.limbs))


	def clone(self):
		r = bignum(len(self))
		for n in xrange(len(self)):
			r[n] = self[n]
		return r

	## this requires built-in bignum support
	def to_native_number(self):
		n = 0
		for i in xrange(len(self)):
			j = len(self) - i - 1
			n += ((BIGNUM_BASE ** i) * self[j])
		return n


	def randomize(self, limb_idx):
		for i in xrange(limb_idx, len(self)):
			self[i] = random.randint(0, LIMB_MAXVAL)

	def is_valid(self):
		for n in self.limbs:
			assert(n >= 0 and n < BIGNUM_BASE)
		return True

	def same_size(a, b):
		return (len(a) == len(b))


	def argmax(a, b):
		assert(a.same_size(b))

		for n in xrange(len(a)):
			if (a[n] > b[n]): return a
			if (a[n] < b[n]): return b

		## all limbs are equal
		return a

	def argmin(a, b):
		assert(a.same_size(b))

		for n in xrange(len(a)):
			if (a[n] < b[n]): return a
			if (a[n] > b[n]): return b

		## all limbs are equal
		return a


	def is_equal(a, b):
		ret = True
		for n in xrange(len(a)):
			ret &= (a[n] != b[n])
		return ret

	def is_greater(a, b): return (a.argmax(b) == a)
	def is_smaller(a, b): return (a.argmin(b) == a)

	def is_odd(self): return ((self[len(self) - 1] & 1) == 1)
	def is_even(self): return ((self[len(self) - 1] & 1) == 0)


	def shl(self, count):
		if (count == 0):
			return (self.clone())

		c = bignum(len(self))

		for n in xrange(count, len(self), 1):
			c[n - count] = self[n]

		return c

	def shr(self, count):
		if (count == 0):
			return (self.clone())

		c = bignum(len(self))

		for n in xrange(len(self) - 1, count - 1, -1):
			c[n] = self[n - count]

		return c




	def add_limbs(self, a_limbs, b_limbs,  ab_limb_order):
		c_limbs = self.limbs

		## MSB of <C> is at index 0, but the limbs are
		## numbered in increasing order from the *L*SB
		limb_idx = len(a_limbs) - ab_limb_order - 1
		limb_sum = a_limbs[limb_idx] + b_limbs[limb_idx]

		## get carry from last limb-addition (if any)
		limb_carry = c_limbs[limb_idx]

		assert(limb_carry <= 1)
		assert(limb_sum < (BIGNUM_BASE + BIGNUM_BASE))

		## base-10 example; order is base^a_i
		##
		##  i |   3   2   1   0
		##  --+----------------
		##  A |   d   c   b   a
		##  B |   h   g   f   e
		##
		##  C = (a+e)*1 + (b+f)*10 + (c+g)*100 + (d+h)*1000
		##
		c_limbs[limb_idx] = limb_sum + limb_carry

		## set new carry
		limb_carry = int(c_limbs[limb_idx] >= BIGNUM_BASE)

		## handle limb overflow
		if (limb_carry == 1):
			c_limbs[limb_idx] -= BIGNUM_BASE

		if (limb_idx == 0):
			## last limb, no wrap-around carry
			return

		assert(c_limbs[limb_idx - 1] == 0)

		## propagate carry to next limb-addition
		c_limbs[limb_idx - 1] = limb_carry


	def sub_limbs(self, a_limbs, b_limbs,  ab_limb_order):
		c_limbs = self.limbs

		limb_idx = len(a_limbs) - ab_limb_order - 1
		limb_dif = a_limbs[limb_idx] - b_limbs[limb_idx]

		## get carry from last limb-subtraction (if any)
		limb_carry = c_limbs[limb_idx]

		c_limbs[limb_idx] = limb_dif - limb_carry

		## set new carry
		limb_carry = int(c_limbs[limb_idx] < 0)

		if (limb_carry == 1):
			c_limbs[limb_idx] += BIGNUM_BASE

		if (limb_idx == 0):
			## last limb, no wrap-around carry
			return

		## propagate carry to next limb-subtraction
		c_limbs[limb_idx - 1] = limb_carry


	def mul_limbs(self, a_limbs, b_limbs,  a_limb_order, b_limb_order):
		c_limbs = self.limbs

		limb_idx_a = len(a_limbs) - a_limb_order - 1
		limb_idx_b = len(a_limbs) - b_limb_order - 1
		limb_idx_c = len(a_limbs) - (a_limb_order + b_limb_order) - 1

		limb_mul = a_limbs[limb_idx_a] * b_limbs[limb_idx_b]

		if (limb_mul == 0):
			return

		## base-10 example; order is (base^a_i * base^b_j)
		##   i |   3   2   1   0
		##   --+----------------
		##   A |   d   c   b   a
		##   B |   h   g   f   e
		##
		##   C =
		##     (a*e)*(1*   1) + (b*e)*(10*   1) + (c*e)*(100*   1) + (d*e)*(1000*   1) +
		##     (a*f)*(1*  10) + (b*f)*(10*  10) + (c*f)*(100*  10) + (d*f)*(1000*  10) +
		##     (a*g)*(1* 100) + (b*g)*(10* 100) + (c*g)*(100* 100) + (d*g)*(1000* 100) +
		##     (a*h)*(1*1000) + (b*h)*(10*1000) + (c*h)*(100*1000) + (d*h)*(1000*1000)
		##
		limb_lo_val = limb_mul % BIGNUM_BASE
		limb_hi_val = limb_mul / BIGNUM_BASE
		limb_excess = False

		if (limb_idx_c >= 0): c_limbs[limb_idx_c    ] += limb_lo_val; limb_excess |= (c_limbs[limb_idx_c    ] >= BIGNUM_BASE)
		if (limb_idx_c >= 1): c_limbs[limb_idx_c - 1] += limb_hi_val; limb_excess |= (c_limbs[limb_idx_c - 1] >= BIGNUM_BASE)

		if (not limb_excess):
			return

		## propagate carry across limbs
		while (limb_idx_c > 0):
			if (c_limbs[limb_idx_c] >= BIGNUM_BASE):
				assert((c_limbs[limb_idx_c] / BIGNUM_BASE) == 1)

				c_limbs[limb_idx_c - 1] += 1
				c_limbs[limb_idx_c    ] -= BIGNUM_BASE

			## terminate propagation early if possible
			if (c_limbs[limb_idx_c - 1] < BIGNUM_BASE):
				break

			limb_idx_c -= 1

		## handle last limb (MSB), no wrap-around
		c_limbs[0] %= BIGNUM_BASE




	def add(a, b):
		assert(a.is_valid() and b.is_valid() and a.same_size(b))

		c = bignum(len(a))

		for ab_limb_order in xrange(len(a)):
			c.add_limbs(a.limbs, b.limbs,  ab_limb_order)

		return c


	def sub(a, b):
		assert(a.is_valid() and b.is_valid() and a.same_size(b))

		c = bignum(len(a))

		for ab_limb_order in xrange(len(a)):
			c.sub_limbs(a.limbs, b.limbs,  ab_limb_order)

		return c


	def mul(a, b):
		assert(a.is_valid() and b.is_valid() and a.same_size(b))

		c = bignum(len(a))

		for a_limb_order in xrange(len(a)):
			for b_limb_order in xrange(len(b)):
				c.mul_limbs(a.limbs, b.limbs,  a_limb_order, b_limb_order)

		return c


	def div(a, b):
		assert(a.is_valid() and b.is_valid() and a.same_size(b))
		assert(b != ZERO_BIGNUM)

		## a/1=a and a/a=1
		if (b == UNIT_BIGNUM):
			return (a.clone(), ZERO_BIGNUM.clone())
		if (b == a):
			return (UNIT_BIGNUM.clone(), ZERO_BIGNUM.clone())

		n = len(a)
		q = bignum(n) ## quotient
		r = bignum(n) ## remainder

		for i in xrange(n):
			## shift remainder left, pull down another limb of
			## numerator (a) to new right-most (LSB) position
			##
			## new remainder is quotient modulo denominator
			r = r.shl(1)
			r[n - 1] = a[i]

			while (r >= b):
				## find greatest multiple of denominator (b) < quotient
				r -= b
				q[i] += 1

		return (q, r)


	## NOTE: redundant since div() also returns the remainder
	def mod(a, b):
		assert(a.is_valid() and b.is_valid() and a.same_size(b))
		assert(b != ZERO_BIGNUM)

		## a%1=0
		if (b == UNIT_BIGNUM):
			return (ZERO_BIGNUM.clone())

		c = bignum(len(a))

		c = a / b ## C =   (A/B)
		c = c * b ## C =   (A/B)*B
		c = a - c ## C = A-(A/B)*B

		return c


	def exp(a, b):
		assert(a.is_valid() and b.is_valid() and a.same_size(b))

		if (b == ZERO_BIGNUM): return (UNIT_BIGNUM.clone()) ## a^0=1
		if (b == UNIT_BIGNUM): return (                  a) ## a^1=a

		c = bignum(len(a))

		if (b.is_odd()):
			## r=x*(x^(n-1))
			c = a ** (b - UNIT_BIGNUM)
			c = a * c
		else:
			## r=(x^(n/2))^2
			c = a ** (b / (UNIT_BIGNUM + UNIT_BIGNUM))
			c = c * c

		return c



def main(argc, argv):
	global BIGNUM_BASE
	global LIMB_MAXVAL

	ARITHM_OPER = ((argc > 1) and int(argv[1])) or 0
	BIGNUM_BASE = ((argc > 2) and int(argv[2])) or 10
	BIGNUM_SIZE = ((argc > 3) and int(argv[3])) or 10
	TEST_ROUNDS = ((argc > 4) and int(argv[4])) or 1000
	LIMB_MAXVAL = BIGNUM_BASE - 1

	init_globals(BIGNUM_SIZE)
	test_init_time = time.time()

	for i in xrange(1, TEST_ROUNDS + 1):
		if ((i % 10) == 0):
			print("[main][i=%d]" % i)

		a = bignum(BIGNUM_SIZE)
		b = bignum(BIGNUM_SIZE)
		c = None
		t = None

		a.randomize(BIGNUM_SIZE / 2)
		b.randomize(BIGNUM_SIZE / 2)

		## swap a and b if b is larger, otherwise we cannot
		## safely assert (p - q) == r because python numbers
		## are signed
		if (b > a):
			t = b
			b = a
			a = t

		if   (ARITHM_OPER == 0): c = a  + b
		elif (ARITHM_OPER == 1): c = a  - b
		elif (ARITHM_OPER == 2): c = a  * b
		elif (ARITHM_OPER == 3): c = a  / b
		elif (ARITHM_OPER == 4): c = a  % b
		elif (ARITHM_OPER == 5): c = a ** b

		p = a.to_native_number()
		q = b.to_native_number()
		r = c.to_native_number()

		if (p  > q): assert(a >  b)
		if (p  < q): assert(a <  b)
		if (p == q): assert(a == b)
		if (p != q): assert(a != b)
		if (p >= q): assert(a >= b)
		if (p <= q): assert(a <= b)

		if   (ARITHM_OPER == 0): assert((p + q) == r)
		elif (ARITHM_OPER == 1): assert((p - q) == r)
		elif (ARITHM_OPER == 2): assert((p * q) == r)
		elif (ARITHM_OPER == 3): assert((p / q) == r)
		elif (ARITHM_OPER == 4): assert((p % q) == r)
		elif (ARITHM_OPER == 5): pass ## assert((p ** q) == r)

		## final sanity-check
		assert(a.div(b)[1] == a.mod(b))

	test_stop_time = time.time()
	test_diff_time = test_stop_time - test_init_time

	print("[main] oper=%d base=%d size=%d iters=%d time=%f" % (ARITHM_OPER, BIGNUM_BASE, BIGNUM_SIZE, TEST_ROUNDS, test_diff_time))
	return 0

sys.exit(main(len(sys.argv), sys.argv))

