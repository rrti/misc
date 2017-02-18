import sys



"""
A non-empty zero-indexed array A consisting of N integers is given.
The leader of this array is the value that occurs in more than half
of the elements of A.

You are given an implementation of a function:

	def solution(A) 

that, given a non-empty zero-indexed array A consisting of N integers,
returns the leader of array A. The function should return -1 if array
A does not contain a leader.

For example, given array A consisting of ten elements such that:

	A[0] = 4
	A[1] = 2
	A[2] = 2
	A[3] = 3
	A[4] = 2
	A[5] = 4
	A[6] = 2
	A[7] = 2
	A[8] = 6
	A[9] = 4

the function should return -1, because the value that occurs most frequently
in the array, 2, occurs five times, and 5 is not more than half of 10.

Given array A consisting of five elements such that:

	A[0] = 1
	A[1] = 1
	A[2] = 1
	A[3] = 50
	A[4] = 1

the function should return 1.
"""

def solution_array_leader(A):
	N = len(A)
	M = N / 2
	T = {}

	## construct table of counts (O(N * log N))
	for i in xrange(N):
		v = A[i]

		if (v in T):
			T[v] += 1
		else:
			T[v] = 1

	kmax = -1
	vmax =  0

	## find maximum count (O(N)) and corresponding value
	for val, cnt in T.iteritems():
		if (cnt > vmax):
			kmax = val
			vmax = cnt

	return ((vmax > M and kmax) or -1)

assert(-1 == solution_array_leader([4, 2, 2, 3, 2, 4, 2, 2, 6, 4]))
assert( 1 == solution_array_leader([1, 1, 1, 50]))






"""
A zero-indexed array A consisting of N integers is given.
An equilibrium index of this array is any integer P such
that 0 <= P < N and the sum of elements of lower indices
is equal to the sum of elements of higher indices, i.e.

    A[0] + A[1] + ... + A[P-1] = A[P+1] + ... + A[N-2] + A[N-1].

Sum of zero elements is assumed to be equal to 0. This
can happen if P = 0 or if P = N - 1.

For example, consider the following array A consisting
of N = 8 elements:

  A[0] = -1
  A[1] =  3
  A[2] = -4
  A[3] =  5
  A[4] =  1
  A[5] = -6
  A[6] =  2
  A[7] =  1

P = 1 is an equilibrium index of this array, because:

	A[0] = -1 = A[2] + A[3] + A[4] + A[5] + A[6] + A[7]

P = 3 is an equilibrium index of this array, because:

	A[0] + A[1] + A[2] = -2 = A[4] + A[5] + A[6] + A[7]

P = 7 is also an equilibrium index, because:

	A[0] + A[1] + A[2] + A[3] + A[4] + A[5] + A[6] = 0

and there are no elements with indices greater than 7.

P = 8 is not an equilibrium index, because it does not
fulfill the condition 0 <= P < N.

Write a function:

	def solution(A)

that, given a zero-indexed array A consisting of N
integers, returns any of its equilibrium indices.
The function should return -1 if no equilibrium
index exists.

For example, given array A shown above, the function
may return 1, 3 or 7, as explained above.

Assume that:

	N is an integer within the range [0..100,000];
	each element of array A is an integer within the
	range [-2,147,483,648 .. 2,147,483,647].

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(N),
	beyond input storage (not counting the storage
	required for input arguments).

Elements of input arrays can be modified.
"""

def solution_equilibrium_index(A):
	N = len(A)
	L = [0] * N
	R = [0] * N

	ls =  0
	rs =  0
	ei = -1

	## ls is the cumulative sum of values from 0   up to index i
	## rs is the cumulative sum of values from N down to index k
	## index i runs left to right, k runs right to left, cumulative
	## sums are written to corresponding arrays (at indices i and k)
	##
	## wherever these arrays have an element of equal value *at the
	## same index* an equilibrium position exists, return the first
	for i in xrange(N):
		k = N - i - 1

		ls += A[i]
		rs += A[k]
		L[i] = ls
		R[k] = rs
		## alternative indexing
		## R[i] = rs

	for i in xrange(N):
		## alternative indexing
		## if (L[i] == R[N - i - 1]):
		## see if we have a match, save the first
		if (L[i] == R[i]):
			ei = i
			break

	return ei

assert(1 == solution_equilibrium_index([1, 2, 1]))
assert(1 == solution_equilibrium_index([-1, 3, -4, 5, 1, -6, 2, 1]))
assert(1 == solution_equilibrium_index([1082132608, 0, 1082132608]))
assert(3 == solution_equilibrium_index([1, 2, -3, 0]))






"""
A small frog wants to get to the other side of a river.
The frog is currently located at position 0, and wants
to get to position X. Leaves fall from a tree onto the
surface of the river.

You are given a non-empty zero-indexed array A consisting
of N integers representing the falling leaves. A[K] represents
the position where one leaf falls at time K, measured in
seconds.

The goal is to find the earliest time when the frog can
jump to the other side of the river. The frog can cross
only when leaves appear at every position across the river
from 1 to X. You may assume that the speed of the current
in the river is negligibly small, i.e. the leaves do not
change their positions once they fall in the river.

For example, you are given integer X = 5 and array A such
that:

	A[0] = 1
	A[1] = 3
	A[2] = 1
	A[3] = 4
	A[4] = 2
	A[5] = 3
	A[6] = 5
	A[7] = 4

In second 6, a leaf falls into position 5. This is the
earliest time when leaves appear in every position across
the river.

Write a function:

	def solution(X, A)

that, given a non-empty zero-indexed array A consisting of N
integers and integer X, returns the earliest time when the
frog can jump to the other side of the river.

If the frog is never able to jump to the other side of the
river, the function should return -1.

For example, given X = 5 and array A such that:

	A[0] = 1
	A[1] = 3
	A[2] = 1
	A[3] = 4
	A[4] = 2
	A[5] = 3
	A[6] = 5
	A[7] = 4

the function should return 6, as explained above.

Assume that:

	N and X are integers within the range [1..100,000];
	each element of array A is an integer within the range
	[1..X].

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(X), beyond
	input storage (not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

def solution_frog_jump(X, A):
	N = len(A)
	K = sys.maxint
	L = [K] * (X + 1)

	## collect minimum times at which leaves appear in a position
	## (we only care about positions less than or equal to X here)
	for time in xrange(N):
		pos = A[time]

		if (pos <= X):
			L[pos] = min(L[pos], time)

	## now we know position <pos> first has a leaf at time L[pos];
	## all intermediate positions must have at least one leaf (i.e.
	## a valid associated time)
	for pos in xrange(1, X + 1):
		if (L[pos] == K):
			return -1

	return (max(L[1: ]))

assert(6 == solution_frog_jump(5, [1, 3, 1, 4, 2, 3, 5, 4]))






"""
A non-empty zero-indexed array A consisting of N
integers is given.

A permutation is a sequence containing each element
from 1 to N once, and only once.

For example, array A such that:

	A[0] = 4
	A[1] = 1
	A[2] = 3
	A[3] = 2

is a permutation, but array A such that:

	A[0] = 4
	A[1] = 1
	A[2] = 3

is not a permutation, because value 2 is missing.

The goal is to check whether array A is a permutation.

Write a function:

	def solution(A)

that, given a zero-indexed array A, returns 1 if array
A is a permutation and 0 if it is not.

For example, given array A such that:

	A[0] = 4
	A[1] = 1
	A[2] = 3
	A[3] = 2

the function should return 1.

Given array A such that:

	A[0] = 4
	A[1] = 1
	A[2] = 3

the function should return 0.

