import sys
import random

class Cluster:
	def __init__(self, index, points):
		self.index = index
		self.points = points
		self.center = [0.0, 0.0]
		self.bounds = [[0.0, 0.0], [0.0, 0.0]]

		self.Update()
	def __repr__(self):
		s = "["
		for p in self.points:
			s += (" (%.3f, %.3f) " % (p[0], p[1]))
		s += "]"
		return s

	def GetIndex(self): return self.index

	def Merge(self, cluster):
		assert(cluster != None)

		for p in cluster.points:
			if (p not in self.points):
				self.points.append(p)

		## set new center and bounds
		self.Update()
		return self

	def SquaredDistance(self, cluster):
		dx = self.center[0] - cluster.center[0]
		dy = self.center[1] - cluster.center[1]
		return ((dx * dx) + (dy * dy))
	def Distance(self, cluster):
		return (self.SquaredDistance(cluster) ** 0.5)

	def Update(self):
		self.center[0] = 0.0
		self.center[1] = 0.0
		self.bounds[0][0] =  99999999.0
		self.bounds[0][1] =  99999999.0
		self.bounds[1][0] = -99999999.0
		self.bounds[1][1] = -99999999.0

		for p in self.points:
			self.center[0] += p[0]
			self.center[1] += p[1]
			self.bounds[0][0] = min(self.bounds[0][0], p[0])
			self.bounds[0][1] = min(self.bounds[0][1], p[1])
			self.bounds[1][0] = max(self.bounds[1][0], p[0])
			self.bounds[1][1] = max(self.bounds[1][1], p[1])

		self.center[0] /= len(self.points)
		self.center[1] /= len(self.points)



## Intuitively, the nearest neighbor chain algorithm repeatedly follows
## a chain of clusters A->B->C-> ... where each cluster is the nearest
## neighbor of the previous, until reaching a pair of clusters that are
## mutual nearest neighbors.
##
## The algorithm performs the following steps:
##   Initialize the set of active clusters to consist of n one-point
##   clusters, one for each input point.
##
##   Let S be a stack data structure, initially empty, the elements
##   of which will be active clusters.
##
##   While there is more than one cluster in the set of clusters:
##     If S is empty, choose an active cluster arbitrarily and push it onto S.
##
##     Let C be the active cluster on the top of S. Compute the distances
##     from C to all other clusters, and let D be the nearest other cluster.
##
##     If D is already in S, it must be the immediate predecessor of C.
##     (because of the chaining assumption A->B->C->..)
##     Pop both clusters from S, merge them, and push the merged cluster
##     onto S.
##
##     Otherwise, if D is not already in S, push it onto S.
##
## If there may be multiple equal nearest neighbors to a cluster, the
## algorithm requires a consistent tie-breaking rule: for instance, in
## this case, the nearest neighbor may be chosen, among the clusters at
## equal minimum distance from C, by numbering the clusters arbitrarily
## and choosing the one with the smallest index.

def FindNearestCluster(C, A):
	minDistance = 1e30
	minCluster = None

	## compute the distances from C to all other clusters
	for i in xrange(len(A)):
		cluster = A[i]

		## reference-comparison
		if (cluster == C):
			continue

		distance = C.SquaredDistance(cluster)

		if (distance < minDistance):
			minDistance = distance
			minCluster = cluster

	return minCluster

def PushActiveClusterIfEmptyStack(S, A):
	if (len(S) == 0):
		k = random.randint(0, len(A) - 1)
		S = [ A[k] ]
		A = A[0: k] + A[k + 1: ]

	return S, A
	
def ChainNearestNeighborCluster(A):
	## start with an empty cluster-stack
	S = []
	R = random.randint

	while (len(A) > 1):
		## if S is empty, choose an active cluster arbitrarily
		## and push it onto S (removing it from the active set)
		S, A = PushActiveClusterIfEmptyStack(S, A)

		## let C be the active cluster on the top of S
		## let D be the nearest other cluster to C
		C = S[0]
		D = FindNearestCluster(C, A)

		assert(C != D)
		assert(D != None)

		if (D in S):
			## pop both clusters from S, merge them, and push the
			## merged cluster onto S; the immediate predecessor of
			## C (stack-index 0) must be D (stack-index 1)
			assert(len(S) >= 2)
			assert(S[0] == C)
			## doesn't always hold because of FindNearestCluster
			## (the "requires consistent tie-breaking rule" part)
			## assert(S[1] == D)

			S = S[2: ]
			C = C.Merge(D)
			S = [C] + S

			## remove D from the active set, note that this will
			## invalidate indices of *all* clusters following i
			i = A.index(D)
			A = A[0: i] + A[i + 1: ]
		else:
			## if D is not already in S, push it onto S
			S = [D] + S

	## final stack contains the cluster-chain
	return S

def main(argc, argv):
	X = ((argc == 3) and int(argv[1])) or 32
	Y = ((argc == 3) and int(argv[2])) or 16

	## points, initial active single-point clusters, resulting cluster-chain
	P = [(X * random.random(), Y * random.random()) for n in xrange(X * Y)]
	C = [Cluster(i, [P[i]]) for i in xrange(len(P))]
	C = ChainNearestNeighborCluster(C)

	points_file = open("data_points.dat", 'w')
	clusts_file = open("data_clusts.dat", 'w')

	for p in P: points_file.write("%f\t%f\n" % (p[0], p[1]))
	for c in C: clusts_file.write("%f\t%f\n" % (c.center[0], c.center[1]))

	points_file.close()
	clusts_file.close()
	return 0

sys.exit(main(len(sys.argv), sys.argv))

