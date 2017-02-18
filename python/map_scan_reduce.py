def swap(a, b):
	return b, a

def next_power_of_two(n):
	k = 1

	while (k < n):
		k <<= 1

	return k



def s_map(data, func):
	if (len(data) == 0):
		return []

	rdata = [0] * len(data)

	for i in xrange(len(data)):
		rdata[i] = func(data[i])

	return rdata

def p_map(data, func):
	assert(False)



## reduce() does not need an identity-element
## (except when called as part of scan() over
## empty inputs)
def s_reduce(data, oper, base = 0):
	if (len(data) == 0):
		return base

	accum = data[0]

	for i in xrange(1, len(data)):
		accum = oper(accum, data[i])

	return accum

def p_reduce(data, oper, base = 0):
	if (len(data) == 0):
		return base

	## handle non-power-of-two array lengths by right-padding
	rdata = data[:] + [base] * (next_power_of_two(len(data)) - len(data))
	index = len(rdata) >> 1

	while (index > 0):
		##
		## iteration 0: indices={0,1,2,3}
		##   A[0] = A[0] + A[1] = 1+2=3
		##   A[1] = A[2] + A[3] = 3+4=7
		##   A[2] = A[4] + A[5] = 5+6=11
		##   A[3] = A[6] + A[7] = 7+8=15
		## iteration 1: indices={0,1}
		##   A[0] = A[0] + A[1] =  3+ 7=10
		##   A[1] = A[2] + A[3] = 11+15=26
		## iteration 2: indices={0}
		##   A[0] = A[0] + A[1] = 10+26=36
		##
		for i in xrange(0, index):
			rdata[i] = oper(rdata[i << 1], rdata[(i << 1) + 1])

		index >>= 1

	return rdata[0]



def s_scan(data, oper, base, incl = True):
	if (len(data) == 0):
		return []

	rdata = [base] * len(data)
	accum = base

	if (incl):
		for i in xrange(0, len(data)):
			accum = oper(accum, data[i])
			rdata[i] = accum
	else:
		for i in xrange(0, len(data)):
			rdata[i] = accum
			accum = oper(accum, data[i])

	return rdata

def p_scan(data, oper, base, incl = True):
	rdata = [base] * len(data)

	## log(N) steps and O(N^2) opers (work) total
	if (incl):
		for i in xrange(len(data)):
			rdata[i] = p_reduce(data[: i + 1], oper, base)
	else:
		for i in xrange(len(data)):
			rdata[i] = p_reduce(data[: i], oper, base)

	return rdata



## Hillis-scan; always inclusive; log(N) steps and N*log(N) opers (work) total
def p_scan_hillis(data, oper, base):
	rdata = data[:] + [base] * (next_power_of_two(len(data)) - len(data))
	wdata = rdata[:]
	shift = 1

	while (shift < len(rdata)):
		for i in xrange(len(rdata)):
			## iterate in reverse s.t. copying elements (when
			## shft > j) is not needed; during each iteration
			## all opers can be run in parallel
			##
			## not strictly true because the reads and writes
			## still access overlapping indices (the parallel
			## version requires ping-pongs between {r,w}data)
			j = len(rdata) - 1 - i
			k = int((j - shift) >= 0)
			v = ((k == 1) and rdata[j - shift]) or base

			## this only works when iterating in reverse
			## rdata[j] = oper(rdata[j], v)
			wdata[j] = oper(rdata[j], v)

		## ping-pong
		tdata = rdata
		rdata = wdata
		wdata = tdata

		shift <<= 1

	return rdata[: len(data)]

## Blelloch-scan; always exclusive; 2*log(N) steps and O(N) opers (work) total
def p_scan_blelloch(data, oper, base):
	rdata = data[:] + [base] * (next_power_of_two(len(data)) - len(data))
	wdata = rdata[:]
	shift = 1

	## stage 1: reduce
	##
	## index pattern for len=8
	##   shift=1,iters=4: i={0,1,2,3}   i*(s*2)+(s-1)=r_lhs={0,2,4,6}    (i+1)*(s*2)-1=r_rhs={1,3,5,7}=w
	##   shift=2,iters=2: i={0,1}       i*(s*2)+(s-1)=r_lhs={    1,5}    (i+1)*(s*2)-1=r_rhs={    3,7}=w
	##   shift=4,iters=1: i={0}         i*(s*2)+(s-1)=r_lhs={      3}    (i+1)*(s*2)-1=r_rhs={      7}=w
	##
	## index pattern for len=16
	##   shift=1,iters=8: i={0,1,2,3,4,5,6,7}   i*(s*2)+(s-1)=r_lhs={0,2,4,6,8,10,12,14}    (i+1)*(s*2)-1=r_rhs={1,3,5,7,9,11,13,15}=w
	##   shift=2,iters=4: i={0,1,2,3}           i*(s*2)+(s-1)=r_lhs={        1, 5, 9,13}    (i+1)*(s*2)-1=r_rhs={        3, 7,11,15}=w
	##   shift=4,iters=2: i={0,1}               i*(s*2)+(s-1)=r_lhs={              3,11}    (i+1)*(s*2)-1=r_rhs={              7,15}=w
	##   shift=8,iters=1: i={0}                 i*(s*2)+(s-1)=r_lhs={                 7}    (i+1)*(s*2)-1=r_rhs={                15}=w
	##
	while (shift < len(rdata)):
		const = shift << 1
		iters = len(rdata) / (shift << 1)

		for i in xrange(iters):
			j = (i    ) * const + (shift - 1)
			k = (i + 1) * const - (        1)
			a = rdata[j] ## lhs
			b = rdata[k] ## rhs

			rdata[k] = oper(a, b)

		shift <<= 1

	## stage 2: downsweep
	shift >>= 1

	## reset final value to identity
	rdata[-1] = base

	while (shift > 0):
		const = shift << 1
		iters = len(rdata) / (shift << 1)

		for i in xrange(iters):
			j = (i    ) * const + (shift - 1)
			k = (i + 1) * const - (        1)
			a = rdata[j] ## lhs
			b = rdata[k] ## rhs

			rdata[j] =     (   b) ## right-to-left copy
			rdata[k] = oper(a, b) ## left-to-right oper

		shift >>= 1

	return rdata[: len(data)]



