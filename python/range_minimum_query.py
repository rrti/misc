class Node:
	def __init__(self, parent, childA, childB, minIdx, maxIdx):
		self.parent = parent
		self.childA = childA
		self.childB = childB

		self.minIdx = minIdx ## inclusive
		self.maxIdx = maxIdx ## exclusive
		self.minVal = 99999999

	def Print(self, tabs):
		if (self.childA != None):
			self.childA.Print(tabs + '\t')

		print("%srange: [%d, %d] min-value: %d" % (tabs, self.minIdx, self.maxIdx, self.minVal))

		if (self.childB != None):
			self.childB.Print(tabs + '\t')

## NOTE: array-length must be a power of 2 (possibly padded)
def BuildRangeTree(node, array, minIdx, maxIdx):
	if ((maxIdx - minIdx) == 1):
		node.minVal = array[minIdx]
		return

	midIdx = (minIdx + maxIdx) >> 1

	childA = Node(node, None, None, minIdx, midIdx)
	childB = Node(node, None, None, midIdx, maxIdx)

	node.childA = childA
	node.childB = childB

	BuildRangeTree(childA, array, minIdx, midIdx)
	BuildRangeTree(childB, array, midIdx, maxIdx)

	## set the minimums on the way back up
	node.minVal = min(childA.minVal, childB.minVal)

def QueryRangeTree(node, array, minIdx, maxIdx):
	assert(maxIdx > minIdx)

	if (node == None):
		return -1

	## query-range does not intersect node-range (eg. node=[1,3) and query=[4,7))
	if (minIdx >= node.maxIdx or maxIdx <= node.minIdx):
		return -1

	## query-range fully encompasses node-range (eg. node=[2,6) and query=[1,7))
	if (node.minIdx >= minIdx and node.maxIdx <= maxIdx):
		return node.minVal

	p1 = QueryRangeTree(node.childA, array, minIdx, maxIdx)
	p2 = QueryRangeTree(node.childB, array, minIdx, maxIdx)

	if (p1 == -1): return p2
	if (p2 == -1): return p1

	return (min(p1, p2))





def pprint(M):
	N = len(M)
	for i in xrange(N):
		print(i, N - 1, M[i])

## GENERAL RMQ Problem: consecutive elements in A have arbitrary values
## RESTRICTED RMQ Problem: consecutive elements in A differ by exactly 1
##
## the general RMQP can be solved in <O(N), O(log N)> time
## the restricted RMQP can be solved in <O(N), O(1)> time
##
##
## let RMQ_A(i, j) be the minimum value of A[i: j]
##
## for every pair of indices (i, j) store the INDEX
## of RMQ_A(i, j) in a 2D table M of size N * N
##
## O(N^2) pre-processing time and space, O(1) lookup
def BuildGeneralQueryTable(M, A):
	N = len(A)

	for i in xrange(N):
		M[i][i] = i

	for i in xrange(N):
		for j in xrange(i + 1, N):
			k = M[i][j - 1]

			if (A[k] < A[j]):
				M[i][j] = k
			else:
				M[i][j] = j

	return M

def BuildRestrictedQueryTable(M, A):
	N = len(A)
	V = [0] * (N - 1)

	for i in xrange(N - 1):
		D = A[i] - A[i + 1]
		V[i] = D
		assert(abs(D) == 1)

	for i in xrange(N - 1):
		A[i] = A[i] - A[i + 1]

	## what now?
	## print(A, V)
	return M


X = [0, 5, 2, 5, 4, 3, 1, 6, 3]
Y = [4, 5, 6, 5, 6, 7, 8, 7, 6]
R = [   1, 2, 3, 4, 5, 6, 7, 8]
M = [[-1] * len(X) for i in xrange(len(X))]

pprint(BuildGeneralQueryTable(M, X))

print
print(M[0][3], X[M[0][3]]); assert(X[M[0][3]] == 0)
print(M[1][3], X[M[1][3]]); assert(X[M[1][3]] == 2)
print(M[2][7], X[M[2][7]]); assert(X[M[2][7]] == 1)
print(M[3][5], X[M[3][5]]); assert(X[M[3][5]] == 3)
print

pprint(BuildRestrictedQueryTable(M, Y))


## tree-based queries
rootNode = Node(None, None, None, 0, len(R))

## BuildRangeTree(n, R, n.minIdx, n.maxIdx)
BuildRangeTree(rootNode, X[0: 8], rootNode.minIdx, rootNode.maxIdx)

rootNode.Print("")

print("")
print("X[0: 8]=%s" % X[0: 8])
print("X[2: 7]=%s" % X[2: 7])
print("min(X[2: 7]=%d" % QueryRangeTree(rootNode, X[0: 8], 2, 7))
print("min(X[3: 6]=%d" % QueryRangeTree(rootNode, X[0: 8], 3, 6))
print("")

