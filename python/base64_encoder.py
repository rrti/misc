import sys

BASE64_SYMBOLS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

def encode(dec_str, padd = False):
	enc_str = ""
	left    = 0
	max_idx = len(dec_str)

	for i in xrange(0, max_idx):
		if (left == 0):
			enc_str += BASE64_SYMBOLS[ord(dec_str[i]) >> 2]
			left = 2
		else:
			if left == 6:
				enc_str += BASE64_SYMBOLS[ord(dec_str[i - 1]) & 63]
				enc_str += BASE64_SYMBOLS[ord(dec_str[i    ]) >> 2]
				left = 2
			else:
				index1 = ord(dec_str[i - 1]) & ((1 << left) - 1)
				index2 = ord(dec_str[i    ]) >> (left + 2)
				index = (index1 << (6 - left)) | index2
				enc_str += BASE64_SYMBOLS[index]
				left += 2

	if (left != 0):
		enc_str += BASE64_SYMBOLS[(ord(dec_str[max_idx - 1]) & ((1 << left) - 1)) << (6 - left)]

	if (padd):
		enc_strLen = len(enc_str)

		for i in xrange(0, (4 - enc_strLen % 4) % 4):
			enc_str += "="

	return enc_str

def decode(enc_str):
	dec_str = ""
	enc_str = enc_str.replace("=", "")
	left    = 0
	max_idx = len(enc_str)

	for i in xrange(0, max_idx):
		if (left == 0):
			left = 6
		else:
			value1 = BASE64_SYMBOLS.index(enc_str[i - 1]) & ((1 << left) - 1)
			value2 = BASE64_SYMBOLS.index(enc_str[i    ]) >> (left - 2)
			value = (value1 << (8 - left)) | value2
			dec_str += chr(value)
			left -= 2

	return dec_str




def main(argc, argv):
	if (argc == 3):
		if (argv[1] == "-e"):
			print("%s" % encode(argv[2]))
		elif (argv[1] == "-d"):
			print("%s" % decode(argv[2]))

		assert(decode(encode(argv[2])) == argv[2])
	else:
		print("\nusage: python %s [-e | -d][str to encode | str to decode]\n" % argv[0])

	return 0

sys.exit(main(len(sys.argv), sys.argv))