def binop_add(a, b): return (a + b)
def binop_mul(a, b): return (a * b)
## not associative, use binop_add with negative values
## def binop_sub(a, b): return (a - b)
def binop_max(a, b): return max(a, b)
def binop_min(a, b): return min(a, b)



## INPUT = [-1, -2, -3, -4, -5, -6, -7, -8]
## INPUT = [+1, +2, +3, +4]
## INPUT = [+1, +2, +3, +4, +5, +6, +7, +8]
## INPUT = [+1, +2, +3, +4, +5, +6, +7, +8, +9, +10, +11, +12, +13, +14, +15]
INPUT = [+1, +2, +3, +4, +5, +6, +7, +8, +9, +10, +11, +12, +13, +14, +15, +16]
OPERS = (binop_add, binop_mul, binop_max, binop_min)
BASES = (0, 1, -1000000, 1000000)

for i in xrange(len(OPERS)):
	assert(s_reduce(INPUT, OPERS[i], BASES[i]) == p_reduce(INPUT, OPERS[i], BASES[i]))
	assert(p_scan_hillis(INPUT, OPERS[i], BASES[i]) == p_scan(INPUT, OPERS[i], BASES[i], True))
	assert(p_scan_blelloch(INPUT, OPERS[i], BASES[i]) == p_scan(INPUT, OPERS[i], BASES[i], False))

	for f in [True, False]:
		assert(s_scan(INPUT, OPERS[i], BASES[i], f) == p_scan(INPUT, OPERS[i], BASES[i], f))



print("s_reduce(add): %d" % s_reduce(INPUT, binop_add))
print("s_reduce(mul): %d" % s_reduce(INPUT, binop_mul))
print("s_reduce(max): %d" % s_reduce(INPUT, binop_max))
print("s_reduce(min): %d" % s_reduce(INPUT, binop_min))
print("")
print("p_reduce(add): %d" % p_reduce(INPUT, binop_add))
print("p_reduce(mul): %d" % p_reduce(INPUT, binop_mul))
print("p_reduce(max): %d" % p_reduce(INPUT, binop_max))
print("p_reduce(min): %d" % p_reduce(INPUT, binop_min))
print("")
print("s_scan_excl(add): %s" % s_scan(INPUT, binop_add, BASES[0], False))
print("s_scan_excl(mul): %s" % s_scan(INPUT, binop_mul, BASES[1], False))
print("s_scan_excl(max): %s" % s_scan(INPUT, binop_max, BASES[2], False))
print("s_scan_excl(min): %s" % s_scan(INPUT, binop_min, BASES[3], False))
print("")
print("s_scan_excl(add): %s" % s_scan([3,1,4,1,5,9,6,2], binop_add, BASES[0], False))
print("p_scan_excl(add): %s" % p_scan([3,1,4,1,5,9,6,2], binop_add, BASES[0], False))
print("s_scan_incl(add): %s" % s_scan([3,1,4,1,5,9,6,2], binop_add, BASES[0], True))
print("p_scan_incl(add): %s" % p_scan([3,1,4,1,5,9,6,2], binop_add, BASES[0], True))
print("")
print("p_scan_hillis(add): %s" % p_scan_hillis(INPUT, binop_add, BASES[0]))
print("p_scan_hillis(mul): %s" % p_scan_hillis(INPUT, binop_mul, BASES[1]))
print("p_scan_blelloch(add): %s" % p_scan_blelloch(INPUT, binop_add, BASES[0]))
print("p_scan_blelloch(max): %s" % p_scan_blelloch([2,1,4,3], binop_max, BASES[2]))

