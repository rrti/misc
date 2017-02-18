import sys
import random

def mod_exp(x, n, m):
	r = 1

	## r = (x ^ n) % m
	while (n > 0):
		if ((n & 1) == 1):
			r = (r * x) % m

		x = (x * x) % m
		n >>= 1

	return r

class diffie_helman_player:
	def __init__(self, p, g):
		self.p = p
		self.g = g

		self.set_pri_key()
		self.set_pub_key()

	def __repr__(self):
		return ("<pub_key=%d :: pri_key=%d>" % (self.pub_key, self.pri_key))

	## private key 'k' from the set [2, ..., p - 2]
	def set_pri_key(self):
		self.pri_key = random.randint(2, self.p - 2)
	## public key 'K' computed as (g ^ k) % p
	def set_pub_key(self):
		self.pub_key = mod_exp(self.g, self.pri_key, self.p)

	def get_pri_key(self): return self.pri_key
	def get_pub_key(self): return self.pub_key
	
	## shared key computed as ([K] ^ k) % p
	def calc_shared_key(self, player):
		return (mod_exp(player.get_pub_key(), self.get_pri_key(), self.p))

def is_group_generator(g, p):
	## sanity-check that g really is a generator (i.e. a group element
	## with maximum order O=|F(p)*| where |..| denotes set cardinality)
	##
	## L[0] must be 0, L[k] must be non-zero for all k
	L = [0] * p
	O = 0

	for k in xrange(1, p):
		n = mod_exp(g, k, p)

		assert(n <  p)
		assert(n != 0)

		if (L[n] != 0):
			O = k - 1
			break
		else:
			L[n] = 1
			O += 1

		## print("\tk=%d ((g * k) %% p)=%d" % (k, n))

	## only these orders are possible for candidate-generators
	## note: F(p)* has p - 1 elements, which is an even number
	assert(((p - 1) % O) == 0)
	assert(sum(L) == O)
	return (O == (p - 1))


def diffie_helman_key_exchange(p, g):
	## p := random large prime chosen by one of the parties, not kept secret
	## g := generator of the prime-group F(p)* = {1, ..., p - 1} (F is cyclic)
	assert(g < p)

	a = diffie_helman_player(p, g)
	b = diffie_helman_player(p, g)

	assert(a.calc_shared_key(b) == b.calc_shared_key(a))
	assert(is_group_generator(g, p))

def main(argc, argv):
	diffie_helman_key_exchange(31, 17)
	diffie_helman_key_exchange(47,  5)
	return 0

sys.exit(main(len(sys.argv), sys.argv))