Assume that:

	N is an integer within the range [1..100,000];
	each element of array A is an integer within the range
	[1..1,000,000,000].

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(N), beyond
	input storage (not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

def solution_perm_check(A):
	N = len(A)
	T = {}

	for i in xrange(N):
		v = A[i]
		if (v in T):
			T[v] += 1
		else:
			T[v] = 1

	for i in xrange(1, N + 1):
		if ((not (i in T)) or T[i] > 1):
			return 0

	return 1






"""
Write a function:

	def solution(A)

that, given a non-empty zero-indexed array A of N integers, returns
the minimal positive integer (greater than 0) that does not occur in
A.

For example, given:
	A[0] = 1
	A[1] = 3
	A[2] = 6
	A[3] = 4
	A[4] = 1
	A[5] = 2

the function should return 5.

Assume that:

	N is an integer within the range [1..100,000];
	each element of array A is an integer within the range
	[-2,147,483,648..2,147,483,647].

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(N), beyond
	input storage (not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

def solution_missing_integer_dict(A):
	N = len(A)
	T = {}

	for i in xrange(N):
		T[ A[i] ] = i

	## only need to check the positive integers
	for i in xrange(1, 1 << 31):
		if (not (i in T)):
			return i

	return -1

def solution_missing_integer(A):
	N = len(A)
	Z = [False] * (N + 1)

	## there are N integers in the input-array A; if A
	## does NOT contain [1, 2, 3, ...] then *at least*
	## one number from the range [1, N] must be missing
	##
	for i in xrange(N):
		v = A[i]

		## map values [1,2,...,N+1] to indices [0,1,...,N]
		if ((v > 0) and (v <= (N + 1))):
			Z[v - 1] = True

	for i in xrange(N + 1):
		if (not Z[i]):
			return (i + 1)

	return -1

assert(5 == solution_missing_integer([1, 3, 6, 4, 1, 2]))





"""
You are given N counters, initially set to 0, and you have
two possible operations on them:

	increase(X) - counter X is increased by 1,
	max counter - all counters are set to the maximum value
	of any counter.

A non-empty zero-indexed array A of M integers is given.
This array represents consecutive operations:

	if A[K] = X, such that 1 <= X <= N, then operation K is
	increase(X), if A[K] = N + 1 then operation K is max
	counter.

For example, given integer N = 5 and array A such that:

	A[0] = 3
	A[1] = 4
	A[2] = 4
	A[3] = 6
	A[4] = 1
	A[5] = 4
	A[6] = 4

the values of the counters after each consecutive operation will be:

	(0, 0, 1, 0, 0)
	(0, 0, 1, 1, 0)
	(0, 0, 1, 2, 0)
	(2, 2, 2, 2, 2)
	(3, 2, 2, 2, 2)
	(3, 2, 2, 3, 2)
	(3, 2, 2, 4, 2)

The goal is to calculate the value of every counter after all operations.

Write a function:

	def solution(N, A)

that, given an integer N and a non-empty zero-indexed array
A consisting of M integers, returns a sequence of integers
representing the values of the counters.

The sequence should be returned as:

	a structure Results (in C), or
	a vector of integers (in C++), or
	a record Results (in Pascal), or
	an array of integers (in any other programming language).

For example, given:

	A[0] = 3
	A[1] = 4
	A[2] = 4
	A[3] = 6
	A[4] = 1
	A[5] = 4
	A[6] = 4

the function should return [3, 2, 2, 4, 2], as explained above.

Assume that:

	N and M are integers within the range [1..100,000];
	each element of array A is an integer within the range
	[1..N + 1].

Complexity:

	expected worst-case time complexity is O(N+M);
	expected worst-case space complexity is O(N), beyond
	input storage (not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

def solution_max_counters(N, A):
	## initial counter values
	C = [0] * N
	## current maximum counter value
	m = 0
	## offset after most recent max-op
	o = 0

	for i in xrange(len(A)):
		v = A[i]

		if (v >= 1 and v <= N):
			## increase-op, first apply offset (if any)
			C[v - 1] = max(C[v - 1], o)
			C[v - 1] += 1

			m = max(m, C[v - 1])
		elif (v == (N + 1)):
			## max-op, update offset
			o = m

	## apply offset to untouched counters
	for i in xrange(N):
		C[i] = max(C[i], o)

	return C

assert([3, 2, 2, 4, 2] == solution_max_counters(5, [3, 4, 4, 6, 1, 4, 4]))






"""
An integer N is given, representing the area of some rectangle.

The area of a rectangle whose sides are of length A and B is
A * B, and the perimeter is 2 * (A + B).

The goal is to find the minimal perimeter of any rectangle
whose area equals N. The sides of this rectangle should be
only integers.

For example, given integer N = 30, rectangles of area 30 are:

	(1, 30), with a perimeter of 62,
	(2, 15), with a perimeter of 34,
	(3, 10), with a perimeter of 26,
	(5, 6), with a perimeter of 22.

Write a function:

	def solution(N)

that, given an integer N, returns the minimal perimeter of any
rectangle whose area is exactly equal to N.

For example, given an integer N = 30, the function should return
22, as explained above.

Assume that:

	N is an integer within the range [1..1,000,000,000].

Complexity:

	expected worst-case time complexity is O(sqrt(N));
	expected worst-case space complexity is O(1).
"""

def solution_min_perimeter_rectangle(N):
	## maximum perimeter occurs in the (A=1)*(B=N) case
	M = 1 * 2 + N * 2
	K = int(N ** 0.5) + 1

	for i in xrange(1, K):
		if ((N % i) == 0):
			A = i
			B = N / i
			P = 2 * (A + B)
			M = min(M, P)

	return M

assert(   4 == solution_min_perimeter_rectangle(   1))
assert( 204 == solution_min_perimeter_rectangle( 101))
assert(1238 == solution_min_perimeter_rectangle(1234))






"""
Write a function:

	def solution(A, B, K)

that, given three integers A, B and K, returns the number of
integers within the range [A..B] that are divisible by K, i.e.:

	{ i : A <= i <= B, i mod K = 0 }

For example, for A = 6, B = 11 and K = 2, your function should
return 3, because there are three numbers divisible by 2 within
the range [6..11], namely 6, 8 and 10.

Assume that:

	A and B are integers within the range [0..2,000,000,000];
	K is an integer within the range [1..2,000,000,000];
	A <= B.

Complexity:

	expected worst-case time complexity is O(1);
	expected worst-case space complexity is O(1).
"""

def solution_count_div_linear(A, B, K):
	N = 0
	for i in xrange(A, B + 1):
		N += (int((i % K) == 0))
	return N

def solution_count_div(A, B, K):
	R = A % K
	N = A - R
	Z = (B - N) / K

	## case 1: ... K ... [A ... B] (K <= A, A >= 1, B >= A)
	## case 2: ... [A ... B] ... K (K > B)
	## case 3: ... [A ... K ... B] (A <= K <= B)
	##
	## let
	##   a = A % K
	## then
	##   A = (a + r * K) and (A - A % K) = (a + r*K - a) = (r * K)
	##
	## the number of multiples of K in [A, B] is equal to the number
	## of multiples of K in [r*K, B] if (a = 0), i.e. when K divides
	## M, and one less if (a > 0), i.e. when K does not divide M
	##
	## the number of multiples of K in [r * K, B] is equal to the
	## number of multiples of K in the interval [0, B - r * K] or
	## ((B - r * K) / K) + 1
	##
	## return (((B / K) - (A / K)) + int((A % K) == 0))
	return (Z + int((A % K) == 0))

assert( 3 == solution_count_div( 6,  11,  2))
assert( 1 == solution_count_div( 6,  11,  4))
assert( 1 == solution_count_div( 6,  11,  5))
assert( 1 == solution_count_div( 6,  11,  9))
assert( 2 == solution_count_div( 6,  19,  9))
assert( 5 == solution_count_div(11,  99, 17))
assert(20 == solution_count_div(11, 345, 17))

assert( 1 == solution_count_div(10,  10,  5))
assert( 0 == solution_count_div(10,  10,  7))
assert( 0 == solution_count_div(10,  10, 20))

assert(2000000001 == solution_count_div(0, 2000000000, 1))






"""
A non-empty zero-indexed array A consisting of N integers is given.
The consecutive elements of array A represent consecutive cars on a
road.

Array A contains only 0s and/or 1s:

	0 represents a car traveling east,
	1 represents a car traveling west.

The goal is to count passing cars. We say that a pair of cars (P, Q),
where 0 <= P < Q < N, is passing when P is traveling to the east and
Q is traveling to the west.

For example, consider array A such that:

	A[0] = 0
	A[1] = 1
	A[2] = 0
	A[3] = 1
	A[4] = 1

We have five pairs of passing cars: (0, 1), (0, 3), (0, 4), (2, 3), (2, 4).

Write a function:

	def solution(A)

that, given a non-empty zero-indexed array A of N integers, returns
the number of pairs of passing cars.

The function should return -1 if the number of pairs of passing cars
exceeds 1,000,000,000.

For example, given:

	A[0] = 0
	A[1] = 1
	A[2] = 0
	A[3] = 1
	A[4] = 1

the function should return 5, as explained above.

Assume that:

	N is an integer within the range [1..100,000];
	each element of array A is an integer that can
	have one of the following values: 0, 1.

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(1), beyond
	input storage not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

def solution_passing_cars(A):
	N = len(A)
	L = [0] * N
	n = 0
	s = 0

	## for each east-bound car, count the number of
	## west-bound cars it will encounter; the total
	## (cumulative) amount of encounters equals the
	## number of pairs
	for i in xrange(N):
		k = N - i - 1
		n += int(A[k] == 1)

		if (A[k] == 0):
			L[k] = n
			s += n

	assert(s == sum(L))
	if (s > 1000000000):
		return -1

	return s

assert(5 == solution_passing_cars([0, 1, 0, 1, 1]))






"""
A zero-indexed array A consisting of N integers is given.
The dominator of array A is the value that occurs in more
than half of the elements of A.

For example, consider array A such that

	A[0] = 3    A[1] = 4    A[2] =  3
	A[3] = 2    A[4] = 3    A[5] = -1
	A[6] = 3    A[7] = 3

The dominator of A is 3 because it occurs "in" 5 out of 8
elements of A (namely in those with indices 0, 2, 4, 6 and
7) and 5 is more than a half of 8.

Write a function

	def solution(A)

that, given a zero-indexed array A consisting of N integers,
returns index of any element of array A in which the dominator
of A occurs. The function should return -1 if array A does
not have a dominator.

Assume that:

	N is an integer within the range [0..100,000];
	each element of array A is an integer within the range
	[-2,147,483,648..2,147,483,647].

For example, given array A such that

	A[0] = 3    A[1] = 4    A[2] =  3
	A[3] = 2    A[4] = 3    A[5] = -1
	A[6] = 3    A[7] = 3

the function may return 0, 2, 4, 6 or 7, as explained above.

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(1), beyond
	input storage (not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

def solution_array_dominator(A):
	N = len(A)
	T = {}

	for i in xrange(N):
		v = A[i]
		if (v in T):
			T[v][1] += 1
		else:
			T[v] = [i, 1]

	for val, cnt in T.iteritems():
		if (cnt[1] > (N / 2)):
			return cnt[0]

	return -1

assert(0 == solution_array_dominator([3, 2, 3, 4, 3, 3, 3, -1]))






"""
A non-empty zero-indexed array A consisting of N integers
is given.

The leader of this array is the value that occurs in more
than half of the elements of A.

An equi leader is an index S such that 0 <= S < N - 1 and
two sequences A[0], A[1], ..., A[S] and A[S + 1], A[S + 2],
..., A[N - 1] have leaders of the same value.

For example, given array A such that:

	A[0] = 4
	A[1] = 3
	A[2] = 4
	A[3] = 4
	A[4] = 4
	A[5] = 2

we can find two equi leaders:

	0, because sequences: (4) and (3, 4, 4, 4, 2) have the
	same leader, whose value is 4.
	2, because sequences: (4, 3, 4) and (4, 4, 2) have the
	same leader, whose value is 4.

	NOT 3: (4,3,4,4) has leader 4, (4,2) has leader -1

The goal is to count the number of equi leaders.

Write a function:

	def solution(A)

that, given a non-empty zero-indexed array A consisting of N
integers, returns the number of equi leaders.

For example, given:

	A[0] = 4
	A[1] = 3
	A[2] = 4
	A[3] = 4
	A[4] = 4
	A[5] = 2

the function should return 2, as explained above.

Assume that:

	N is an integer within the range [1..100,000];
	each element of array A is an integer within the range
	[-1,000,000,000..1,000,000,000].

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(N), beyond
	input storage (not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

def solution_array_equi_leader(A):
	N = len(A)

	L = [-1] * N; LD = {}
	R = [-1] * N; RD = {}

	ml = 0; lv = -1
	mr = 0; rv = -1

	##
	## for all i we want the leader up to index i and the
	## leader down to index i at the same time, then loop
	## through both tables and see where they match
	##
	for i in xrange(N):
		j = N - i - 1
		vi = A[i]
		vj = A[j]

		if (not (vi in LD)):
			LD[vi] = 1
		else:
			LD[vi] += 1

		## update left-leader
		##
		## if we have seen value <vi> as many times as our
		## current leader and vi is not equal to it, reset;
		## otherwise if we have seen <vi> more times than
		## our current leader, change
		##
		if (ml == LD[vi] and lv != vi):
			lv = -1
		elif (ml < LD[vi]):
			ml = LD[vi]
			lv = vi


		if (not (vj in RD)):
			RD[vj] = 1
		else:
			RD[vj] += 1

		## update right-leader; same logic as above
		if (mr == RD[vj] and rv != vj):
			rv = -1
		elif (mr < RD[vj]):
			mr = RD[vj]
			rv = vj

		if (ml > ((i + 1) / 2)): L[i] = lv
		if (mr > ((N - j) / 2)): R[j] = rv

	n = 0

	## finally sum up the number of equi-leaders
	for i in xrange(N - 1):
		n += int(L[i] == R[i + 1] and L[i] != -1)

	return n

assert(2 == solution_array_equi_leader([4, 3, 4, 4, 4, 2]))
assert(0 == solution_array_equi_leader([2, 2, 3, 3, 3, 4, 4, 5, 5, 5, 5]))
assert(0 == solution_array_equi_leader([4,4,4,4,5,5,5,5,3]))
assert(0 == solution_array_equi_leader([4,4,4,4,4,5,5,5,5,3,1,  2,2,2,2,2,2,2,2,2,2,2]))






"""
A positive integer D is a factor of a positive integer
N if there exists an integer M such that N = D * M.

For example, 6 is a factor of 24, because M = 4 satisfies
the above condition (24 = 6 * 4).

Write a function:

	def solution(N)

that, given a positive integer N, returns the number of
its factors.

For example, given N = 24, the function should return 8,
because 24 has 8 factors, namely 1, 2, 3, 4, 6, 8, 12, 24.
There are no other factors of 24.

Assume that:

	N is an integer within the range [1..2,147,483,647].

Complexity:

	expected worst-case time complexity is O(sqrt(N));
	expected worst-case space complexity is O(1).
"""

def solution_count_factors(N):
	K = min(N, int(N ** 0.5) + 1)
	n = 0

	if (N > 1):	
		for i in xrange(1, K + 1, 1):
			## do not double-count symmetric factors (i.e. 5*5=25)
			b = int(i != (N / i))
			s = (b * 2) + (1 - b) * 1

			n += (int((N % i) == 0) * s)
	else:
		n = 1

	return n

def solution_count_factors_alt(N):
	n = 0
	i = 1

	if (N > 1):	
		while ((i * i) < N):
			n += (int((N % i) == 0) * 2)
			i += 1

		n += (int((i * i) == N) * 1)
	else:
		n = 1

	return n

assert(1 == solution_count_factors( 1))
assert(8 == solution_count_factors(24))
assert(3 == solution_count_factors(25))
assert(4 == solution_count_factors(69))






"""
A prime is a positive integer X that has exactly two distinct
divisors: 1 and X. The first few prime integers are 2, 3, 5,
7, 11 and 13.

A semiprime is a natural number that is the product of two (not
necessarily distinct) prime numbers. The first few semiprimes are
4, 6, 9, 10, 14, 15, 21, 22, 25, 26.

You are given two non-empty zero-indexed arrays P and Q, each
consisting of M integers. These arrays represent queries about
the number of semiprimes within specified ranges.

Query K requires you to find the number of semiprimes within the
range (P[K], Q[K]), where 1 <= P[K] <= Q[K] <= N.

For example, consider an integer N = 26 and arrays P, Q such that:

	P[0] = 1    Q[0] = 26
	P[1] = 4    Q[1] = 10
	P[2] = 16   Q[2] = 20

The number of semiprimes within each of these ranges is as follows:

	(1, 26) is 10,
	(4, 10) is 4,
	(16, 20) is 0.

Write a function:

	def solution(N, P, Q)

that, given an integer N and two non-empty zero-indexed arrays P and
Q consisting of M integers, returns an array consisting of M elements
specifying the consecutive answers to all the queries.

For example, given an integer N = 26 and arrays P, Q such that:

	P[0] = 1    Q[0] = 26
	P[1] = 4    Q[1] = 10
	P[2] = 16   Q[2] = 20

the function should return the values [10, 4, 0], as explained above.

Assume that:

	N is an integer within the range [1..50,000];
	M is an integer within the range [1..30,000];
	each element of arrays P, Q is an integer within the range
	[1..N]; P[i] <= Q[i].

Complexity:

	expected worst-case time complexity is O(N*log(log(N))+M);
	expected worst-case space complexity is O(N+M), beyond
	input storage (not counting the storage required for
	input arguments).
"""

def solution_count_semi_primes(N, P, Q):
	def is_prime(n):
		## there is only one even-valued prime
		if ((n <= 1) or ((n & 1) == 0)):
			return (n == 2)

		i = 3
		k = 2

		while ((i * i) <= n):
			if ((n % i) == 0):
				return False

			i += k

		return True

	def gen_primes(K):
		R = []
		U = int(K ** 0.5) + 1
		L = [-1] * (N + 1)
		L[0] = 1
		L[1] = 1
		T = {}

		if (False):
			for i in xrange(2, U):
				if (L[i * 1] != -1):
					continue

				## cross out all multiples
				for j in xrange(2, (N / i) + 1):
					if ((i * j) <= N and L[i * j] == -1):
						L[i * j] = i

		else:
			i = 2

			while ((i * i) <= K):
				if (L[i] != -1):
					i += 1
					continue

				## i={2, 3, 4, 5, ...}, j={4, 9, 16, 25, ...}
				j = i * i

				## cross out all multiples
				while (j <= K):
					s = int(L[j] == -1)
					v = (i * s) + L[j] * (1 - s)
					L[j] = v
					j += i

				i += 1

		for i in xrange(len(L)):
			if (L[i] == -1):
				T[i] = 1

		return (T.keys())

	def bsearch_array(L, v):
		assert(len(L) != 0)

		imin = 0
		imax = len(L) - 1
		icur = (imin + imax) / 2
		iprv = -1

		while (icur != iprv):
			if (v <= L[icur]): imax = icur
			if (v >= L[icur]): imin = icur

			iprv = icur
			icur = (imin + imax) / 2

		return icur

	L = [0] * len(P)
	R = gen_primes(N)

	if (N <= 1):
		return L

	if (True):
		Z = [0] * ((len(R) * len(R)) / 1)

		## generate all semi-primes once
		for j in xrange(len(R)):
			for k in xrange(j, len(R)):
				assert((j * len(R) + k) < len(Z))
				assert(Z[j * len(R) + k] == 0)

				Z[j * len(R) + k] = R[j] * R[k]

		## sort by increasing order so we can use BS
		Z.sort()

		for i in xrange(len(P)):
			rmin = P[i]; kmin = bsearch_array(Z, rmin)
			rmax = Q[i]; kmax = bsearch_array(Z, rmax)

			n = (kmax - kmin) + 1
			n -= (int(Z[kmin] < rmin))
			n -= (int(Z[kmax] > rmax))
			L[i] = n
	else:
		##
		## NOTE: can we merge the range-bounds somehow?
		##
		for i in xrange(len(P)):
			rmin = P[i]
			rmax = Q[i]

			n = 0

			## calculate number of unique semi-primes in range [rmin, rmax]
			for j in xrange(len(R)):
				p = R[j]

				for k in xrange(j, len(R)):
					q = R[k]
					n += int((p * q) >= rmin and (p * q) <= rmax)

			L[i] = n

			"""
			## this way requires a dictionary to prevent double-counting
			T = {}

			for p in R:
				for q in R:
					if ((p * q) < rmin): continue
					if ((p * q) > rmax): continue
					T[p * q] = 1
			"""

			"""
			for j in xrange(rmin, rmax + 1):
				for k in xrange(2, len(R)):
					## k must be prime; the factor j / k must also be
					## must also prevent double-counting (6=2*3 == 3*2)
					if (R[k] and R[ j / k ] and ((j % k) == 0) and (not (j in T))):
						T[j] = 1
			"""

	return L

assert(solution_count_semi_primes( 1, [1, 4, 16], [26, 10, 20]))
assert(solution_count_semi_primes(26, [1, 4, 16], [26, 10, 20]))






"""
A non-empty zero-indexed array A consisting of N integers is
given. A pair of integers (P, Q), such that 0 <= P < Q < N,
is called a slice of array A (notice that the slice contains
at least two elements). The average of a slice (P, Q) is the
sum of A[P] + A[P + 1] + ... + A[Q] divided by the length of
the slice. To be precise, the average equals:

	(A[P] + A[P + 1] + ... + A[Q]) / (Q - P + 1)

For example, array A such that:

	A[0] = 4
	A[1] = 2
	A[2] = 2
	A[3] = 5
	A[4] = 1
	A[5] = 5
	A[6] = 8

contains the following example slices:

	slice (1, 2), whose average is (2 + 2) / 2 = 2;
	slice (3, 4), whose average is (5 + 1) / 2 = 3;
	slice (1, 4), whose average is (2 + 2 + 5 + 1) / 4 = 2.5.

The goal is to find the starting position of a slice whose
average is minimal.

Write a function:

	def solution(A)

that, given a non-empty zero-indexed array A consisting of N
integers, returns the starting position of the slice with the
minimal average. If there is more than one slice with a minimal
average, you should return the smallest starting position of
such a slice.

For example, given array A such that:

	A[0] = 4
	A[1] = 2
	A[2] = 2
	A[3] = 5
	A[4] = 1
	A[5] = 5
	A[6] = 8

the function should return 1, as explained above.

Assume that:

	N is an integer within the range [2..100,000];
	each element of array A is an integer within the range
	[-10,000..10,000].

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(N), beyond
	input storage (not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

"""
def solution_min_avg_two_slice_quadratic(A):
	def prefix_sums(A):
		n = len(A)
		P = [0] * (n + 1)
		for k in xrange(1, n + 1):
			P[k] = P[k - 1] + A[k - 1]
		return P

	def suffix_sums(A):
		n = len(A)
		P = [0] * (n + 1)
		for k in xrange(0, n):
			j = n - k
			P[j - 1] = P[j] + A[j - 1]
		return P

	def count_total(P, x, y):
		return (P[y + 1] - P[x])

	N = len(A)
	P = prefix_sums(A)

	m = sys.maxint
	k = 0

	for x in xrange(N):
		for y in xrange(x + 1, N):
			t = count_total(P, x, y)
			a = (t * 1.0) / (y - x + 1)

			if (a < m):
				m = a
				k = x

	return k
"""

def solution_min_avg_two_slice_noprefix(A):
	N = len(A)

	min_idx = 0
	min_val = sys.maxint

	## evaluate all two-slices (NB: (x / 2) * 6 == x * 3)
	for idx in xrange(0, N - 1):
		val = (A[idx] + A[idx + 1]) * 3

		if (val < min_val):
			min_val = val
			min_idx = idx

	## evaluate all three-slices (NB: (x / 3) * 6 == x * 2)
	for idx in xrange(0, N - 2):
		val = (A[idx] + A[idx + 1] + A[idx + 2]) * 2

		if (val < min_val):
			min_val = val
			min_idx = idx

	## by proof of induction we do not need to
	## consider slices of lengths larger than 3
	return min_idx

def solution_min_avg_two_slice_prefix(A):
	N = len(A)
	S = [0] * (N + 1)

	## calculate prefix sums; really not needed here
	for i in xrange(N):
		S[i + 1] = S[i] + A[i]

	min_idx = 0
	min_val = sys.maxint

	for i in xrange(N - 2):
		## try pair average
		val = (S[i + 2] - S[i]) * 3

		if (val < min_val):
			min_val = val
			min_idx = i

		## try triplet average
		avg = (S[i + 3] - S[i]) * 2

		if (val < min_val):
			min_val = val
			min_idx = i

	## last pair (which the dual-loop did not visit)
	val = 1.0 * (S[N] - S[N - 2]) * 3

	if (val < min_val):
		min_idx = N - 2

	return min_idx

assert(0 == solution_min_avg_two_slice_prefix([3, 5, 11, 7]))
assert(0 == solution_min_avg_two_slice_prefix([3, 5, 11]))
assert(1 == solution_min_avg_two_slice_prefix([4, 2, 2, 5, 1, 5, 8]))






"""
A binary gap within a positive integer N is any maximal sequence
of consecutive zeros that is surrounded by ones at both ends in
the binary representation of N.

For example, number 9 has binary representation 1001 and contains
a binary gap of length 2. The number 529 has binary representation
1000010001 and contains two binary gaps: one of length 4 and one of
length 3. The number 20 has binary representation 10100 and contains
one binary gap of length 1. The number 15 has binary representation
1111 and has no binary gaps.

Write a function:

	def solution(N)

that, given a positive integer N, returns the length of its longest
binary gap. The function should return 0 if N doesn't contain a binary
gap.

For example, given N = 1041 the function should return 5, because N
has binary representation 10000010001 and so its longest binary gap
is of length 5.

Assume that:

	N is an integer within the range [1..2,147,483,647].

Complexity:

	expected worst-case time complexity is O(log(N));
	expected worst-case space complexity is O(1).
"""

def solution_binary_gap_length(N):
	m = 0
	k = 0

	while (N > 0):
		b = 0

		## shift away blocks of 1's
		while ((N & 1) == 1):
			N >>= 1
			b = 1

		## shift away blocks of 0's; each 0 counts towards
		## the current gap-length if we shifted a 1 before
		while (N > 0 and (N & 1) == 0):
			N >>= 1
			k += (1 * b)

		## if current LSB is a 1 again, we have a valid gap
		k *= int((N & 1) == 1)
		m = max(m, k)
		k = 0

	return m

assert(0 == solution_binary_gap_length(  16))
assert(1 == solution_binary_gap_length(  20))
assert(4 == solution_binary_gap_length( 529))
assert(5 == solution_binary_gap_length(1041))






"""
BinaryTreeHeight
"""

def solution_bin_tree_height(T):
	def depth(T, d):
		## if T is None this also works: conventionally
		## an empty tree has height -1 and depth equals
		## 0 for the first call
		if (T == None):
			return (d - 1)

		return (max(depth(T.l, d + 1), depth(T.r, d + 1)))

	return (depth(T, 0))






"""
Find a symmetry point of a string, if any.

Write a function:

	def solution(S)

that, given a string S, returns the index (counting from 0)
of a character such that the part of the string to the left
of that character is a reversal of the part of the string
to its right. The function should return -1 if no such index
exists.

Reversing an empty string (i.e. a string whose length is zero)
gives an empty string.

For example, given a string:

	"racecar"

the function should return 3, because the substring to the
left of the character "e" at index 3 is "rac", and the one
to the right is "car".

Given a string:

	"x"

the function should return 0, because both substrings are empty.

Assume that:

	the length of S is within the range [0..2,000,000].

Complexity:

	expected worst-case time complexity is O(length(S));
	expected worst-case space complexity is O(1) (not
	counting the storage required for input arguments).
"""

def solution_str_symmetry_point(S):
	N = len(S)

	if (N == 0): return -1
	## if (N == 1): return  0
	## only odd-length strings can have a symmetry point
	if ((N & 1) == 0):
		return -1

	i = 0
	j = N - i - 1

	while (i < j):
		if (S[i] != S[j]):
			break

		i += 1
		j -= 1

	## if we made it to the middle of the string it
	## is a palindrome, and an SP must exist at that
	## position
	if (i == j):
		return i

	return -1

assert(-1 == solution_str_symmetry_point(""))
assert( 0 == solution_str_symmetry_point("x"))
assert( 3 == solution_str_symmetry_point("racecar"))
assert(-1 == solution_str_symmetry_point("racecarx"))
assert(-1 == solution_str_symmetry_point("xracecar"))






"""
A non-empty zero-indexed array A consisting of N integers
is given. The array contains an odd number of elements, and
each element of the array can be paired with another element
that has the same value, except for one element that is left
unpaired.

For example, in array A such that:

	A[0] = 9  A[1] = 3  A[2] = 9
	A[3] = 3  A[4] = 9  A[5] = 7
	A[6] = 9

	the elements at indexes 0 and 2 have value 9,
	the elements at indexes 1 and 3 have value 3,
	the elements at indexes 4 and 6 have value 9,
	the element at index 5 has value 7 and is unpaired.

Write a function:

	def solution(A)

that, given an array A consisting of N integers fulfilling
the above conditions, returns the value of the unpaired element.

For example, given array A such that:

	A[0] = 9  A[1] = 3  A[2] = 9
	A[3] = 3  A[4] = 9  A[5] = 7
	A[6] = 9

the function should return 7, as explained in the example above.

Assume that:

	N is an odd integer within the range [1..1,000,000];
	each element of array A is an integer within the range
	[1..1,000,000,000]; all but one of the values in A occur
	an even number of times.

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(1), beyond
	input storage (not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

def solution_odd_array_occurrences(A):
	N = len(A)
	T = {}

	for i in xrange(N):
		v = A[i]

		if (v in T):
			T[v] += 1
		else:
			T[v] = 1

	for val, cnt in T.iteritems():
		if ((cnt & 1) == 1):
			return val

	return -1

assert(7 == solution_odd_array_occurrences([9, 3, 9, 3, 9, 7, 9]))






"""
You are given integers K, M and a non-empty zero-indexed
array A consisting of N integers. Every element of the
array is not greater than M.

You should divide this array into K blocks of consecutive
elements. The size of the block is any integer between 0
and N. Every element of the array should belong to some
block.

The sum of the block from X to Y equals A[X] + A[X + 1] + ... + A[Y].
The sum of empty block equals 0.

The large sum is the maximal sum of any block.

For example, you are given integers K = 3, M = 5 and array
A such that:

	A[0] = 2
	A[1] = 1
	A[2] = 5
	A[3] = 1
	A[4] = 2
	A[5] = 2
	A[6] = 2

The array can be divided, for example, into the following blocks:

	[2, 1, 5, 1, 2, 2, 2], [], [] with a large sum of 15;
	[2], [1, 5, 1, 2], [2, 2] with a large sum of 9;
	[2, 1, 5], [], [1, 2, 2, 2] with a large sum of 8;
	[2, 1], [5, 1], [2, 2, 2] with a large sum of 6.

The goal is to minimize the large sum. In the above example,
6 is the minimal large sum.

Write a function:

	def solution(K, M, A)

that, given integers K, M and a non-empty zero-indexed array
A consisting of N integers, returns the minimal large sum.

For example, given K = 3, M = 5 and array A such that:

	A[0] = 2
	A[1] = 1
	A[2] = 5
	A[3] = 1
	A[4] = 2
	A[5] = 2
	A[6] = 2

the function should return 6, as explained above.

Assume that:

	N and K are integers within the range [1..100,000];
	M is an integer within the range [0..10,000];
	each element of array A is an integer within the range
	[0..M].

Complexity:

	expected worst-case time complexity is O(N*log(N+M));
	expected worst-case space complexity is O(1), beyond
	input storage (not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

##
## K := number of blocks (so K - 1 split-indices)
## M := maximum array element value
##
def solution_min_max_division(K, M, A):
	def prefix_sums(A):
		n = len(A)
		P = [0] * (n + 1)
		for k in xrange(1, n + 1):
			P[k] = P[k - 1] + A[k - 1]
		return P

	def split_array(A, P,  blk_min_idx, blk_max_idx,  cur_depth, max_depth,  cur_sums):
		if (cur_depth >= max_depth):
			return (max(cur_sums))

		## min_sum = M * len(A)
		min_sum = sys.maxint

		## a block of length 1 does not need further splitting
		if ((blk_max_idx - blk_min_idx) == 1):
			return (max(max(cur_sums), P[blk_max_idx] - P[blk_min_idx]))

		## NOTE:
		##   we do not need to bother checking the [] + [rhs] splits
		##   because sum([rhs]) will equal sum(blk) which is maximal
		##   (all other splits will have at least one sub-block with
		##   smaller sum), hence the +1
		##
		##   does this generalize correctly? i.e. can we split into
		##   equal-sized halves safely at all levels? does not seem
		##   so: [1,2,3,4] --> m(s([1,2]+[3,4])) > m(s([1,2,3]+[4]))
		##
		##   might this have the same structure as min_avg_two_slice?
		##
		## NOTE: still too slow
		##
		for blk_split_idx in xrange(blk_min_idx + 1, blk_max_idx):
			## assert(sum(A[blk_min_idx : blk_split_idx]) == (P[blk_split_idx] - P[blk_min_idx]))
			## assert(sum(A[blk_split_idx : ]) == (P[blk_max_idx] - P[blk_split_idx]))
			##
			## cur_sums[-1] = sum(A[blk_min_idx : blk_split_idx])
			## cur_sums.append(sum(A[blk_split_idx : ]))

			## replace sum of entire block by sum of its LHS, then append sum(RHS)
			cur_sums[-1] = (P[blk_split_idx] - P[blk_min_idx])
			cur_sums.append(P[blk_max_idx] - P[blk_split_idx])

			max_sum = split_array(A, P,  blk_split_idx, blk_max_idx,  cur_depth + 1, max_depth,  cur_sums)
			min_sum = min(max_sum, min_sum)

			cur_sums.pop()

		return min_sum

	return (split_array(A, prefix_sums(A),  0, len(A),  0, K - 1,  [sum(A)]))

assert(6 == solution_min_max_division(3, 5, [2, 1, 5, 1, 2, 2, 2]))
assert(1 == solution_min_max_division(3, 1, [1]))
assert(4 == solution_min_max_division(3, 5, [1, 2, 3,4]))






"""
PolygonConcavityIndex
"""

class Point2D:
	def __init__(self, x = 0, y = 0):
		self.x = x
		self.y = y
	def __repr__(self):
		return ("<x=%.3f, y=%.3f>" % (self.x, self.y))

def solution_concavity_index(A):
	def vec_dot_xy(a, b):
		assert(type(a) == tuple)
		assert(type(b) == tuple)
		return (a[0] * b[0] + a[1] * b[1])
	def vec_sub_xy(a, b):
		assert(type(a) == tuple)
		assert(type(b) == tuple)
		return (a[0] - b[0], a[1] - b[1])

	def pnt_dot_xy(a, b):
		## assert(type(a) == Point2D)
		## assert(type(b) == Point2D)
		return (a.x * b.x + a.y * b.y)
	def pnt_sub_xy(a, b):
		## assert(type(a) == Point2D)
		## assert(type(b) == Point2D)
		return (Point2D(a.x - b.x, a.y - b.y))

	def pnt_norm_xy(v):
		## assert(type(v) == Point2D)
		sqr_mag = pnt_dot_xy(v, v)
		if (sqr_mag < 0.0001):
			return v[:]
		inv_mag = 1.0 / (sqr_mag ** 0.5)
		return (Point2D(v.x * inv_mag, v.y * inv_mag))
	def vec_norm_xy(v):
		assert(type(v) == tuple)
		sqr_mag = vec_dot_xy(v, v)
		if (sqr_mag < 0.0001):
			return v[:]
		inv_mag = 1.0 / (sqr_mag ** 0.5)
		return (v[0] * inv_mag, v[1] * inv_mag)

	def edge_ori_sign(edge_a, edge_b):
		## construct vector orthogonal to edge_a
		o = Point2D(-edge_a.y, edge_a.x)
		## calculate cosine of angle(ortho, edge_b)
		d = pnt_dot_xy(o, edge_b)
		## positive angles denote left-hand turns
		return (int(d >= (0.0 - 0.0)))

	N = len(A)

	## determine orientation of e12 relative to e01;
	## all other pairs of edges must have the same
	e01 = pnt_norm_xy(pnt_sub_xy(A[1], A[0]))
	e12 = pnt_norm_xy(pnt_sub_xy(A[2], A[1]))

	prv_sign = edge_ori_sign(e01, e12)
	prv_edge = e12

	for i in xrange(2, N):
		cur_edge = pnt_norm_xy(pnt_sub_xy(A[(i + 1) % N], A[i]))
		cur_sign = edge_ori_sign(prv_edge, cur_edge)
		prv_edge = cur_edge

		## colinearity check (unneeded?)
		if (False and pnt_dot_xy(prv_edge, cur_edge) > 0.99999):
			continue

		if (cur_sign != prv_sign):
			return i

	return -1

assert(-1 == solution_concavity_index([Point2D(0, 0), Point2D(0, 1), Point2D(1, 1), Point2D(1, 0)]))
assert(-1 == solution_concavity_index([Point2D(-1, 3), Point2D(3, 1), Point2D(0, -1), Point2D(-2, 1)]))
assert( 2 == solution_concavity_index([Point2D(-1, 3), Point2D(3, 1), Point2D(1, 1), Point2D(0, -1), Point2D(-2, 1), Point2D(-1, 2)]))






"""
There are N ropes numbered from 0 to N - 1, whose lengths
are given in a zero-indexed array A, lying on the floor in
a line. For each I (0 <= I < N), the length of rope I on the
line is A[I].

We say that two ropes I and I + 1 are adjacent. Two adjacent
ropes can be tied together with a knot, and the length of the
tied rope is the sum of lengths of both ropes. The resulting
new rope can then be tied again.

For a given integer K, the goal is to tie the ropes in such
a way that the number of ropes whose length is greater than
or equal to K is maximal.

For example, consider K = 4 and array A such that:

	A[0] = 1
	A[1] = 2
	A[2] = 3
	A[3] = 4
	A[4] = 1
	A[5] = 1
	A[6] = 3

We can tie:

	rope 1 with rope 2 to produce a rope of length A[1] + A[2] = 5;
	rope 4 with rope 5 with rope 6 to produce a rope of length A[4] + A[5] + A[6] = 5.

After that, there will be three ropes whose lengths are greater
than or equal to K = 4. It is not possible to produce four such
ropes.

Write a function:

	def solution(K, A)

that, given an integer K and a non-empty zero-indexed array
A of N integers, returns the maximum number of ropes of length
greater than or equal to K that can be created.

For example, given K = 4 and array A such that:

	A[0] = 1
	A[1] = 2
	A[2] = 3
	A[3] = 4
	A[4] = 1
	A[5] = 1
	A[6] = 3

the function should return 3, as explained above.

Assume that:

	N is an integer within the range [1..100,000];
	K is an integer within the range [1..1,000,000,000];
	each element of array A is an integer within the range
	[1..1,000,000,000].

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(N), beyond
	input storage (not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

def solution_tie_ropes(K, A):
	N = 0
	S = 0

	for i in xrange(len(A)):
		if (A[i] < K):
			S += A[i]
		else:
			## a rope with length >= K should not be tied, since
			## that would decrease the count by 1 (if the other
			## rope is also longer than K) or leave it unchanged
			## (if the other rope is shorter than K)
			S = A[i]

		if (S >= K):
			N += 1
			S = 0

	return N

assert(3 == solution_tie_ropes(4, [1, 2, 3, 4, 1, 1, 3]))
assert(2 == solution_tie_ropes(2, [1, 1, 1, 1]))
assert(3 == solution_tie_ropes(2, [4, 5, 6]))
assert(1 == solution_tie_ropes(2, [1, 20]))






"""
A non-empty zero-indexed array A consisting of N numbers
is given. The array is sorted in non-decreasing order. The
absolute distinct count of this array is the number of
distinct absolute values among the elements of the array.

For example, consider array A such that:

	A[0] = -5
	A[1] = -3
	A[2] = -1
	A[3] =  0
	A[4] =  3
	A[5] =  6

The absolute distinct count of this array is 5, because
there are 5 distinct absolute values among the elements
of this array, namely 0, 1, 3, 5 and 6.

Write a function:

	def solution(A)

that, given a non-empty zero-indexed array A consisting of
N numbers, returns absolute distinct count of array A.

For example, given array A such that:

	A[0] = -5
	A[1] = -3
	A[2] = -1
	A[3] =  0
	A[4] =  3
	A[5] =  6

the function should return 5, as explained above.

Assume that:

	N is an integer within the range [1..100,000];
	each element of array A is an integer within the range
	[-2,147,483,648..2,147,483,647];
	array A is sorted in non-decreasing order.

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(N), beyond
	input storage (not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

def solution_count_distinct(A):
	## assumes the input array is unsorted, O(N log N)
	## (using a hash-table (dict) reduces this to O(N))
	A.sort()

	## handle empty and single-element lists
	k = min(1, len(A))

	for i in xrange(1, len(A)):
		k += int(A[i] > A[i - 1])

	return k

def solution_count_abs_distinct_dict(A):
	N = len(A)
	T = {}
	k = 0

	## using a dict feels like cheating
	for i in xrange(N):
		v = abs(A[i])

		if (not (v in T)):
			T[v] = 1
			k += 1

	return k

def solution_count_abs_distinct(A):
	def count_abs_distinct(A):
		## assumes the input array is already in ascending
		## order; a non-empty array has at least one unique
		## value
		k = 1

		for i in xrange(1, len(A)):
			k += int(A[i] > A[i - 1])

		return k

	if (A[0] >= 0):
		return (count_abs_distinct(A))

	N = len(A)
	Z = A[:]

	i = 0 ## index of last negative value
	j = 0 ## index of first positive value

	## flip all negative values
	##
	## if *all* happen to be negative, the merge
	## loop will simply reverse the elements s.t.
	## count_abs_distinct still works fine
	##
	while (j < N and A[j] < 0):
		Z[j] = -A[j]
		j += 1

	i = j - 1
	n = 0

	## merge both now-positive halves; handle corner
	## cases if either index reaches the end before
	## the other
	while (n < N):
		if (i == -1): A[n] = Z[j]; j += 1; n += 1; continue
		if (j ==  N): A[n] = Z[i]; i -= 1; n += 1; continue

		if (Z[i] > Z[j]):
			A[n] = Z[j]
			j += 1
		else:
			A[n] = Z[i]
			i -= 1

		n += 1

	return (count_abs_distinct(A))

assert(5 == solution_count_abs_distinct([-5, -3, -1, 0, 3, 6]))
assert(5 == solution_count_abs_distinct([1, 2, 3, 4, 5]))
assert(1 == solution_count_abs_distinct([5, 5, 5]))
assert(3 == solution_count_abs_distinct([-3, -2, -1]))
assert(1 == solution_count_abs_distinct([0, 0, 0]))
assert(4 == solution_count_abs_distinct([-3, -2, -2, -1, 0]))
assert(2 == solution_count_abs_distinct([1, 2]))






"""
MaxProductOfThree
"""

def solution_max_triple_product(A):
	A.sort()
	## if the two smallest numbers are both negative,
	## their product combined with the largest number
	## might exceed the obvious triplet
	a = A[ 0] * A[ 1] * A[-1]
	b = A[-3] * A[-2] * A[-1]
	return (max(a, b))

solution_max_triple_product([-3, 1, 2, -2, 5, 6])
solution_max_triple_product([-5, -4, -3, -2])
solution_max_triple_product([-1000, -1000, -999, -998, 1, 998, 999, 1000, 1000])






"""
You have to climb up a ladder. The ladder has exactly
N rungs, numbered from 1 to N. With each step, you can
ascend by one or two rungs. More precisely:

	with your first step you can stand on rung 1 or 2,
	if you are on rung K, you can move to rungs K + 1 or K + 2,
	finally you have to stand on rung N.

Your task is to count the number of different ways of
climbing to the top of the ladder.

For example, given N = 4, you have five different ways
of climbing, ascending by:

	1, 1, 1 and 1 rung,
	1, 1 and 2 rungs,
	1, 2 and 1 rung,
	2, 1 and 1 rungs, and
	2 and 2 rungs.

Given N = 5, you have eight different ways of climbing,
ascending by:

	1, 1, 1, 1 and 1 rung,
	1, 1, 1 and 2 rungs,
	1, 1, 2 and 1 rung,
	1, 2, 1 and 1 rung,
	1, 2 and 2 rungs,
	2, 1, 1 and 1 rungs,
	2, 1 and 2 rungs, and
	2, 2 and 1 rung.

The number of different ways can be very large, so it is
sufficient to return the result modulo 2^P, for a given
integer P.

Write a function:

	def solution(A, B)

that, given two non-empty zero-indexed arrays A and B of
L integers, returns an array consisting of L integers
specifying the consecutive answers; position I should
contain the number of different ways of climbing the
ladder with A[I] rungs modulo 2^B[I].

For example, given L = 5 and:

	A[0] = 4   B[0] = 3
	A[1] = 4   B[1] = 2
	A[2] = 5   B[2] = 4
	A[3] = 5   B[3] = 3
	A[4] = 1   B[4] = 1

the function should return the sequence [5, 1, 8, 0, 1],
as explained above.

Assume that:

	L is an integer within the range [1..30,000];
	each element of array A is an integer within the range [1..L];
	each element of array B is an integer within the range [1..30].

Complexity:

	expected worst-case time complexity is O(L);
	expected worst-case space complexity is O(L), beyond
	input storage (not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

def solution(A, B):
	def fib_tbl(N):
		T = [0] * (N + 1)

		T[0] = 0
		T[1] = 1

		for i in xrange(2, N + 1):
			T[i] = T[i - 1] + T[i - 2]

		return T

	L = len(A)
	R = [0] * L
	F = fib_tbl(max(A) + 1)

	## the number of ways to reach the N-th rung
	## is equal to the N+1'th Fibonacci-sequence
	## value
	##
	##  A[0] = rung=4   B[0] = expo=3  -->  L[0] = ways=5 % (2**3) = 5
	##  A[1] = rung=4   B[1] = expo=2  -->  L[1] = ways=5 % (2**2) = 1
	##  A[2] = rung=5   B[2] = expo=4  -->  L[2] = ways=8 % (2**4) = 8
	##  A[3] = rung=5   B[3] = expo=3  -->  L[3] = ways=8 % (2**3) = 0
	##  A[4] = rung=1   B[4] = expo=1  -->  L[4] = ways=1 % (2**1) = 1
	##
	for i in xrange(L):
		rung = A[i]
		expo = B[i]
		ways = F[rung + 1]

		R[i] = ways % (2 ** expo)

	return R

assert([5, 1, 8, 0, 1] == solution([4, 4, 5, 5, 1], [3, 2, 4, 3, 1]))






"""
A string S consisting of N characters is called properly nested
if:

	S is empty;
	S has the form "(U)" where U is a properly nested string;
	S has the form "VW" where V and W are properly nested strings.

For example, string "(()(())())" is properly nested but string "())"
isn't.

Write a function:

	def solution(S)

that, given a string S consisting of N characters, returns 1 if
string S is properly nested and 0 otherwise.

For example, given S = "(()(())())", the function should return
1 and given S = "())", the function should return 0, as explained
above.

Assume that:

	N is an integer within the range [0..1,000,000];
	string S consists only of the characters "(" and/or ")".

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(1) (not counting
	the storage required for input arguments).
"""

def solution_string_nesting(S):
	Q = []
	r = 1

	for i in xrange(len(S)):
		c = S[i]

		if (c == '('):
			Q.append(c)
		elif (c == ')'):
			if (len(Q) == 0):
				r = 0
				break

			Q.pop()

	if (len(Q) != 0):
		return 0

	return r

def solution_adv_string_nesting(S):
	## active stacks (one per character-type)
	Q = [ [], [], [] ]
	## stack of active stacks
	Z = []
	## character-types
	C = [ ('(', ')'), ('[', ']'), ('{', '}') ]
	r = 1

	for i in xrange(len(S)):
		c = S[i]

		for j in xrange(len(C)):
			if (c == C[j][0]):
				## opening parenthesis-type
				Q[j].append(c)
				Z.append(Q[j])
			elif (c == C[j][1]):
				## closing parenthesis-type
				if (len(Z) == 0 or Z[-1] != Q[j]):
					r = 0
					break
				if (len(Q[j]) == 0):
					r = 0
					break

				Z.pop()
				Q[j].pop()

	for j in xrange(len(Q)):
		if (len(Q[j]) != 0):
			return 0

	return r

assert(1 == solution_string_nesting("(()(())())"))
assert(0 == solution_string_nesting("())"))
assert(1 == solution_adv_string_nesting("{[()()]}"))
assert(0 == solution_adv_string_nesting("([)()]"))
assert(0 == solution_adv_string_nesting("(" * 10000 + ")" * 10000 + ")("))






"""
You are going to build a stone wall. The wall should be
straight and N meters long, and its thickness should be
constant; however, it should have different heights in
different places. The height of the wall is specified by
a zero-indexed array H of N positive integers. H[I] is
the height of the wall from I to I+1 meters to the right
of its left end. In particular, H[0] is the height of the
wall's left end and H[N-1] is the height of the wall's right
end.

The wall should be built of cuboid stone blocks (that is,
all sides of such blocks are rectangular). Your task is
to compute the minimum number of blocks needed to build
the wall.

Write a function:

	def solution(H)

that, given a zero-indexed array H of N positive integers
specifying the height of the wall, returns the minimum
number of blocks needed to build it.

For example, given array H containing N = 9 integers:

	H[0] = 8    H[1] = 8    H[2] = 5
	H[3] = 7    H[4] = 9    H[5] = 8
	H[6] = 7    H[7] = 4    H[8] = 8

the function should return 7. The figure shows one possible
arrangement of seven blocks.

Assume that:

	N is an integer within the range [1..100,000];
	each element of array H is an integer within the range [1..1,000,000,000].

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(N), beyond
	input storage (not counting the storage required for
	input arguments).

Elements of input arrays can be modified.
"""

def solution_stone_wall(H):
	## start with the first block
	S = [H[0]]
	n = 1

	for i in xrange(1, len(H)):
		h = H[i]

		## same height at next point, do not need a new block
		if (S[-1] == h):
			continue

		elif (h < S[-1]):
			## lower height at next point, need a new block
			##
			## drop down the block-stack as far as possible
			## (to ensure the number of blocks used will be
			## minimal)
			while (len(S) > 0 and h < S[-1]):
				S.pop()

		if (len(S) == 0 or h != S[-1]):        
			S.append(h)
			n += 1

	return n

assert(3 == solution_stone_wall([3, 5, 4]))
assert(1 == solution_stone_wall([8, 8, 8]))
assert(2 == solution_stone_wall([8, 8, 8, 9]))
assert(7 == solution_stone_wall([8, 8, 5, 7, 9, 8, 7, 4, 8]))
assert(6 == solution_stone_wall([1, 2, 3, 4, 5, 6]))
assert(6 == solution_stone_wall([6, 5, 4, 3, 2, 1]))
assert(3 == solution_stone_wall([1, 2, 1, 2]))
assert(4 == solution_stone_wall([1, 2, 5, 1, 2]))








"""
Two positive integers N and M are given. Integer N represents
the number of chocolates arranged in a circle, numbered from 0
to N - 1.

You start to eat the chocolates. After eating a chocolate you
leave only a wrapper.

You begin with eating chocolate number 0. Then you omit the next
M - 1 chocolates or wrappers on the circle, and eat the following
one.

More precisely, if you ate chocolate number X, then you will next
eat the chocolate with number (X + M) modulo N (remainder of division).

You stop eating when you encounter an empty wrapper.

For example, given integers N = 10 and M = 4. You will eat the
following chocolates: 0, 4, 8, 2, 6.

The goal is to count the number of chocolates that you will eat,
following the above rules.

Write a function:

	def solution(N, M)

that, given two positive integers N and M, returns the number of
chocolates that you will eat.

For example, given integers N = 10 and M = 4. the function should
return 5, as explained above.

Assume that:

	N and M are integers within the range [1..1,000,000,000].

Complexity:

	expected worst-case time complexity is O(log(N+M));
	expected worst-case space complexity is O(log(N+M)).
"""

def solution_chocolates_by_numbers(N, M):
	def gcd_sub(a, b):
		if (a > b):
			return (gcd_sub(a - b, b))
		if (a < b):
			return (gcd_sub(a, b - a))

		return a

	def gcd_div(a, b):
		if ((a % b) == 0):
			return b

		return (gcd_div(b, a % b))

	def lcm(a, b):
		return ((a * b) / gcd_div(a, b))

	k = 0

	if (False):
		## O(N) solution
		X = [True] * N
		i = 0

		while (X[i]):
			X[i] = False
			i += M
			i %= N
			k += 1
	else:
		## O(log(N)) solution
		## initial starting point
		S = 0
		## table of wraparound values of S
		D = {}

		while (not (S in D)):
			D[S] = 1

			## number of chocolates we will eat this round
			## starting from S there are ((N - S) / M) + 1
			## steps before wrap-around occurs, *unless* M
			## divides (N - S) exactly
			## c.f. frog_jump and count_div
			##
			r = (N - S) % M
			i = ((N - S) / M) + int(r != 0)
			k += i

			## update starting point to remainder
			S += (M * i)
			S %= N

	return k

assert( 5 == solution_chocolates_by_numbers(10, 4))
assert(11 == solution_chocolates_by_numbers(11, 3))
assert(11 == solution_chocolates_by_numbers(11, 4))
assert(11 == solution_chocolates_by_numbers(11, 5))






"""
A zero-indexed array A consisting of N integers is given.
It contains daily prices of a stock share for a period of
N consecutive days. If a single share was bought on day P
and sold on day Q, where 0 <= P <= Q < N, then the profit
of such transaction is equal to A[Q] - A[P], provided that
A[Q] >= A[P]. Otherwise, the transaction brings loss of
A[P] - A[Q].

For example, consider the following array A consisting of
six elements such that:

	A[0] = 23171
	A[1] = 21011
	A[2] = 21123
	A[3] = 21366
	A[4] = 21013
	A[5] = 21367

If a share was bought on day 0 and sold on day 2, a loss of
2048 would occur because A[2] - A[0] = 21123 - 23171 = -2048.
If a share was bought on day 4 and sold on day 5, a profit of
354 would occur because A[5] - A[4] = 21367 - 21013 = 354.
Maximum possible profit was 356. It would occur if a share
was bought on day 1 and sold on day 5.

Write a function,

	def solution(A)

that, given a zero-indexed array A consisting of N integers
containing daily prices of a stock share for a period of N
consecutive days, returns the maximum possible profit from
one transaction during this period. The function should
return 0 if it was impossible to gain any profit.

For example, given array A consisting of six elements such
that:

	A[0] = 23171
	A[1] = 21011
	A[2] = 21123
	A[3] = 21366
	A[4] = 21013
	A[5] = 21367

the function should return 356, as explained above.

Assume that:

	N is an integer within the range [0..400,000];
	each element of array A is an integer within the range [0..200,000].

Complexity:

	expected worst-case time complexity is O(N);
	expected worst-case space complexity is O(1), beyond
	input storage (not counting the storage required
	for input arguments).

Elements of input arrays can be modified.
"""

def solution_max_profit(A):
	def max_slice_sum(A):
		max_ending = 0
		max_slice = 0

		for a in A:
			max_ending = max(0, max_ending + a)
			max_slice = max(max_slice, max_ending)

		return max_slice

	def max_slice_dif(A):
		max_ending = 0
		max_slice = 0

		for i in xrange(len(A) - 1):
			## running sum of local differences
			max_ending = max(0, max_ending + (A[i + 1] - A[i]))
			## maximum profit possible up to now
			max_slice = max(max_slice, max_ending)

		return max_slice

	## we do not want the max-slice sum here, but
	## the max-slice difference (which is an easy
	## modification of max-slice-sum)
	if (True):
		return (max_slice_dif(A))

	## quadratic-time solution
	n = len(A)
	r = 0

	for p in xrange(n):
		for q in xrange(p, n):
			r = max(r, A[q] - A[p])

	return r

assert(356 == solution_max_profit([23171, 21011, 21123, 21366, 21013, 21367]))

