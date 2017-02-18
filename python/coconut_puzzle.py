## K=5 guys, each gives one coconut from pile to monkey
## and takes 1/K-th of the remainder, and then one last
## coconut is removed after which the pile is divisible
## by K and split evenly --> we need to reach a depth of
## K + 1

## recursive
def fr(i, d):
	n = i - 1

	if (d >= 6): return True
	if ((n % 5) != 0): return False

	return (fr(n - (n / 5), d + 1))

## iterative
def fi(i, d):
	n = 0

	while (n < 6):
		if (((i - 1) % 5) != 0):
			break

		i = i - 1
		i = i - (i / 5)
		n += 1

	return (n >= 6)
		

## find the smallest initial amount of nuts
for i in xrange(100000):
	if (fi(i, 0)):
		print("%d" % i)
		break

