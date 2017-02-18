import random

l1 = [9, 5, 2, 8, 7, 3, 1, 6, 4]
l2 = [53, 39, 19, 51, 29, 70, 7, 26, 66, 59, 64, 37, 80, 81, 33, 86, 62, 87, 27, 12]
l3 = [1, 3, 2, 4, 3, 5, 4, 6]
l4 = [int(random.random() * 100) for i in xrange(10)]


## find the maximum element
## in a list and its index
def FindMax(s):
	n = len(s)
	k = 0
	m = 0

	for i in xrange(n):
		if (s[i] > m):
			m = s[i]
			k = i

	return (m, k)

def Trace(prev, seq, t):
	## traces back the LIS
	l = []
	k = t[1]

	while (k >= 0):
		l.append(seq[k])
		if (prev[k] == k):
			break
		k = prev[k]

	l.reverse()
	return l



def LongestIncreasingSubRowAlt(seq):
	## NOTE:
	##   this formulation does *not* store any back-pointers,
	##   so the longest increasing subsequence is impossible
	##   to reconstruct (only its length and at which element
	##   in <seq> it ends)
	##
	n = len(seq)
	best = [0] * n

	for i in xrange(0, n):
		best[i] = 1
		for j in xrange(0, i):
			if (seq[i] > seq[j]):
				best[i] = max(best[i], best[j] + 1)

	t = FindMax(best)



def LongestIncreasingSubRow(seq):
	N = len(seq)
	best = [1] * N   ## stores for each s[i] length of LIS ending at i
	prev = range(N)  ## for backtracking the LIS
	m = 0

	for i in range(1, N):
		for j in range(0, i):
			if (seq[i] > seq[j] and best[i] < best[j] + 1):
				best[i] = best[j] + 1
				prev[i] = j

	## get length of LIS and index
	## in sequence S where it ends
	t = FindMax(best)
	l = Trace(prev, seq, t)

	print("[LongestIncreasingSubRow]")
	print("\tsequ: %s" % seq)
	print("\tindx: %s" % range(N))
	print("\tbest: %s" % best)
	print("\tprev: %s" % prev)
	print("\tLIS:  %s" % l)
	print("")

LongestIncreasingSubRow(l1)
LongestIncreasingSubRow(l2)
LongestIncreasingSubRow(l3)
LongestIncreasingSubRow(l4)

