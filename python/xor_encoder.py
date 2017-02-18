import sys
import os

def xor(key, inp, out):
	assert(len(inp) == len(out))

	for i in xrange(len(inp)):
		c = ord(inp[i])
		k = ord(key[i % len(key)])
		v = c ^ k

		assert(v >= 0)
		assert(v <= 255)
		out[i] = chr(v)

def parse_arg(arg):
	ret = None

	if (os.path.isfile(arg)):
		## file
		hnd = open(arg, "rb")
		inp = hnd.read()
		ret = list(inp)
		hnd = hnd.close()
	else:
		## literal string
		ret = list(arg)

	return ret

def main(argc, argv):
	if (argc != 3):
		print("usage: python %s <file> <key>" % argv[0])
		return -1

	inp = parse_arg(argv[1])
	key = parse_arg(argv[2])
	out = [0] * len(inp)

	if (len(inp) == 0): return -1
	if (len(key) == 0): return -1

	## <out> = xor(<inp>, <key>)
	xor(key, inp, out)

	## dump the encoded data
	hnd = open(argv[1] + ".xor", "wb")
	hnd.write("".join(out))
	hnd.close()
	return 0

sys.exit(main(len(sys.argv), sys.argv))

