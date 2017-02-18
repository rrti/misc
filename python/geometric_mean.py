## the geometric mean is defined as the n-th root (where n
## is the count of numbers) of the product of the numbers
##
## the geometric mean applies only to positive numbers, but
## zero counts as positive so it must be treated separately
##
def geometric_mean(numbers):
	total = 1
	zeros = 0

	assert(len(numbers) > 0)

	for n in numbers:
		assert(n >= 0)

		if (n == 0):
			zeros += 1
		else:
			total *= n

	root = len(numbers) - zeros
	mean = total ** (1.0 / root)

	return mean

print(geometric_mean([2.0, 8.0]))
print(geometric_mean([4.0, 1.0, 1.0 / 32.0]))
