import sys

M_NONE					= (  0, "NONE")
M_PARSE_SECTION_NAME	= (  1, "PARSE_SECTION_NAME")
M_PUSH_SECTION_NAME		= (  2, "PUSH_SECTION_NAME")
M_PUSH_SECTION			= (  4, "PUSH_SECTION")
M_PARSE_SECTION			= ( 16, "PARSE_SECTION")
M_COMMENT_SL			= ( 32, "COMMENT_SL")
M_COMMENT_ML			= ( 64, "COMMENT_ML")
M_PARSE_SECTION_KEY		= (128, "PARSE_SECTION_KEY")
M_PARSE_SECTION_VAL		= (256, "PARSE_SECTION_VAL")

TOKENS = "{}[];=/*\n"

def print_stack(stack):
	s = ""
	for m in stack: s += (m[1] + ' ')
	return s

def parse(s):
	s = s.replace('\r', '\n')
	s = s.replace('\t', ' ')

	i = 0
	j = 0
	k = len(s)

	sec = ""
	key = ""
	val = ""

	pairs = {}
	sStack = []
	mStack = [M_NONE]

	while (i < k):
		j = i + 1
		c = s[i].lower()
		m = mStack[-1][0]
		t = TOKENS.find(c)

		## if (c == '\n'): print("%s, '%s'" % (print_stack(mStack), "\\n"))
		## else: print("%s, '%c'" % (print_stack(mStack), c))

		if (t >= 0):
			if (c == '['):
				if (m == M_NONE[0] or m == M_PARSE_SECTION[0]):
					mStack.append(M_PARSE_SECTION_NAME)
				else:
					print("[parse] syntax error: stray '[' encountered")
			if (c == ']'):
				if (m == M_PARSE_SECTION_NAME[0]):
					del mStack[-1]

					sStack.append(sec)
					pairs[sec] = {}
					sec = ""
				else:
					print("[parse] syntax error: stray ']' encountered")

			if (c == '{'):
				if (m == M_NONE[0] or m == M_PARSE_SECTION[0]):
					## we want to read keys or section names now
					mStack.append(M_PARSE_SECTION)
				else:
					print("[parse] syntax error: stray '{' encountered")
			if (c == '}'):
				if (m == M_PARSE_SECTION[0]):
					del sStack[-1]
					del mStack[-1]
				else:
					print("[parse] syntax error: stray '}' encountered")


			if (c == '='):
				if (m == M_PARSE_SECTION_KEY[0]):
					## we want to read section val now
					del mStack[-1]
					mStack.append(M_PARSE_SECTION_VAL)
				else:
					if (m != M_COMMENT_SL[0] and m != M_COMMENT_ML[0]):
						print("[parse] syntax error: stray '=' encountered")
			if (c == ';'):
				if (m == M_PARSE_SECTION_VAL[0]):
					del mStack[-1]

					pairs[sStack[-1]][key] = val
					key = ""
					val = ""
				else:
					if (m != M_COMMENT_SL[0] and m != M_COMMENT_ML[0]):
						print("[parse] syntax error: stray ';' encountered")


			if (c == '/'):
				## '/' can occur in values too
				if (m == M_PARSE_SECTION_VAL[0]):
					val += c

				elif (i < k - 1):
					if (s[j] == '/'):
						if (m != M_COMMENT_SL[0] and m != M_COMMENT_ML[0]):
							mStack.append(M_COMMENT_SL)

					if (s[j] == '*'):
						if (m != M_COMMENT_SL[0] and m != M_COMMENT_ML[0]):
							mStack.append(M_COMMENT_ML)

			if (c == '*'):
				if (i < k - 1 and s[j] == '/'):
					if (m == M_COMMENT_ML[0]):
						del mStack[-1]
					else:
						print("[parse] syntax error: stray */ comment terminator encountered")

			if (c == '\n'):
				if (m == M_COMMENT_SL[0]):
					del mStack[-1]

		else:
			## char is not a token
			if (mStack[-1][0] == M_PARSE_SECTION[0] and c != ' '):
				mStack.append(M_PARSE_SECTION_KEY)

			if (mStack[-1][0] == M_PARSE_SECTION_NAME[0]): sec += c
			if (mStack[-1][0] == M_PARSE_SECTION_KEY[0]): key += c
			if (mStack[-1][0] == M_PARSE_SECTION_VAL[0]): val += c

		i += 1

	return pairs


def log(pairs):
	s = ""

	for sectionName in pairs.keys():
		s += ("%s\n" % sectionName)

		for key in pairs[sectionName]:
			s += ("\t\"%s\" = \"%s\"\n" % (key, pairs[sectionName][key]))

	return s


def parse_file(fname):
	f = open(fname, 'r')
	s = log(parse(f.read()))
	f = f.close()
	print(s)

def main(argc, argv):
	if (argc <= 1):
		print("usage: python %s [filename]" % sys.argv[0])
	else:
		for arg in argv[1: ]:
			parse_file(arg)

	return 0

sys.exit(main(len(sys.argv), sys.argv))

