"""
  BrainTeaser for Amazon interview

  Coding exercise:

  Every athlete is characterized by his mass 'm' (in kg) and strength 's' (in kg).
  You are to find the maximum number of athletes that can form a tower standing one
  upon another. An athlete can hold a tower of athletes with total mass less than or
  equal to his strength. Input contains the number of athletes n and their parameters.
  These inputs can be assumed to be passed as arguments (Integer n and parameterList)
  appropriate for your language of choice: For example:

    n  m_1 s_1  m_2 s_2  ...  m_n s_n

  If m_i > m_j then s_i > s_j, but athletes with equal masses can be of different
  strength. Number of athletes n < 100000. Masses and strengths are all positive
  integers less than 2000000. For example an input of "4   3 4   2 2   7 6   4 5"
  would yield an output of 3.
"""


import sys
import random

MAX_ATHLETES =  100000
MAX_ATTR_VAL = 2000000

DATA_STRINGS = [
	"4   3 4   2 2   7 6   4 5",
	"6   1 1   1 1   1 1   1 1   1 1   5 5",
	"6   1 1   1 1   1 1   4 2   1 1   6 6",
	"1000 " + (" 1 1 " * 999) + " 5000 5000",
]


def gen_ex_data():
	data = "%d " % MAX_ATHLETES

	## note: this violates the "if m_i > m_j then s_i > s_j" constraint
	## technically we would have to filter out every strictly dominated
	## athlete before using these random values as input
	for i in xrange(MAX_ATHLETES):
		m = random.randint(1, MAX_ATTR_VAL)
		s = random.randint(1, MAX_ATTR_VAL)
		data += ("%d %d " % (m, s))

	return data

def parse_ex_data(inp_data):
	inp_data = inp_data.split()
	out_data = [None] * int(inp_data[0])

	assert((len(inp_data) - 1) == (len(out_data) * 2))

	for i in xrange(1, len(inp_data), 2):
		m = int(inp_data[i    ])
		s = int(inp_data[i + 1])

		out_data[i >> 1] = (m, s, i)

	def cmp_data_func(a, b):
		## first compare athletes by strength
		if (a[1] < b[1]): return -1
		if (a[1] > b[1]): return  1
		## if strengths are equal, compare by mass
		if (a[0] < b[0]): return -1
		if (a[0] > b[0]): return  1
		return 0

	## re-order from weakest-lightest to strongest-heaviest
	out_data.sort(cmp = cmp_data_func)
	return out_data


def calc_tower_size(athlete_data, athletes_set,  twr_data,  twr_mass, twr_size):
	athlete_idx = ((len(athletes_set) > 0 and (max(athletes_set) + 1)) or 0)

	if (False):
		while (athlete_idx < len(athlete_data)):
			athlete_tup = athlete_data[athlete_idx]
			athlete_idx += 1

			## check if this athlete has enough strength to
			## hold the tower built up so far, then recurse
			##
			## too slow by many orders of magnitude, because
			## it generates every *possible* tower including
			## suboptimal ones
			if (athlete_tup[1] < twr_mass):
				continue
			if ((athlete_idx - 1) in athletes_set):
				continue

			athletes_set.add(athlete_idx - 1)
			calc_tower_size(athlete_data, athletes_set,  twr_data,  twr_mass + athlete_tup[0], twr_size + 1)
			athletes_set.remove(athlete_idx - 1)

		## NOTE:
		##   if multiple maximal-length towers can be built,
		##   mass will correspond to the *first* one we find
		if (twr_size > twr_data[1]):
			twr_data[0] =    (             twr_mass)
			twr_data[1] = max(twr_data[1], twr_size)
	else:
		## repeatedly pick the weakest-lightest athlete that can hold the
		## partial tower built so far (from the pre-sorted athlete array)
		## the construction is top-down, other way around does not work
		while (athlete_idx < len(athlete_data)):
			athlete_tup = athlete_data[athlete_idx]
			athlete_idx += 1

			if (athlete_tup[1] < twr_mass):
				continue

			twr_mass += athlete_tup[0]
			twr_size += 1

		twr_data[0] = twr_mass
		twr_data[1] = twr_size


def main(argc, argv):
	if (argc == 2):
		data = DATA_STRINGS[ int(argv[1]) % len(DATA_STRINGS) ]
	else:
		data = gen_ex_data()

	twr_data = [0, 0]
	calc_tower_size(parse_ex_data(data), set(), twr_data, 0, 0)

	print("[main] tower=(size=%d :: mass=%d)" % (twr_data[1], twr_data[0]))
	return 0

sys.exit(main(len(sys.argv), sys.argv))

