##
## multiplicative formula: (N over K) = PI_{i=1,K} ((n - (k - i)) / i) = PI_{i=1,K} ((n + 1 - i) / i)
##
def binomialCoefficient(n, k):
	if (k < 0 or k > n):
		return 0
	if (k == 0 or k == n):
		return 1

	## take advantage of symmetry
	k = min(k, n - k)
	c = 1

	if (False):
		for i in xrange(k):
			c = (c * (n - i)) / (i + 1)
	else:
		for i in xrange(1, k + 1):
			c = (c * (n + 1 - i)) / i

	return c

print("%d" % binomialCoefficient(10, 2))
print("%d" % binomialCoefficient(11, 3))
print("%d" % binomialCoefficient(12, 4))
print("%d" % binomialCoefficient(52, 5))

