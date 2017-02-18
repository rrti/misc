import sys
import random


def get_num_digits(n, r):
	i = int(n == 0)

	while (n > 0):
		n /= r
		i += 1

	return i


def get_max_val(vals):
	max_val = 0

	for val in vals:
		max_val = max(max_val, val[1])

	return max_val

def get_max_div(n, r, k):
	if (n == 0):
		return 1

	e = 0

	while ((r ** e) <= n):
		e += 1

	e -= 1

	## no negative exponents
	assert(e >= 0)

	if (k > e):
		return 1

	return (r ** (e - k))


def radix_sort(inp_vals, num_radix, msd_shift = 0, asc_sort = True):
	if (len(inp_vals) <= 1):
		return inp_vals

	if (msd_shift > get_num_digits(get_max_val(inp_vals), num_radix)):
		return inp_vals

	## prepare buckets (one per digit)
	msd_bins = [ [] for i in xrange(num_radix) ]
	out_vals = []

	for val in inp_vals:
		## put numbers into buckets based on k-th MSD
		div = get_max_div(val[1], num_radix, msd_shift)
		idx = (val[1] / div) % num_radix

		msd_bins[idx] += [val]

	## sort each bucket recursively
	for b in msd_bins:
		out_vals += radix_sort(b, num_radix, msd_shift + 1, asc_sort)

	if (msd_shift == 0 and asc_sort == False):
		out_vals.reverse()

	return out_vals


## can not be used (numbers are not sorted by total value)
def is_sorted(inp_vals, asc_sort):
	ret = True

	if (asc_sort):
		for i in xrange(len(inp_vals) - 1):
			ret &= (inp_vals[i][1] <= inp_vals[i + 1][1])
	else:
		for i in xrange(len(inp_vals) - 1):
			ret &= (inp_vals[i][1] >= inp_vals[i + 1][1])

	return ret


def main(argc, argv):
	max_val  = (argc > 1 and     (int(argv[1]))) or 9999
	num_vals = (argc > 2 and     (int(argv[2]))) or 10
	asc_sort = (argc > 3 and bool(int(argv[3]))) or False

	inp_vals = [(i, random.randint(0, max_val)) for i in xrange(num_vals)]
	out_vals = radix_sort(inp_vals, 10, 0, asc_sort)

	print("[main]\n\tinp_vals=%s\n\tout_vals=%s" % (inp_vals, out_vals))
	return 0

sys.exit(main(len(sys.argv), sys.argv))

