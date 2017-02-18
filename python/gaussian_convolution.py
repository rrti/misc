import sys

def blur(inp_data, kernel):
	assert(len(inp_data) != 0)
	assert((len(kernel) & 1) == 1)

	## note: hwidth assumes an odd width
	width = len(kernel)
	hwidth = width >> 1

	## copy input data
	out_data = [inp_data[i] for i in xrange(len(inp_data))]

	## move kernel stencil across the input data
	for i in xrange(hwidth, len(inp_data) - hwidth):
		out_value = 0.0

		## apply stencil to local neighborhood
		for j in xrange(width):
			out_value += (kernel[j] * inp_data[i + (j - hwidth)])

		out_data[i] = out_value

	return out_data

def sum_squared_difference(inp_data, out_data):
	assert(len(inp_data) == len(out_data))
	assert(len(inp_data) !=             0)

	sum_sqr_diff = 0.0

	for i in xrange(len(inp_data)):
		sum_sqr_diff += (inp_data[i] - out_data[i]) ** 2.0

	return (sum_sqr_diff / len(inp_data))

def main(argc, argv):
	kernel = [1.0, 2.0, 4.0,  8.0,  4.0, 2.0, 1.0]
	weight = 1.0 / sum(kernel)
	kernel = [kernel[i] * weight for i in xrange(len(kernel))]
	passes = ((argc > 1) and int(argv[1])) or 1

	## cur_heights = [ 0.0,  0.0,  0.0,  0.0,  0.0, 0.0, 0.0, 0.0, 0.0,  10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0]
	## cur_heights = [10.0, 10.0, 10.0, 10.0,  0.0, 0.0, 0.0, 0.0, 0.0,  10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0]
	cur_heights = [-10.0, -10.0, -10.0, -10.0, -10.0,  10.0, 10.0, 10.0, 10.0, 10.0]
	nxt_heights = None

	for i in xrange(passes):
		print("[main][pre] pass=%d heights=%s" % (i, cur_heights))

		nxt_heights = blur(cur_heights, kernel)

		## bail when the blur operation no longer changes the data
		if (sum_squared_difference(nxt_heights, cur_heights) < 0.001):
			break

		cur_heights = nxt_heights

		print("[main][pst] pass=%d heights=%s" % (i, nxt_heights))
		print("")

	return 0;

sys.exit(main(len(sys.argv), sys.argv))

