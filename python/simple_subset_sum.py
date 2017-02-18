import sys

def sum_subsets(values):
	neg_sum = 0
	pos_sum = 0

	## separately sum the positive and negative values
	for v in values:
		neg_sum += (v * (v < 0))
		pos_sum += (v * (v > 0))

	table = [{} for i in xrange(len(values))]

	## base case for the recursive construction
	for s in xrange(neg_sum, pos_sum + 1):
		table[0][s] = (values[0] == s)

	## recursive case
	for i in xrange(1, len(values)):
		v = values[i]

		for s in xrange(neg_sum, pos_sum + 1):
			## check if (s - v) is a theoretically possible subset-sum
			above_min_bound = ((s - v) >= neg_sum)
			below_max_bound = ((s - v) <= pos_sum)
			in_table_bounds = above_min_bound and below_max_bound

			table[i][s] = ((v == s) or table[i - 1][s] or (in_table_bounds and table[i - 1][s - v]))

	## 0 is always in the range [neg_sum, pos_sum]
	assert(0 in table[len(values) - 1])
	return table

def has_subset(sums_table, subset_sum = 0):
	sums = sums_table[len(sums_table) - 1]
	smin = min(sums.keys())
	smax = max(sums.keys())

	assert(subset_sum >= smin)
	assert(subset_sum <= smax)
	return (sums[subset_sum])


def main(argc, argv):
	if (argc < 2):
		return 0

	vals = [int(argv[i]) for i in xrange(1, argc)]
	sums = sum_subsets(vals)
	ssum = 0

	print("[main] has_subset(%s, sum=%d)=%d" % (vals, ssum, has_subset(sums, ssum)))
	return 0

sys.exit(main(len(sys.argv), sys.argv))

